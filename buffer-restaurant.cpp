#include <stdlib.h> 
#include <sys/mman.h> 
#include <stdio.h> 
#include <unistd.h> 
#include <semaphore.h> 
#include <time.h> 
#include <pthread.h>

// This program is an exercise on semaphores and buffers, with a restaurant-themed twist. There are
// two chefs and three types of customers: vegan and non-vegan, and additionally hybrid for customers. 


#define BUFFER_SIZE 10

int * NVtray;
int * Vtray;

int NVin;
int NVout;
int Vin;
int Vout;

sem_t NVmutex;
sem_t NVfull;
sem_t NVempty;

sem_t Vmutex;
sem_t Vfull;
sem_t Vempty;

void *donatelloFunction(void* arg);
void *NVconsumerFunction(void* arg);
void *portecelliFunction(void* arg);
void *VconsumerFunction(void* arg);
void *hybridConsumerFunction(void * param);

/* Main()
Creates and initalizes the two bounded buffers, then initializes the semaphores for each and creates 
the five producer and consumer threads. Afterwards, it counts and prints the number of items in each
tray every 10 seconds.
*/

int main() {

    srand(time(NULL));

    printf("\n\n\033[0mNon-vegan items are represented as \033[31mred\033[0m\nVegan items are represented as \033[32mgreen\033[0m\n\n");

    sleep(3);

    // Initialize the bounded buffer trays
    NVtray = (int*)malloc(BUFFER_SIZE*sizeof(int));
    Vtray = (int*)malloc(BUFFER_SIZE*sizeof(int));

    for (int i = 0; i < BUFFER_SIZE; i++) {
        NVtray[i] = 0;
        Vtray[i] = 0;
    }

    // Initialize semaphores
    sem_init(&NVmutex, 0, 1);
    sem_init(&NVempty, 0, BUFFER_SIZE);
    sem_init(&NVfull, 0, 0);

    sem_init(&Vmutex, 0, 1);
    sem_init(&Vempty, 0, BUFFER_SIZE);
    sem_init(&Vfull, 0, 0);

    pthread_t donatello;
    pthread_t NVconsumer;
    pthread_t portecelli;
    pthread_t Vconsumer;
    pthread_t hybridConsumer;

    // create Donatello
    if (pthread_create(&donatello, NULL, donatelloFunction, NULL) != 0) {
        printf("Error creating Donatello\n");
    }

    // create non-vegan consumer
    if (pthread_create(&NVconsumer, NULL, NVconsumerFunction, NULL) != 0) {
        printf("Error creating Non-vegan consumer\n");
    }

    // create Portecelli
    if (pthread_create(&donatello, NULL, portecelliFunction, NULL) != 0) {
        printf("Error creating Donatello\n");
    }

    // create vegan consumer
    if (pthread_create(&NVconsumer, NULL, VconsumerFunction, NULL) != 0) {
        printf("Error creating vegan consumer\n");
    }

    // create hybrid consumer
    if (pthread_create(&hybridConsumer, NULL, hybridConsumerFunction, NULL) != 0) {
        printf("Error creating hybrid consumer\n");
    }

    // Count and print the number of items in each tray, every 10 seconds
    int NVitems = 0;
    int Vitems = 0;

    while (true) {
        for (int i = 0; i < BUFFER_SIZE; i++) {
            if (NVtray[i] != 0) NVitems++;
            if (Vtray[i] != 0) Vitems++;
        }
        printf("\n\n\033[31mItems in non-vegan tray: %d/%d\033[0m\n", NVitems, BUFFER_SIZE);
        printf("\033[32mItems in vegan tray: %d/%d\033[0m\n\n", Vitems, BUFFER_SIZE);
        NVitems = 0;
        Vitems = 0;
        sleep(10);
    }    

    pthread_join(donatello, NULL);
    pthread_join(NVconsumer, NULL);
    pthread_join(portecelli, NULL);
    pthread_join(Vconsumer, NULL);
    pthread_join(hybridConsumer, NULL);

    free(NVtray);
    free(Vtray);
    return 0;
}

/* donatelloFunction()
Function to handle the non-vegan producer Donatello. Enters a while loop waiting for the two semaphores,
then generates a random dish and adds it to the tray. Posts/signals the appropriate semaphores and then 
sleeps for 1-5 seconds before looping again.
*/

void * donatelloFunction(void * param) {

    int dishAdded = 0;
    sleep(1);

    // While loop to add items to non-vegan tray
    while (true) {

        // Wait for appropriate semaphores
        sem_wait(&NVempty);
        sem_wait(&NVmutex);
        
        // Produce a random number, either 1 or 2, for the dish
        dishAdded = rand() % 2 + 1;
        NVtray[NVin] = dishAdded;
        NVin = (NVin+1)%BUFFER_SIZE;

        // Print result to the console
        switch(dishAdded) {
            case 1:
                printf("Donatello creates non-vegan dish: \033[31mFettuccine Chicken Alfredo\033[0m\n");
                break;
            case 2:
                printf("Donatello creates non-vegan dish: \033[31mGarlic Sirloin Steak\033[0m\n");
                break;
        }
        
        // Post the appropriate semaphores
        sem_post(&NVfull);
        sem_post(&NVmutex);

        //Sleep between 1 and 5 seconds
        sleep(1 + rand() % 5);
    }

    pthread_exit(0);
}

