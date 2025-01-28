/* Mavridis Konstantinos, 1100620 */
/* Mitsainas Mixail-Xaralampos, 1100628 */
/* Loukanaris Konstantinos, 1100610 */

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

void fcfs_v1();
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
	int core_index;
	
} proc_t;

struct single_queue {
	proc_t	*first;
	proc_t	*last;
	long members;
};


#define FREE    0
#define BUSY    1

typedef struct core { 
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
	if (proc!=NULL) {
		global_q.first = proc->next;
		proc->next = NULL;
	}else return NULL;

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

#define MAX_CORES 32

int number_of_cores = 1;
core cores[MAX_CORES];

#define MAX_PIDS 32768  

proc_t*  pid_hmap[MAX_PIDS];//hashmap for freed cores 

int main(int argc,char **argv)
{
	FILE *input;
	char exec[80];
	int c;
	proc_t *proc;

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
		}else if (!strcmp(argv[1],"RR_AFF")) {
			policy = RR_AFF;
			quantum = atoi(argv[2]);
			input = fopen(argv[3],"r");
			if (input == NULL) err_exit("invalid input file name");
		}else {
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

	for (int i = 0; i < MAX_CORES; i++) {
    	cores[i].proc = NULL;
   		cores[i].state = FREE;
	}

	/* Read input file */
	while ((c=fscanf(input, "%s", exec))!=EOF) {
		// printf("fscanf returned %d\n", c);
		// printf("exec = %s\n", exec);

		proc = malloc(sizeof(proc_t));
		proc->next = NULL;
		strcpy(proc->name, exec);
		proc->pid = -1;
		proc->status = PROC_NEW;
		proc->t_submission = proc_gettime();
		proc_to_rq_end(proc);
	}

	//print_queue(&global_q);

  	global_t = proc_gettime();
	switch (policy) {
		case FCFS:
			fcfs_v1();
			break;

		case RR:
			rr();
			break;
		case RR_AFF:
			rr_aff();
			break;
		default:
			err_exit("Unimplemented policy");
			break;
	}

	printf("WORKLOAD TIME: %.2lf secs\n", proc_gettime()-global_t);
	printf("scheduler exits\n");
	return 0;
}

int check_busy_cores(){
    for (int i = 0; i < number_of_cores; i++) 
        if (cores[i].state == BUSY)  return 1;  
    return 0;   
}

void fcfs_v1()
{
    proc_t *proc;
    int status;
    // run until all processes completed and all cores are free
    while (proc_queue_empty(&global_q)!=1 || check_busy_cores()==1) 
    {
        for (int i = 0; i < number_of_cores; i++) {
			//fill evey free core
            if (cores[i].state == FREE) {
                if (proc_queue_empty(&global_q)!=1) {
                    proc = proc_rq_dequeue();
                    if (proc->status == PROC_NEW) {
                        proc->t_start = proc_gettime();
                        int pid = fork();
                        if (pid == -1) {
                            err_exit("fork failed");
                        }
                        if (pid == 0) {
                            printf("executing %s\n", proc->name);
                            execl(proc->name, proc->name, NULL);
                        } else {
                            proc->pid = pid;
                            proc->status = PROC_RUNNING;
							
							//hashmap child_id to proc
							pid_hmap[proc->pid] = proc;
							proc->core_index=i;
							//change core state
                            cores[i].proc = proc;
                            cores[i].state = BUSY;
                        }
                    }
            
                }
            }
        }

    // wait for any child to finish 
        int child_pid = wait(&status);
        if (child_pid > 0) {    
			proc_t * proc=pid_hmap[child_pid];
			int core_index=proc->core_index;
			//free core that completed child
			if (proc!=NULL) {
				proc->status = PROC_EXITED;
				proc->t_end = proc_gettime();
				printf("PID %d - CMD: %s\n", proc->pid, proc->name);
				printf("\tElapsed time = %.2lf secs\n",proc->t_end - proc->t_submission);
				printf("\tExecution time = %.2lf secs\n", proc->t_end - proc->t_start);
				printf("\tWorkload time = %.2lf secs\n",  proc->t_end - global_t);
			}
			proc = NULL;
			cores[core_index].state = FREE;
    	} 
	}
    printf("FCFS completed all processes.\n");
}


void sigchld_handler(int signo, siginfo_t *info, void *context) {
    proc_t * proc=pid_hmap[info->si_pid];
	int core_index=proc->core_index;
    if (proc!=NULL) {
        printf("FINISHED :Process %s  in Core %d\n", proc->name,core_index);
        proc->status = PROC_EXITED;
        proc->t_end = proc_gettime();
        printf("PID %d - CMD: %s\n", proc->pid, proc->name);
        printf("\tElapsed time = %.2lf secs\n", proc->t_end - proc->t_submission);
        printf("\tExecution time = %.2lf secs\n", proc->t_end - proc->t_start);
        printf("\tWorkload time = %.2lf secs\n", proc->t_end - global_t);
    }
	cores[core_index].state = FREE;
    cores[core_index].proc = NULL; 
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

    while (proc_queue_empty(&global_q)!=1 || check_busy_cores()==1) { 
   
		for (int i = 0; i < number_of_cores; i++) {
			//fill evey free core
			if (cores[i].state == FREE) { 
				if (proc_queue_empty(&global_q)!=1) {
					proc = proc_rq_dequeue();
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
							proc->status = PROC_RUNNING;
							//hashmap child_id to proc
							pid_hmap[proc->pid] = proc;
							proc->core_index=i;
							//change core state
							cores[i].proc = proc;
							cores[i].state = BUSY;
						}
					} 
					else if (proc->status == PROC_STOPPED) { 
						proc->status = PROC_RUNNING;
						proc->core_index=i;
						printf("RE-STARTED :Process %s in core %d\n", proc->name, i);
						cores[i].proc = proc;
						cores[i].state = BUSY;
						kill(proc->pid, SIGCONT); // resume process
					}
				}
			}
		}

		//koimhsou osos to kvanto
		nanosleep(&req, &rem);

		// afou primenes oso to kvanto twra stamata tis diergasies
		for (int i = 0; i < number_of_cores; i++) {
			if (cores[i].state == BUSY) {
				proc_t *proc = cores[i].proc;
				if (proc != NULL && proc->status == PROC_RUNNING) {
					kill(proc->pid, SIGSTOP);
					proc->status = PROC_STOPPED;
					proc_to_rq_end(proc);
					printf("stopping %s in core %d\n", proc->name, i);
					cores[i].state = FREE; 
					cores[i].proc = NULL;
				}
			}
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

    while (proc_queue_empty(&global_q)!=1 || check_busy_cores()==1) { 
   
		for (int i = 0; i < number_of_cores; i++) {
			//fill evey free core
			if (cores[i].state == FREE) { 
				if (proc_queue_empty(&global_q)!=1) {
					proc = proc_rq_dequeue();
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
							proc->status = PROC_RUNNING;
							//hashmap child_id to proc
							pid_hmap[proc->pid] = proc;
							proc->core_index=i;
							//change core state
							cores[i].proc = proc;
							cores[i].state = BUSY;
							proc->core_index=i;
						}
					} 
					else if (proc->status == PROC_STOPPED) { // an einai stamatimenh ksanaksekina th
						if(proc->core_index==i){
							proc->status = PROC_RUNNING;
							printf("RE-STARTED :Process %s in core %d\n", proc->name, i);
							cores[i].proc = proc;
							cores[i].state = BUSY;
							kill(proc->pid, SIGCONT); // to kill den skotwnei aparethta!!! apla stelnei shmata
						} else {
							proc_to_rq(proc); // Requeue if not the same core
							printf("ENSURING PROCESS AFFINITY for Process %s and core %d\n", proc->name, proc->core_index);
						}
							
					}
				}
			}
		}

		//koimhsou osos to kvanto
		nanosleep(&req, &rem);

		// afou primenes oso to kvanto twra stamata tis diergasies
		for (int i = 0; i < number_of_cores; i++) {
			if (cores[i].state == BUSY) {
				proc_t *proc = cores[i].proc;
				if (proc != NULL && proc->status == PROC_RUNNING) {
					kill(proc->pid, SIGSTOP);
					proc->status = PROC_STOPPED;
					proc_to_rq_end(proc);
					 printf("PAUSED :Process %s  in Core %d\n", proc->name,i);
					cores[i].state = FREE; //tis stamathses tis ekanes requeue kai twra eleutherwneis ton pisrina
					cores[i].proc = NULL;
				}
			}
		}

	}
}



