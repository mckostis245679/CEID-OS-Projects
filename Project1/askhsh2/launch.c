/* Mavridis Konstantinos, 1100620 */
/* Mitsainas Mixail-Xaralampos, 1100628 */
/* Loukanaris Konstantinos, 1100610 */
#include "ipc_utils.h"

//allagh an theloume na valoyme alla oria varkwn kai passengers
    #define MAX_PASSENGERS 200
    #define MAX_BOATS 50

    sem_t boat_space;
    sem_t boarding_queue;
    sem_t boats_available;

    int main() {

        srand(time(NULL));

        int passengers, boats, seats_per_boat;

        
        printf("Enter number of passengers(max: %d): ", MAX_PASSENGERS);
        scanf("%d", &passengers);
        printf("Enter number of boats(max: %d): ", MAX_BOATS); 
        scanf("%d", &boats);
        printf("Enter seats per boat: "); 
        scanf("%d", &seats_per_boat);

       

        if (passengers > MAX_PASSENGERS || boats > MAX_BOATS || seats_per_boat <= 0) {
            printf("Invalid input! Please check the limits.\n");
            return 1;
        }

        int queue[passengers];
        int queue_size = passengers;

        // Arxikopoihsh ouras epibatwn
        for (int i = 0; i < passengers; i++) {
            queue[i] = i + 1;
        }

        // Arxikopoihsh semaphorwn
        init_semaphore(&boat_space, seats_per_boat);
        init_semaphore(&boarding_queue, 1);
        init_semaphore(&boats_available, boats);

        
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

            //kathe fora pairnw to passenger_id ws queue[0] ara to front, kai afou phra to front shiftarw gia na paei o epomenos
            for (int j = 0; j < queue_size - 1; j++) {
                queue[j] = queue[j + 1];
            }

       
            queue_size--;
            }
    }

    
    destroy_semaphore(&boat_space);
    destroy_semaphore(&boarding_queue);
    destroy_semaphore(&boats_available);

        printf("All passengers have been rescued!\n");
        return 0;
}