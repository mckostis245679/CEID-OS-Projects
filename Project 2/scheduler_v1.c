#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <sys/wait.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h> 
#include <sys/mman.h> 

#define MAX_LINE_LENGTH 80

void fcfs();
void rr();
void rr_aff();

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
	int core_num; //---------------------- new: gia na kserw se poio pirina trexei h diergasia (rr_aff)
} proc_t;


struct single_queue {
	proc_t	*first;
	proc_t	*last;
	long members;
};

#define FREE    0
#define BUSY    1

typedef struct core { 
	int core_id;
	int state;
	proc_t *proc;
} core; 




struct single_queue global_q;

#define proc_queue_empty(q) ((q)->first==NULL)

void proc_queue_init (register struct single_queue * q)
{
	q->first = q->last = NULL;
	q->members = 0;
}

void proc_to_rq (register proc_t *proc,struct single_queue * q)
{
	if (proc_queue_empty (q))
		q->last = proc;
	proc->next = q->first;
	q->first = proc;
}

void proc_to_rq_end (register proc_t *proc,struct single_queue * q)
{
	if (proc_queue_empty (q))
		q->first = q->last = proc;
	else {
		q->last->next = proc;
		q->last = proc;
		proc->next = NULL;
	}
}

proc_t *proc_rq_dequeue (struct single_queue * q)
{
	register proc_t *proc;

	proc = q->first;
	if (proc==NULL) return NULL;

	proc = q->first;
	if (proc!=NULL) {
		q->first = proc->next;
		proc->next = NULL;
	}

	return proc;
}


void print_queue(struct single_queue * q)
{
	proc_t *proc;

	proc = q->first;
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

void err_exit(char *msg)
{
	printf("Error: %s\n", msg);
	exit(1);
}

#define FCFS	0
#define RR		1
#define RR_AFF	2

int policy = FCFS;
int quantum = 100;	/* ms */
proc_t *running_proc;
double global_t;
int number_of_cores = 1; //-----------------new: an den dei arg 3 paei sto default me 1 core
int number_of_processes=0;


typedef struct {
    struct single_queue global_q;
	struct core *cores;
	int running_cores;
	int completed_processes;
} shared_data_t;


shared_data_t *data;



int main(int argc,char **argv)
{
	
	FILE *input;
	char exec[80];
	int c;
	proc_t *proc;

	data = mmap(NULL, sizeof(shared_data_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS,-1, 0);


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
		} else if (!strcmp(argv[1],"RR")) {
			policy = RR;
			quantum = atoi(argv[2]);
			input = fopen(argv[3],"r");
			if (input == NULL) err_exit("invalid input file name");
		} else if (!strcmp(argv[1],"RR_AFF")) {
			policy = RR_AFF;
			quantum = atoi(argv[2]);
			input = fopen(argv[3],"r");
			if (input == NULL) err_exit("invalid input file name");
		}else {
			err_exit("invalid usage");
		}	      
	} 
	
	
	if (argc > 3) { 
		number_of_cores = atoi(argv[argc-1]);
		if (number_of_cores < 0) {
			err_exit("invalid number of cores");
		} 
	} else {
		number_of_cores = 1;
	}
	
	data->running_cores=0;
	data->completed_processes=0;

	data->cores = malloc(number_of_cores*sizeof(core)); 
	for(int i = 0; i < number_of_cores; i++) {
		data->cores[i].state = FREE;
	}

	number_of_processes=0;
	
	/* Read input file */
	while ((c=fscanf(input, "%s", exec))!=EOF) {
		proc = malloc(sizeof(proc_t));
		proc->next = NULL;
		strcpy(proc->name, exec);
		proc->pid = -1;
		proc->status = PROC_NEW;
		proc->t_submission = proc_gettime();
		proc_to_rq_end(proc,&data->global_q);
		number_of_processes++;
	}

  	global_t = proc_gettime();
	switch (policy) {
		case FCFS:
			fcfs();
			break;

		case RR:
			//rr();
			break;
		case RR_AFF:
			//rr_aff();
			break;

		default:
			err_exit("Unimplemented policy");
			break;
	}

	printf("WORKLOAD TIME: %.2lf secs\n", proc_gettime()-global_t);
	printf("scheduler exits\n");
	munmap(data, sizeof(shared_data_t));
	return 0;
}



