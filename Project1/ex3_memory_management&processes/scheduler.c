#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>

#define MEMORY_SIZE 512 // megethos mnimis KB
#define TQ 3 // Round Robin time quantum ms

typedef struct {
    int pid;
    int arrival_time;
    int duration;
    int remaining_time;
    int req_memory;
    bool in_memory;
} Process;

typedef struct {
    int start;
    int size;
    bool isfree;
    int pid;
} Memory_Block;

Memory_Block memory[MEMORY_SIZE]; //memory anaparistate ws memory block
int num_blocks = 0;

// arxikopoisi dynamic mnimis, xoris na eisaxthei kapoia diergasia akoma
void memory_init(){
    memory[0].start = 0;
    memory[0].size = MEMORY_SIZE;
    memory[0].isfree = true;
    memory[0].pid = -1;     //because free
    num_blocks = 1;
}

void print_memory(){
    printf("\n---Memory Status---\n");
    for(int i = 0; i < num_blocks; ++i)
        printf("Block %d: Start = %d, Size = %d, isFree = %s, pID = %d\n",
                    i, memory[i].start, memory[i].size, memory[i].isfree ? "yes" : "no", memory[i].pid);

}

void print_process_status(Process processes[], int n){
    printf("\n---Process Status---\n");
    for(int i = 0; i < n; i++){
        printf("pID = %d, Arrival = %d, Duration = %d, Remaining = %d, Memory = %d, InMemory = %s\n",
               processes[i].pid, processes[i].arrival_time, processes[i].duration,
               processes[i].remaining_time, processes[i].req_memory, processes[i].in_memory ? "yes" : "no");
    }
}

bool allocate_memory(Process *p){
    for(int i=0; i<num_blocks; ++i){
        if(memory[i].isfree && memory[i].size >= p->req_memory){
            if(memory[i].size > p->req_memory){
                for(int j=num_blocks; j>i; j--) memory[j] =  memory[j-1];

                num_blocks++;
                //to 2o miso toy new block pou einai empty (is_empty() == true)
                memory[i+1].start = memory[i].start + p->req_memory;
                memory[i+1].size = memory[i].size - p->req_memory;
                memory[i+1].isfree = true;
                memory[i+1].pid = -1;
            }
            //to 1o miso toy kainourioy block me to process
            memory[i].size = p->req_memory;
            memory[i].isfree = false;
            memory[i].pid = p->pid;
            p->in_memory = true;
            return true;
        }
    }
    return false;
}

bool deallocate_memory(int pid){
    int k = -1;

    //find and deallocate mem block
    for (int i = 0; i < num_blocks; ++i) {
        if (memory[i].pid == pid) {
            memory[i].isfree = true;
            memory[i].pid = -1;
            k = i;
            break;
        }
    }

    // If the block with the given PID was not found
    if (k == -1) {
        return false;
    }

    // kane merge me previous block if free (de-fragmentation)
    if (k > 0 && memory[k - 1].isfree) {
        memory[k - 1].size += memory[k].size;
        for (int j = k; j < num_blocks - 1; ++j) {
            memory[j] = memory[j + 1];
        }
        num_blocks--;
        k--; //kanonikopoisi epeidi meiothike kata 1 block i mnimi
    }

    //tsekarei to idio me to mprostino block
    if (k < num_blocks - 1 && memory[k + 1].isfree) {
        memory[k].size += memory[k + 1].size;
        for (int j = k + 1; j < num_blocks - 1; ++j) {
            memory[j] = memory[j + 1];
        }
        num_blocks--;
    }

    return true; 
}

//epistrefei tin epomeni diergasia me to mikrotero arrival_time
int next_arrival_time(Process processes[], int n, int cur_time){
    int min_arrival = INT_MAX;
    for(int i = 0; i < n; ++i){
        if(processes[i].remaining_time > 0){
            if(processes[i].arrival_time > cur_time) min_arrival = processes[i].arrival_time;
        }
    }
    if(min_arrival ==  INT_MAX) return -1;

    return min_arrival;
}

void round_robin(Process processes[], int p){
    int time = 0;
    bool done = false;

    while(!done){
        done = true;  //Assume all processes are completed
        bool any_ready = false; //an brikame kapoio process auton ton kiklo (arrival_time related)

        for(int i = 0; i < p; ++i){
            if(processes[i].remaining_time > 0 && processes[i].arrival_time <= time){   //an den exei teleiosei KAI exei erthei.
                done = false;  //Brethike process me != 0 burst time
                any_ready = true;

                //AN DEN einai stin mnimi, tote allocate
                if (!processes[i].in_memory) {
                    if(!allocate_memory(&processes[i])) continue; //skip process exec if allocation fails
                }

                //burst time > time quantum
                else if(processes[i].remaining_time > TQ){
                    time += TQ;
                    processes[i].remaining_time -= TQ;
                }
                //Process CAN BE completed
                else{ 
                    time += processes[i].remaining_time;
                    processes[i].remaining_time = 0;
                    deallocate_memory(processes[i].pid);
                    processes[i].in_memory = false; //deallocated
                }
                printf("\n[Time = %d] | Process %d is scheduled.\n", time, processes[i].pid);
                print_memory();
                print_process_status(processes, p);
            }
        }
        if(!any_ready){
            int jump = next_arrival_time(processes, p, time);
            if (jump == -1) break; //afou den vrike kati tote teleiosame
            else {
                time = jump; //skip forward in time gia an vrei to epomeno arriving process
                done = false;
            }
        }
    }
}

int main(){
    int p;
    printf("Enter amount of processes: ");
    scanf("%d", &p);
    Process processes[p];
    memory_init();

    for(int i=0; i<p; ++i){
        printf("Enter pID, Arrival Time, Duration (Burst Time), Required Memory(KB):\n");
        scanf("%d %d %d %d", &processes[i].pid, &processes[i].arrival_time, &processes[i].duration, &processes[i].req_memory);
        processes[i].remaining_time = processes[i].duration;
        processes[i].in_memory = false;
    }

    round_robin(processes, p);

    return 0;
}
