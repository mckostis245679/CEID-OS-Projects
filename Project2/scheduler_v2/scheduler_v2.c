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
	int pid;
	proc_t *proc;
} pirines_t; //----------------------

pirines_t *cores_proc;  //----------------------- new:pinakas apo pirines pou tha exi to pid tou process kai to idio to process pou trexei ston pirina

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
	if (proc_queue_empty (&global_q))
		global_q.last = proc;
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
	register proc_t *proc;

	proc = global_q.first;
	if (proc==NULL) return NULL;

	proc = global_q.first;
	if (proc!=NULL) {
		global_q.first = proc->next;
		proc->next = NULL;
	}

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
	int c;
	proc_t *proc;
	int num_cores;
	
	

	if (argc == 1) {
		err_exit("invalid usage");
	} else if (argc == 2) {
		input = fopen(argv[1],"r");
		if (input == NULL) err_exit("invalid input file name");
	} else if (argc > 2) {
		if (!strcmp(argv[1],"FCFS")) {
			policy = FCFS;
			input = fopen(argv[2],"r");
			if (input == NULL) err_exit("invalid input file name");
		} else {
			err_exit("invalid usage");
		}	      
	} 
	
	
	if (argc > 3) { //----------------------- NEW: an exei 4o orisma (DLD >1 CORES)
		number_of_cores = atoi(argv[argc-1]);
		if (number_of_cores < 0) {
			err_exit("invalid number of cores");
		} else if (number_of_cores == 0) { //an einai 0 dld den evale orisma tote paei sto default me 1 core *an to afhsei keno tote to sistima tou vazei timh 0 idk why*
			number_of_cores = 1;
		}
	} else {
		number_of_cores = 1;
	}

	cores_proc = malloc(number_of_cores*sizeof(pirines_t)); //-----------new: arxikopoioume ta cores me -1 giati ola einai free arxika kai desmeuoume mnhmh gia tous pirines
	for(int i = 0; i < number_of_cores; i++) {
		cores_proc[i].pid = -1;
	}

	/* Read input file */
	while ((c=fscanf(input, "%s %d", exec, &num_cores))!=EOF) {
		// printf("fscanf returned %d\n", c);
		// printf("exec = %s\n", exec);

		proc = malloc(sizeof(proc_t));
		proc->next = NULL;
		strcpy(proc->name, exec);
		proc->pid = -1;
		proc->status = PROC_NEW;
		proc->t_submission = proc_gettime();


		if (num_cores > 0) {
			proc->cores_required = num_cores;
		} else if (num_cores == 0) {
			num_cores = 1;
		} else {
			err_exit("invalid number of cores");
		}

		
		proc_to_rq_end(proc);
	}

	//print_queue(&global_q);

  	global_t = proc_gettime();
	switch (policy) {
		case FCFS:
			fcfs();
			break;

		default:
			err_exit("Unimplemented policy");
			break;
	}

	 for (int i = 0; i < number_of_cores; i++) { //an den exoun teleiwsei oles oi diergasies se olous tous pirines den stamatame
        if (cores_proc[i].pid != -1) {
            waitpid(cores_proc[i].pid, NULL, 0);
        }
    }

	
	printf("WORKLOAD TIME: %.2lf secs\n", proc_gettime()-global_t);
	printf("scheduler exits\n");
	return 0;
}

/// den douleuei h fcfs

void fcfs()
{
	proc_t *proc;
	int pid;
	int status;
	int available_cores = number_of_cores; //arxika oloi oi pirines einai eleutheres

	while (1) {
		// printf("Dequeued process with name %s\n", proc->name);
	
		 if (available_cores == 0) { //alliws an den exei free core perimenw
            int finished_pid = waitpid(-1, &status, 0); //perimew na teleiwsei opoiadipote auto paei na pei to -1 sto orisma
            if (finished_pid > 0) { //an teleiwse kapoia diergasia
                for (int i = 0; i < number_of_cores; i++) { //psaxnw poios pirinas exei to pid pou teleiwse
                    if (cores_proc[i].pid == finished_pid) {   //an to vrw
                        cores_proc[i].pid = -1; //eleuthervnw ton pirina
						available_cores+=cores_proc[i].proc->cores_required; //auksanw ta free cores kata to posa pirines xreiastike
                        break;
                    }
                }
            }
            continue; 
        }


 		proc = proc_rq_dequeue();
        if (proc == NULL) {
            // edw den douleue an to evaza mesa sto megalo while kai etsi mou doulepse
            break;
        }


		if (proc->status == PROC_NEW) {
			proc->t_start = proc_gettime();
			pid = fork();
			if (pid == -1) {
				err_exit("fork failed!");
			}
			if (pid == 0) {
				printf("executing %s in core %d\n", proc->name, available_core);
				execl(proc->name, proc->name, NULL);
			} else { 
				cores_proc[available_core].pid = pid;
				proc->status = PROC_RUNNING;
				cores_proc[available_core].proc = proc;
				cores_proc[available_core].proc->t_end = proc_gettime();
			   //afou paizw me pirines prepei na kanw waitpid gia kathe pirina pou auto ginetai katw edw apla gemizoun oloi prwta
				printf("PID %d - CMD: %s\n", pid, proc->name);
				printf("\tElapsed time = %.2lf secs\n", proc->t_end-proc->t_submission);
				printf("\tExecution time = %.2lf secs\n", proc->t_end-proc->t_start);
				printf("\tWorkload time = %.2lf secs\n", proc->t_end-global_t);
			 }
		}
	}


	for (int i = 0; i < number_of_cores; i++) { //an den exoun teleiwsei oles oi diergasies se olous tous pirines den stamatame
    if (cores_proc[i].pid != -1) {
        waitpid(cores_proc[i].pid, NULL, 0); 
		cores_proc[i].pid = -1;
    }
}
}