void fcfs()
{
	proc_t *proc;
	int status;
	sem_t *sem = sem_open("/mysem", O_CREAT, 0666, 1);

	while(1){
		while ( data->running_cores <number_of_cores &&  data->completed_processes< number_of_processes) {
			int outer_fork_id = fork();
            if (outer_fork_id < 0) {
                err_exit("INNER fork failed!");
            } 
			else if (outer_fork_id == 0) {
				sem_wait(sem); 
            	proc=proc_rq_dequeue(&data->global_q);
				if (proc==NULL) break;
            	sem_post(sem); 
                if (proc->status == PROC_NEW) {
					proc->t_start = proc_gettime();
					int pid = fork();
					if (pid == -1) {
						err_exit("INNER fork failed!");
					}
					if (pid == 0) {
						printf("executing %s\n", proc->name);
						execl(proc->name, proc->name, NULL);
					} 
					else {
						proc->pid = pid;
						proc->status = PROC_RUNNING;
						pid = waitpid(proc->pid, &status, 0);
						proc->status = PROC_EXITED;
						if (pid < 0) err_exit("waitpid failed");
						proc->t_end = proc_gettime();
						printf("PID %d - CMD: %s\n", pid, proc->name);
						printf("\tElapsed time = %.2lf secs\n", proc->t_end-proc->t_submission);
						printf("\tExecution time = %.2lf secs\n", proc->t_end-proc->t_start);
						printf("\tWorkload time = %.2lf secs\n", proc->t_end-global_t);
						sem_wait(sem); 
						data->running_cores--;
            			sem_post(sem); 
						exit(0);
					}
				}
            } 
			else{
				data->running_cores++;
				data->completed_processes++;
				
			}
		}
		sem_wait(sem); 
		if (data->running_cores==0 && data->completed_processes==number_of_processes) break;
        sem_post(sem); 
	}


	wait(NULL);//wait for child to die

	sem_close(sem);
    sem_unlink("/mysem");
} 

