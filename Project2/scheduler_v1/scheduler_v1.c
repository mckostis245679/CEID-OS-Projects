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

typedef struct pirines { //---------------------- new: struct gia kathe pirina
	int pid;
	proc_t *proc;
} pirines_t; //----------------------

pirines_t *cores_proc; //---------------------- new:pinakas apo pirines pou tha exi to pid tou process kai to idio to process pou trexei ston pirina

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

#define FCFS	0
#define RR		1
#define RR_AFF	2

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
			fcfs();
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

	 for (int i = 0; i < number_of_cores; i++) { //an den exoun teleiwsei oles oi diergasies se olous tous pirines den stamatame
        if (cores_proc[i].pid != -1) {
            waitpid(cores_proc[i].pid, NULL, 0);
        }
    }

	printf("WORKLOAD TIME: %.2lf secs\n", proc_gettime()-global_t);
	printf("scheduler exits\n");
	return 0;
}


void fcfs()
{
	proc_t *proc;
	int pid;
	int status;

	while (1) {
		// printf("Dequeued process with name %s\n", proc->name);
		int available_core = -1;

		for(int i = 0; i < number_of_cores; i++) {  // psaxnw gia free core, an vrw vgainw kai kanw oti einai na kanw
			if(cores_proc[i].pid == -1) {
				available_core = i;
				break;
			}
		}

		 if (available_core == -1) { //alliws an den exei free core perimenw
            int finished_pid = waitpid(-1, &status, 0); //perimew na teleiwsei opoiadipote auto paei na pei to -1 sto orisma
            if (finished_pid > 0) { //an teleiwse kapoia diergasia
                for (int i = 0; i < number_of_cores; i++) { //psaxnw poios pirinas exei to pid pou teleiwse
                    if (cores_proc[i].pid == finished_pid) {   //an to vrw
                        cores_proc[i].pid = -1; //eleuthervnw ton pirina
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
				proc->pid = pid;
				cores_proc[available_core].pid = pid;
				proc->status = PROC_RUNNING;
				cores_proc[available_core].proc = proc;
				proc->t_end = proc_gettime();
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

void sigchld_handler(int signo, siginfo_t *info, void *context) {
    printf("child %d exited\n", info->si_pid);
    for (int i = 0; i < number_of_cores; i++) {
        if (cores_proc[i].pid == info->si_pid) {
            printf("Core %d: Process %d (%s) exited\n", i, info->si_pid, cores_proc[i].proc->name);
            cores_proc[i].proc->status = PROC_EXITED;
            cores_proc[i].proc->t_end = proc_gettime();
            printf("PID %d - CMD: %s\n", cores_proc[i].proc->pid, cores_proc[i].proc->name);
            printf("\tElapsed time = %.2lf secs\n", cores_proc[i].proc->t_end - cores_proc[i].proc->t_submission);
            printf("\tExecution time = %.2lf secs\n", cores_proc[i].proc->t_end - cores_proc[i].proc->t_start);
            printf("\tWorkload time = %.2lf secs\n", cores_proc[i].proc->t_end - global_t);

            cores_proc[i].pid = -1; // kanto free
            cores_proc[i].proc = NULL;
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
   
    for (int i = 0; i < number_of_cores; i++) {
        if (cores_proc[i].pid == -1) { //an yparxei free core
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
                    cores_proc[i].pid = pid;
                    cores_proc[i].proc = proc;
                    proc->status = PROC_RUNNING;
                }
            } else if (proc->status == PROC_STOPPED) { // an einai stamatimenh ksanaksekina th
                proc->status = PROC_RUNNING;
                cores_proc[i].proc = proc;
                cores_proc[i].pid = proc->pid;
                kill(proc->pid, SIGCONT); // to kill den skotwnei aparethta!!! apla stelnei shmata
            }
        }
    }

    //koimhsou osos to kvanto
    nanosleep(&req, &rem);

    // afou primenes oso to kvanto twra stamata tis diergasies
    for (int i = 0; i < number_of_cores; i++) {
        if (cores_proc[i].pid != -1) {
            proc_t *proc = cores_proc[i].proc;
            if (proc->status == PROC_RUNNING) {
                kill(proc->pid, SIGSTOP);
                proc->status = PROC_STOPPED;
                proc_to_rq_end(proc);
                cores_proc[i].pid = -1; //tis stamathses tis ekanes requeue kai twra eleutherwneis ton pisrina
                cores_proc[i].proc = NULL;
            }
        }
    }

    // an teleisane oles oi diergasies kai den exei alloi sthn oura
    int all_idle = 1;
    for (int i = 0; i < number_of_cores; i++) {
        if (cores_proc[i].pid != -1) {
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
		for (int i = 0; i < number_of_cores; i++) {
			if (cores_proc[i].pid == -1) { //an yparxei free core
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
						cores_proc[i].pid = pid;
						cores_proc[i].proc = proc;
						proc->status = PROC_RUNNING;
						proc->core_num = i; //gia na kserw se poio pirina trexei h diergasia
					}
				} else if (proc->status == PROC_STOPPED) { // an einai stamatimenh ksanaksekina th
						if(proc->core_num==i){
						proc->status = PROC_RUNNING;
						cores_proc[i].proc = proc;
						cores_proc[i].pid = proc->pid;
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
		for (int i = 0; i < number_of_cores; i++) {
			if (cores_proc[i].pid != -1) {
				proc_t *proc = cores_proc[i].proc;
				if (proc->status == PROC_RUNNING) {
					kill(proc->pid, SIGSTOP);
					proc->status = PROC_STOPPED;
					proc_to_rq_end(proc);
					cores_proc[i].pid = -1; //tis stamathses tis ekanes requeue kai twra eleutherwneis ton pisrina
					cores_proc[i].proc = NULL;
				}
			}
		}

		// an teleisane oles oi diergasies kai den exei alloi sthn oura
		int all_idle = 1;
		for (int i = 0; i < number_of_cores; i++) {
			if (cores_proc[i].pid != -1) {
				all_idle = 0;
				break;
			}
		}
		if (all_idle && proc_queue_empty(&global_q)) {
			break; //telos den emeine tpt
		}
	}
}

