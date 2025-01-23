#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h> //gia th sleep


// global shmaforoi
extern sem_t boat_space;
extern sem_t boarding_queue;
extern sem_t boats_available;


void init_semaphore(sem_t *sem, int value);
void destroy_semaphore(sem_t *sem);
void wait_semaphore(sem_t *sem);
void post_semaphore(sem_t *sem);

void passenger_routine(int passenger_id, int total_passengers, int totalboats, int seats_per_boat);


