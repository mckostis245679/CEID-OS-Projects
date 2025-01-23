#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_LINE_LENGTH 80

void fcfs();

#define PROC_NEW	0
#define PROC_STOPPED	1
#define PROC_RUNNING	2
#define PROC_EXITED	3

typedef struct proc_desc {
	struct proc_desc *next;
	char name[80];
	int pid;
	int status;
	double t_submission, t_start, t_end;
	int cores_required; //----------------------- new: posa cores thelei h kathemia
} proc_t;

typedef struct pirines { //---------------------- new: struct gia kathe pirina
	int pid;        // to pid tou process pou einai assigned (-1 if free)
	int id;         // core id
} pirines_t; //----------------------

pirines_t *cores_proc;  //----------------------- new:pinakas apo pirines pou tha exi to pid tou process kai to idio to process pou trexei ston pirina
int total_cores;

struct single_queue {
	proc_t	*first;
	proc_t	*last;
	long members;
};

struct single_queue global_q;

#define proc_queue_empty(q) ((q)->first==NULL)

void proc_queue_init (register struct single_queue * q)
{
	q->first = q->last = NULL;
	q->members = 0;
}

void proc_to_rq (register proc_t *proc)
{
	if (proc_queue_empty (&global_q)) global_q.last = proc;
	proc->next = global_q.first;
	global_q.first = proc;
}

void proc_to_rq_end (register proc_t *proc)
{
	if (proc_queue_empty (&global_q))
		global_q.first = global_q.last = proc;
	else {
		global_q.last->next = proc;
		global_q.last = proc;
		proc->next = NULL;
	}
}

proc_t *proc_rq_dequeue ()
{
	proc_t *proc = global_q.first;
	if (proc==NULL) return NULL;

    global_q.first = proc->next;
    proc->next = NULL;

	return proc;
}


void print_queue()
{
	proc_t *proc;

	proc = global_q.first;
	while (proc != NULL) {
		printf("proc: [name:%s pid:%d]\n", 
			proc->name, proc->pid);
		proc = proc->next;
	}
}

double proc_gettime()
{
	struct timeval tv;
	gettimeofday(&tv, 0);
	return (double) (tv.tv_sec+tv.tv_usec/1000000.0);
}

#define FCFS 0

int policy = FCFS;
int quantum = 100;	/* ms */
proc_t *running_proc;
double global_t;
int number_of_cores = 1; //-----------------new: an den dei arg 3 paei sto default me 1 core



void err_exit(char *msg)
{
	printf("Error: %s\n", msg);
	exit(1);
}

int main(int argc,char **argv)
{
	 FILE *input;
    char exec[80];
    int num_cores;
    proc_t *proc;

    if (argc < 3) {
        fprintf(stderr, "Usage: %s <policy> <input_file> [number_of_cores]\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "FCFS") != 0) {
        fprintf(stderr, "Unsupported policy: %s\n", argv[1]);
        return EXIT_FAILURE;
    }

    input = fopen(argv[2], "r");
    if (!input) {
        perror("Failed to open input file");
        return EXIT_FAILURE;
    }

    total_cores = (argc > 3) ? atoi(argv[3]) : 1;
    if (total_cores <= 0) {
        fprintf(stderr, "Invalid number of cores\n");
        fclose(input);
        return EXIT_FAILURE;
    }

    cores_proc = malloc(total_cores * sizeof(pirines_t));
    for (int i = 0; i < total_cores; i++) {
        cores_proc[i].id = i;
        cores_proc[i].pid = -1;
    }

    while (fscanf(input, "%s %d", exec, &num_cores) != EOF) {
        proc = malloc(sizeof(proc_t));
        strcpy(proc->name, exec);
        proc->pid = -1;
        proc->status = PROC_NEW;
        proc->t_submission = proc_gettime();
        proc->cores_required = (num_cores > 0) ? num_cores : 1;
        proc_to_rq_end(proc);
    }

    fclose(input);

    global_t = proc_gettime();
    fcfs();

    free(cores_proc);
    printf("WORKLOAD TIME: %.2lf secs\n", proc_gettime() - global_t);
    return 0;
}


void fcfs()
{
	while (!proc_queue_empty(&global_q)) {
        proc_t *proc = proc_rq_dequeue();
		int available_core = -1;
        if(proc == NULL) break;

        //check if process needs more cores than available
        if(proc->cores_required > total_cores){
            printf("process %s requires %d cores, but only %d are available. freeing process...", proc->name, proc->cores_required, total_cores);
            free(proc);
            continue;
        }

        int allocated_cnt = 0;
        for(int i = 0; i < total_cores; ++i){
            if(cores_proc[i].pid == -1 && allocated_cnt < proc->cores_required){
                cores_proc[i].pid = proc->pid; //desmeui to core se auto to process
                allocated_cnt++;
            }
        }
        
        if(allocated_cnt < proc->cores_required){
            //not enough cores, release cores and requeue process
            for(int i = 0; i < total_cores; ++i){
                if(cores_proc[i].pid == proc->pid) cores_proc[i].pid = -1;
            }
            printf("Not enoigh cores for process %s. requeuing to end of queue.\n", proc->name);
            proc_to_rq_end(proc);
            continue;
        }

        proc->status = PROC_RUNNING;
        proc->t_start = proc_gettime();

        int pid = fork();
        if (pid == -1){
            perror("fork failed");
            exit(EXIT_FAILURE);
        }

        if(pid == 0){
            printf("Executing process %s on %d cores.\n", proc->name, proc->cores_required);
            execl(proc->name, proc->name, NULL);
            perror("execl failed");
            exit(EXIT_FAILURE);
        }
        else{
            proc->pid = pid;
            printf("Process %s (PID %d) is running.\n", proc->name, pid);
        }

        int status;
        waitpid(pid, &status, 0);

        for(int i = 0; i < total_cores; ++i){
            if(cores_proc[i].pid == proc->pid) cores_proc[i].pid = -1;
        }

        proc->status = PROC_EXITED;
        proc->t_end = proc_gettime();

        printf("Process %s has finished.\n", proc->name);
        printf("\tElapsed time = %.2lf secs\n", proc->t_end - proc->t_submission);
        printf("\tExecution time = %.2lf secs\n", proc->t_end - proc->t_start);

        free(proc);
    }		
}