/*
void sigchld_handler(int signo, siginfo_t *info, void *context) {
    printf("child %d exited\n", info->si_pid);
    for (int i = 0; i < number_of_data->cores; i++) {
        if (data->cores[i].pid == info->si_pid) {
            printf("Core %d: Process %d (%s) exited\n", i, info->si_pid, data->cores[i].proc->name);
            data->cores[i].proc->status = PROC_EXITED;
            data->cores[i].proc->t_end = proc_gettime();
            printf("PID %d - CMD: %s\n", data->cores[i].proc->pid, data->cores[i].proc->name);
            printf("\tElapsed time = %.2lf secs\n", data->cores[i].proc->t_end - data->cores[i].proc->t_submission);
            printf("\tExecution time = %.2lf secs\n", data->cores[i].proc->t_end - data->cores[i].proc->t_start);
            printf("\tWorkload time = %.2lf secs\n", data->cores[i].proc->t_end - global_t);

            data->cores[i].pid = -1; // kanto free
            data->cores[i].proc = NULL;
            break;
        }
    }
}

void rr() {
    struct sigaction sig_act;
    proc_t *proc;
    int pid;
    struct timespec req, rem;

    req.tv_sec = quantum / 1000;
    req.tv_nsec = (quantum % 1000) * 1000000;

    printf("tv_sec = %ld\n", req.tv_sec);
    printf("tv_nsec = %ld\n", req.tv_nsec);

    // Setup the SIGCHLD handler
    sigemptyset(&sig_act.sa_mask);
    sig_act.sa_handler = 0;
    sig_act.sa_flags = SA_SIGINFO | SA_NOCLDSTOP;
    sig_act.sa_sigaction = sigchld_handler;
    sigaction(SIGCHLD, &sig_act, NULL);

    while (1) { 
   
    for (int i = 0; i < number_of_data->cores; i++) {
        if (data->cores[i].state == -1) { //an yparxei free core
            proc = proc_rq_dequeue();
            if (proc == NULL) {
                continue; // an teleiwsame phgene parakatw
            }

            if (proc->status == PROC_NEW) { 
                proc->t_start = proc_gettime();
                pid = fork();
                if (pid == -1) {
                    err_exit("fork failed!");
                }
                if (pid == 0) {
                    printf("executing %s in core %d\n", proc->name, i);
                    execl(proc->name, proc->name, NULL);
                } else {
                    proc->pid = pid;
                    data->cores[i].state = pid;
                    data->cores[i].proc = proc;
                    proc->status = PROC_RUNNING;
                }
            } else if (proc->status == PROC_STOPPED) { // an einai stamatimenh ksanaksekina th
                proc->status = PROC_RUNNING;
                data->cores[i].proc = proc;
                data->cores[i].state = proc->pid;
                kill(proc->pid, SIGCONT); // to kill den skotwnei aparethta!!! apla stelnei shmata
            }
        }
    }

    //koimhsou osos to kvanto
    nanosleep(&req, &rem);

    // afou primenes oso to kvanto twra stamata tis diergasies
    for (int i = 0; i < number_of_data->cores; i++) {
        if (data->cores[i].state != -1) {
            proc_t *proc = data->cores[i].proc;
            if (proc->status == PROC_RUNNING) {
                kill(proc->pid, SIGSTOP);
                proc->status = PROC_STOPPED;
                proc_to_rq_end(proc);
                data->cores[i].state = -1; //tis stamathses tis ekanes requeue kai twra eleutherwneis ton pisrina
                data->cores[i].proc = NULL;
            }
        }
    }

    // an teleisane oles oi diergasies kai den exei alloi sthn oura
    int all_idle = 1;
    for (int i = 0; i < number_of_data->cores; i++) {
        if (data->cores[i].pid != -1) {
            all_idle = 0;
            break;
        }
    }
    if (all_idle && proc_queue_empty(&global_q)) {
        break; //telos den emeine tpt
    }
}
}


void rr_aff() {
	struct sigaction sig_act;
	proc_t *proc;
	int pid;
	struct timespec req, rem;
	
	req.tv_sec = quantum / 1000;
	req.tv_nsec = (quantum % 1000) * 1000000;

	printf("tv_sec = %ld\n", req.tv_sec);
	printf("tv_nsec = %ld\n", req.tv_nsec);

	// Setup the SIGCHLD handler
	sigemptyset(&sig_act.sa_mask);
	sig_act.sa_handler = 0;
	sig_act.sa_flags = SA_SIGINFO | SA_NOCLDSTOP;
	sig_act.sa_sigaction = sigchld_handler;
	sigaction(SIGCHLD, &sig_act, NULL);

	while (1) { 
		for (int i = 0; i < number_of_data->cores; i++) {
			if (data->cores[i].pid == -1) { //an yparxei free core
				proc = proc_rq_dequeue();
				if (proc == NULL) {
					continue; // an teleiwsame phgene parakatw
				}

				if (proc->status == PROC_NEW) { 
					proc->t_start = proc_gettime();
					pid = fork();
					if (pid == -1) {
						err_exit("fork failed!");
					}
					if (pid == 0) {
						printf("executing %s in core %d\n", proc->name, i);
						execl(proc->name, proc->name, NULL);
					} else {
						proc->pid = pid;
						data->cores[i].pid = pid;
						data->cores[i].proc = proc;
						proc->status = PROC_RUNNING;
						proc->core_num = i; //gia na kserw se poio pirina trexei h diergasia
					}
				} else if (proc->status == PROC_STOPPED) { // an einai stamatimenh ksanaksekina th
						if(proc->core_num==i){
						proc->status = PROC_RUNNING;
						data->cores[i].proc = proc;
						data->cores[i].pid = proc->pid;
						kill(proc->pid, SIGCONT);
						} else {
						proc_to_rq(proc); // Requeue if not the same core
					}
				 }
				}
			}
		

		//koimhsou osos to kvanto
		nanosleep(&req, &rem);

		// afou primenes oso to kvanto twra stamata tis diergasies
		for (int i = 0; i < number_of_data->cores; i++) {
			if (data->cores[i].pid != -1) {
				proc_t *proc = data->cores[i].proc;
				if (proc->status == PROC_RUNNING) {
					kill(proc->pid, SIGSTOP);
					proc->status = PROC_STOPPED;
					proc_to_rq_end(proc);
					data->cores[i].pid = -1; //tis stamathses tis ekanes requeue kai twra eleutherwneis ton pisrina
					data->cores[i].proc = NULL;
				}
			}
		}

		// an teleisane oles oi diergasies kai den exei alloi sthn oura
		int all_idle = 1;
		for (int i = 0; i < number_of_data->cores; i++) {
			if (data->cores[i].pid != -1) {
				all_idle = 0;
				break;
			}
		}
		if (all_idle && proc_queue_empty(&global_q)) {
			break; //telos den emeine tpt
		}
	}
}*/