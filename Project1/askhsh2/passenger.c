/* Mavridis Konstantinos, 1100620 */
/* Mitsainas Mixail-Xaralampos, 1100628 */
/* Loukanaris Konstantinos, 1100610 */
#include "ipc_utils.h"

extern sem_t boat_space;
extern sem_t boarding_queue;
extern sem_t boats_available;

void init_semaphore(sem_t *sem, int value) {
    if (sem_init(sem, 0, value) != 0) {
        perror("sem_init failed");
        exit(1);
    }
}

void destroy_semaphore(sem_t *sem) {
    if (sem_destroy(sem) != 0) {
        perror("sem_destroy failed");
        exit(1);
    }
}

void wait_semaphore(sem_t *sem) {
    if (sem_wait(sem) != 0) {
        perror("sem_wait failed");
        exit(1);
    }
}

void post_semaphore(sem_t *sem) {
    if (sem_post(sem) != 0) {
        perror("sem_post failed");
        exit(1);
    }
}


void passenger_routine(int passenger_id, int total_passengers, int totalboats, int seats_per_boat) {
    static int remaining_passengers = -1;
    static int passengers_in_boat = 0;    
    static int boats_filled = 0;          
    
    if (remaining_passengers == -1) {
        remaining_passengers = total_passengers;
    }

    if (remaining_passengers <= 0) {
        return;
    }

    wait_semaphore(&boarding_queue);

    if (remaining_passengers > 0) {
  
        wait_semaphore(&boat_space);

        remaining_passengers--; 
        printf("Passenger %d boards the boat no %d. Remaining passengers: %d\n", passenger_id, boats_filled+1 ,remaining_passengers);

        passengers_in_boat++;

        if (passengers_in_boat == seats_per_boat) {
            
            wait_semaphore(&boats_available);
            boats_filled++;
            
            if (boats_filled == totalboats) {
                printf("%d boat(s) depart(s) with %d passengers each.\n", boats_filled, passengers_in_boat);
                sleep(2); 
                printf("The boats have returned.\n");
                boats_filled = 0;
                 for (int i = 0; i < totalboats; i++) {
                      post_semaphore(&boats_available);
                 }
            }
              for (int i = 0; i < passengers_in_boat; i++) {
                 post_semaphore(&boat_space);
               }       
            passengers_in_boat = 0;
        } 
    }

    post_semaphore(&boarding_queue);
}


