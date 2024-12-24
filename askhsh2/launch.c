   #include "ipc_utils.h"

    #define MAX_PASSENGERS 100
    #define MAX_BOATS 10

    sem_t boat_space;
    sem_t boarding_queue;
    sem_t boats_available;

    int main() {

        srand(time(NULL));

        int passengers, boats, seats_per_boat;

        // Eisagwgh arithmou epibatwn
        printf("Enter number of passengers: ");
        scanf("%d", &passengers);
        printf("Enter number of boats: ");  // Eisagwgh arithmou lemvou
        scanf("%d", &boats);
        printf("Enter seats per boat: ");    //eisagwgh thesewn ana lemvo
        scanf("%d", &seats_per_boat);

        // Elegxos orion
        if (passengers > MAX_PASSENGERS || boats > MAX_BOATS || seats_per_boat <= 0) {
            printf("Invalid input! Please check the limits.\n");
            return EXIT_FAILURE;
        }

        int queue[passengers];
        int queue_size = passengers;

        // Arxikopoihsh ouras epibatwn
        for (int i = 0; i < passengers; i++) {
            queue[i] = i + 1;
        }

        // Arxikopoihsh semaphorewn
        init_semaphore(&boat_space, seats_per_boat);
        init_semaphore(&boarding_queue, 1);
        init_semaphore(&boats_available, boats);

        // Kyklos gia thn eksypiretisi twn epibatwn
     while (queue_size > 0) {
            int passenger_id = queue[0];

            // 20% pithanothta na metakinithei o epibaths sto telos ths ouras
            if ((rand() % 100) < 20) {
            printf("Passenger %d chooses not to board and moves to the back of the queue.\n", passenger_id);

            // an den epibastei kanoume shift thn oura
            for (int j = 0; j < queue_size - 1; j++) {
                queue[j] = queue[j + 1];
            }


            //kai paei sto telos
            queue[queue_size - 1] = passenger_id;
            } else {
            // Kalesma synarthshs gia thn diadikasia epibivashs
            passenger_routine(passenger_id, passengers, boats, seats_per_boat);

            //epeidh sthn arxh pairnoume ton prwto epibath (queue[0]) gia na ton pame pisw an xreiastei, an den allaksei gnwmh shiftaroume gia na pame ston epomeno (queue[1])
            for (int j = 0; j < queue_size - 1; j++) {
                queue[j] = queue[j + 1];
            }
            //paei epivivasthke
            queue_size--;
            }
    }

    //cleanup tous shmaforous
    destroy_semaphore(&boat_space);
    destroy_semaphore(&boarding_queue);
    destroy_semaphore(&boats_available);

        printf("All passengers have been rescued!\n");
        return EXIT_SUCCESS;
}