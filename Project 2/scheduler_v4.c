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
#define MAX_CPUS 64

static int num_cpus = 1;

struct cpu_state {
    int busy;
    struct proc_desc *proc;
} cpus[MAX_CPUS];

#define PROC_NEW      0
#define PROC_STOPPED  1
#define PROC_RUNNING  2
#define PROC_EXITED   3

typedef struct proc_desc {
    struct proc_desc *next;
    char name[80];
    int pid;
    int status;
    double t_submission, t_start, t_end;

    int requested_cpus;
    int assigned_count;
    int assigned_cpus[MAX_CPUS];
} proc_t;

struct single_queue {
    proc_t *first;
    proc_t *last;
    long members;
};

struct single_queue global_q;

#define proc_queue_empty(q) ((q)->first == NULL)

void proc_queue_init(struct single_queue *q) {
    q->first = q->last = NULL;
    q->members = 0;
}

void proc_to_rq(proc_t *proc) {
    if (proc_queue_empty(&global_q))
        global_q.last = proc;
    proc->next = global_q.first;
    global_q.first = proc;
}

void proc_to_rq_end(proc_t *proc) {
    if (proc_queue_empty(&global_q)) {
        global_q.first = global_q.last = proc;
        proc->next = NULL;
    } else {
        global_q.last->next = proc;
        global_q.last = proc;
        proc->next = NULL;
    }
}

proc_t *proc_rq_dequeue() {
    proc_t *proc = global_q.first;
    if (!proc) return NULL;
    global_q.first = proc->next;
    proc->next = NULL;
    if (!global_q.first)
        global_q.last = NULL;
    return proc;
}

double proc_gettime() {
    struct timeval tv;
    gettimeofday(&tv, 0);
    return (double)(tv.tv_sec + tv.tv_usec / 1000000.0);
}

#define FCFS  0
#define RR    1

int policy = FCFS;
double global_t;

void err_exit(const char *msg) {
    fprintf(stderr, "Error: %s\n", msg);
    exit(EXIT_FAILURE);
}

int all_finished() {
    if (!proc_queue_empty(&global_q)) return 0;
    for (int i = 0; i < num_cpus; i++) {
        if (cpus[i].busy) return 0;
    }
    return 1;
}

proc_t *find_proc_by_pid(pid_t pid) {
    for (int i = 0; i < num_cpus; i++) {
        if (cpus[i].busy && cpus[i].proc && cpus[i].proc->pid == pid) {
            return cpus[i].proc;
        }
    }
    return NULL;
}

void free_cpu_for_proc(proc_t *p) {
    for (int i = 0; i < p->assigned_count; i++) {
        int idx = p->assigned_cpus[i];
        cpus[idx].busy = 0;
        cpus[idx].proc = NULL;
    }
    p->assigned_count = 0;
}

int try_start_process(proc_t *p) {
    int free_indices[MAX_CPUS];
    int count_found = 0;
    for (int i = 0; i < num_cpus; i++) {
        if (!cpus[i].busy) {
            free_indices[count_found++] = i;
            if (count_found == p->requested_cpus) break;
        }
    }
    if (count_found < p->requested_cpus) {
        return 0;
    }
    for (int i = 0; i < count_found; i++) {
        cpus[free_indices[i]].busy = 1;
        cpus[free_indices[i]].proc = p;
        p->assigned_cpus[i] = free_indices[i];
    }
    p->assigned_count = count_found;

    if (p->status == PROC_NEW) {
        p->t_start = proc_gettime();
        pid_t pid = fork();
        if (pid == -1) err_exit("fork failed");
        if (pid == 0) {
            execl(p->name, p->name, NULL);
            err_exit("execl failed");
        } else {
            p->pid = pid;
            p->status = PROC_RUNNING;
        }
    } else if (p->status == PROC_STOPPED) {
        p->status = PROC_RUNNING;
        kill(p->pid, SIGCONT);
    }
    return 1;
}

void sigchld_handler(int signo, siginfo_t *info, void *context) {
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        proc_t *proc = find_proc_by_pid(pid);
        if (!proc) {
            fprintf(stderr, "Unknown process (PID %d) terminated.\n", pid);
            continue;
        }
        proc->status = PROC_EXITED;
        proc->t_end = proc_gettime();
        free_cpu_for_proc(proc);

        printf("PID %d (%s) finished:\n", proc->pid, proc->name);
        printf("\tElapsed time = %.2lf secs\n", proc->t_end - proc->t_submission);
        printf("\tExecution time = %.2lf secs\n", proc->t_end - proc->t_start);
    }
}

void fcfs() {
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO | SA_NOCLDSTOP;
    sa.sa_sigaction = sigchld_handler;
    sigaction(SIGCHLD, &sa, NULL);

    for (int i = 0; i < num_cpus; i++) {
        cpus[i].busy = 0;
        cpus[i].proc = NULL;
    }

    while (1) {
        while (!proc_queue_empty(&global_q)) {
            proc_t *p = proc_rq_dequeue();
            if (!p) break;
            if (!try_start_process(p)) {
                proc_to_rq(p);
                break;
            }
        }
        if (all_finished()) break;
        pause();
    }
}

int main(int argc, char **argv) {
    if (argc < 3) {
        err_exit("Usage: ./scheduler <policy> <num_cpus> <workload_file>");
    }

    if (!strcmp(argv[1], "FCFS")) {
        policy = FCFS;
    } else {
        err_exit("Unsupported policy. Use FCFS.");
    }

    num_cpus = atoi(argv[2]);
    if (num_cpus <= 0 || num_cpus > MAX_CPUS) {
        err_exit("Invalid number of CPUs.");
    }

    FILE *input = fopen(argv[3], "r");
    if (!input) {
        perror("Failed to open workload file");
        return EXIT_FAILURE;
    }

    proc_queue_init(&global_q);

    char exec[80];
    int requested_cpus;
    while (fscanf(input, "%s %d", exec, &requested_cpus) != EOF) {
        proc_t *proc = malloc(sizeof(proc_t));
        memset(proc, 0, sizeof(proc_t));
        strcpy(proc->name, exec);
        proc->pid = -1;
        proc->status = PROC_NEW;
        proc->t_submission = proc_gettime();
        proc->requested_cpus = (requested_cpus > 0) ? requested_cpus : 1;
        proc->assigned_count = 0;
        proc_to_rq_end(proc);
    }
    fclose(input);

    global_t = proc_gettime();

    if (policy == FCFS) {
        fcfs();
    }

    printf("Total workload time: %.2lf seconds\n", proc_gettime() - global_t);
    return 0;
}
