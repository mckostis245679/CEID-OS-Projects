#include "ipc_utils.h"

extern sem_t boat_space;
extern sem_t boarding_queue;
extern sem_t boats_available;

void init_semaphore(sem_t *sem, int value) {
    if (sem_init(sem, 0, value) != 0) {
        perror("sem_init failed");
        exit(EXIT_FAILURE);
    }
}

void destroy_semaphore(sem_t *sem) {
    if (sem_destroy(sem) != 0) {
        perror("sem_destroy failed");
        exit(EXIT_FAILURE);
    }
}

void wait_semaphore(sem_t *sem) {
    if (sem_wait(sem) != 0) {
        perror("sem_wait failed");
        exit(EXIT_FAILURE);
    }
}

void post_semaphore(sem_t *sem) {
    if (sem_post(sem) != 0) {
        perror("sem_post failed");
        exit(EXIT_FAILURE);
    }
}

void passenger_routine(int passenger_id, int total_passengers, int totalboats, int seats_per_boat) {
    static int remaining_passengers = -1;  // Arxika to remaining_passengers einai -1 gia na ginei h arxikopoiisi
    static int passengers_in_boat = 0;    
    static int boats_filled = 0;          
    
    if (remaining_passengers == -1) {
        remaining_passengers = total_passengers;  // an to remaining_passengers einai -1, arxikopoioume me to synoliko plithos epibaton
    }

    if (remaining_passengers <= 0) {
        return;  //An den exoun apomeinei epibates, termatizei h diadikasia
    }

    wait_semaphore(&boarding_queue);  //perimenoume seira gia na kanoume thn epibivasi

    if (remaining_passengers > 0) {
        wait_semaphore(&boats_available);  // Perimenoume na ginei diathesimh mia lemvos
        wait_semaphore(&boat_space);       // Perimenoume na vrethoun thesies sth lemvo

        remaining_passengers--; 
        printf("Passenger %d boards the boat no %d. Remaining passengers: %d\n", passenger_id, boats_filled+1 ,remaining_passengers);

        passengers_in_boat++; //bainei autos o enas

        if (passengers_in_boat == seats_per_boat || remaining_passengers == 0) {
            boats_filled++;  //an eixe gemisei, enhmerwnoume ta boats_filled
            for (int i = 0; i < totalboats; i++) {
                post_semaphore(&boats_available);  //h lemvos einai available pali
            }
            for (int i = 0; i < passengers_in_boat; i++) {
                post_semaphore(&boat_space);  //post gia na apeleytherwsoume tis theseis sth lemvo
            }

            if (boats_filled == totalboats || remaining_passengers == 0) {
                printf("%d boat(s) depart(s) with %d passengers each.\n", boats_filled, passengers_in_boat);
                sleep(2); 
                printf("The boats have returned.\n");
                boats_filled = 0;  //hrthan pisw ara pali apthn arxh
            }
            passengers_in_boat = 0;  //epanafora tou arithmou epibaton
        }
    }

    post_semaphore(&boarding_queue);  //post gia na mpei o epomenos epibaths
}