/* NVconsumerFunction()
Function to handle the non-vegan consumers. Enters a while loop waiting for the two semaphores,
then takes the next dish from the non-vegan tray. Posts/signals the appropriate semaphores and then 
sleeps for 10-15 seconds before looping again.
*/

void * NVconsumerFunction(void * param) {

    int dishRemoved = 0;

    while (true) {

        // Wait for semaphores
        sem_wait(&NVfull);
        sem_wait(&NVmutex);
        
        // Remove a dish from the non-vegan tray
        int dishRemoved = NVtray[NVout];
        NVtray[NVout] = 0;
        NVout = (NVout+1)%BUFFER_SIZE;

        // Print the result to the console
        switch(dishRemoved) {
            case 1:
                printf("Non-vegan customer removes non-vegan dish: \033[31mFettuccine Chicken Alfredo\033[0m\n");
                break;
            case 2:
                printf("Non-vegan customer removes non-vegan dish: \033[31mGarlic Sirloin Steak\033[0m\n");
                break;
        }

        // Post the semaphores
        sem_post(&NVmutex);
        sem_post(&NVempty);

        // Sleep between 10 and 15 seconds
        sleep(10 + rand() % 6);
    }

    pthread_exit(0);
}

/* portecelliFunction()
Function to handle the vegan producer Portecelli. Enters a while loop waiting for the two semaphores,
then generates a random dish and adds it to the tray. Posts/signals the appropriate semaphores and then 
sleeps for 1-5 seconds before looping again.
*/

void * portecelliFunction(void * param) {

    int dishAdded = 0;
    sleep(1);

    while (true) {

        // Wait for the semaphores
        sem_wait(&Vempty);
        sem_wait(&Vmutex);
        
        // Produce a random number, either 1 or 2, for the dish
        dishAdded = rand() % 2 + 1;
        Vtray[Vin] = dishAdded;
        Vin = (Vin+1)%BUFFER_SIZE;

        // Print the result to the console
        switch(dishAdded) {
            case 1:
                printf("Portecelli creates vegan dish: \033[32mPistachio Pesto Pasta\033[0m\n");
                break;
            case 2:
                printf("Portecelli creates vegan dish: \033[32mAvocado Fruit Salad\033[0m\n");
                break;
        }
        
        // Post the semaphores
        sem_post(&Vfull);
        sem_post(&Vmutex);

        //Sleep between 1 and 5 seconds
        sleep(1 + rand() % 5);
    }

    pthread_exit(0);
}

/* VconsumerFunction()
Function to handle the vegan consumers. Enters a while loop waiting for the two semaphores,
then removes the next dish from the vegan tray. Posts/signals the appropriate semaphores and then 
sleeps for 10-15 seconds before looping again.
*/

void * VconsumerFunction(void * param) {

    int dishRemoved = 0;

    while (true) {

        // Wait for the semaphores
        sem_wait(&Vfull);
        sem_wait(&Vmutex);
        
        // Remove a dish from the vegan tray
        int dishRemoved = Vtray[Vout];
        Vtray[Vout] = 0;
        Vout = (Vout+1)%BUFFER_SIZE;

        // Print the result to the console
        switch(dishRemoved) {
            case 1:
                printf("Vegan customer removes vegan dish: \033[32mPistachio Pesto Pasta\033[0m\n");
                break;
            case 2:
                printf("Vegan customer removes vegan dish: \033[32mAvocado Fruit Salad\033[0m\n");
                break;
        }

        // Post the semaphores
        sem_post(&Vmutex);
        sem_post(&Vempty);

        // Sleep between 10 and 15 seconds
        sleep(10 + rand() % 6);
    }

    pthread_exit(0);
}

/* hybridConsumerFunction()
Function to handle the hybrid consumers. Enters a while loop waiting for all four semaphores,
then removes the next dish from the non-vegan tray and the vegan tray. Posts/signals the 
appropriate semaphores and then sleeps for 10-15 seconds before looping again.
*/

void * hybridConsumerFunction(void * param) {

    int NVdishRemoved = 0;
    int VdishRemoved = 0;

    while (true) {

        // Wait for all four semaphores
        sem_wait(&NVfull);
        sem_wait(&NVmutex);
        sem_wait(&Vfull);
        sem_wait(&Vmutex);
        
        // Remove a dish from the non-vegan tray
        int NVdishRemoved = NVtray[NVout];
        NVtray[NVout] = 0;
        NVout = (NVout+1)%BUFFER_SIZE;

        switch(NVdishRemoved) {
            case 1:
                printf("Hybrid customer removes non-vegan dish: \033[31mFettuccine Chicken Alfredo\033[0m");
                break;
            case 2:
                printf("Hybrid customer removes non-vegan dish: \033[31mGarlic Sirloin Steak\033[0m");
                break;
        }

        // Remove a dish from the vegan tray
        int VdishRemoved = Vtray[Vout];
        Vtray[Vout] = 0;
        Vout = (Vout+1)%BUFFER_SIZE;

        switch(VdishRemoved) {
            case 1:
                printf(", and vegan dish: \033[32mPistachio Pesto Pasta\033[0m\n");
                break;
            case 2:
                printf(", and vegan dish: \033[32mAvocado Fruit Salad\033[0m\n");
                break;
        }

        // Post all four semaphores
        sem_post(&NVmutex);
        sem_post(&NVempty);
        sem_post(&Vmutex);
        sem_post(&Vempty);

        // Sleep between 10 and 15 seconds
        sleep(10 + rand() % 6);
    }

    pthread_exit(0);
}