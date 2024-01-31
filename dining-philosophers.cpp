
#include <stdio.h>   
#include <pthread.h>  
#include <string> 
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <iostream>
#include <semaphore.h>
#include <chrono>
#include <algorithm>
#include <vector>
#include <time.h>
using namespace std;

// A simulation of the dining philosophers problem using monitors

pthread_barrier_t barrier;
sem_t sems[5];
double avgTimes[5];

//This is a monitor representing the waiter for our A3 Part 1
//This implementation is directly derived from operating systems concepts 10th edition 6.7.2: Implementing a Monitor Using Semaphores
struct WaiterMonitor
{
    //How many chopsticks are available - this should be user input between 5 and 10 (inclusive)
    int chopsticks_available;
	
    //Mutex-semaphore used to restrict threads entering a method in this monitor
    //Keep in mind many threads may be inside a method in this monitor, but at most ONE should be executing (the rest should be waiting)
    sem_t mutex_sem;
	
    //Mutex-semaphore and counter used to keep track of how many threads are waiting inside a method in this monitor (besides those waiting for a condition)
    sem_t next_sem;
    int next_count;

    //Semaphore and counter to keep track of threads waiting for the 'at least one chopstick available' condition
    //These should be the philosopher threads waiting on the right chopstick
    sem_t condition_can_get_1_sem;
    int condition_can_get_1_count;

    //Semaphore and counter to keep track of threads waiting for the 'at least two chopsticks available' condition
    //These should be the philosopher threads waiting on the left chopstick
    sem_t condition_can_get_2_sem;
    int condition_can_get_2_count;

    void init(int n)
    {
        //Ive hard-coded the number of chopsticks (You may not do this!)
        chopsticks_available = n;
        //Mutex to gain access to (any) method in this monitor is initialized to 1
        sem_init(&mutex_sem, 0, 1);
        
        //'Next' semaphore - the semaphore for threads waiting inside a method of this monitor - is initialized to 0 (meaning no threads are initially in a method, as is logical)
        sem_init(&next_sem, 0, 0);
        next_count = 0;

        //Condition semaphores are intialized to 0 (meaning no threads are initially waiting on these conditions, as is logical)
        sem_init(&condition_can_get_1_sem, 0, 0);
        condition_can_get_1_count = 0;
        sem_init(&condition_can_get_2_sem, 0, 0);
        condition_can_get_2_count = 0;
    }

    void destroy()
    {
        sem_destroy(&mutex_sem);
        sem_destroy(&condition_can_get_1_sem);
        sem_destroy(&condition_can_get_2_sem);
    }

    //This is the manual implementation of pthread_cond_wait() using semaphores
    void condition_wait(sem_t &condition_sem, int &condition_count)
    {
		//condition count is the number of threads waiting on the condition, increment it since the thread calling this method is about to wait
        condition_count++;
		//If there is a waiting thread INSIDE a method in this monitor, they get priority, so post to that semaphore
        if (next_count > 0)
            sem_post(&next_sem);
		//Otherwise, post to the general entry semaphore (the mutex, that is)
        else
            sem_post(&mutex_sem);
		//Wait for this condition to be posted to (Note that as soon as someone posts to this condition, they will halt as this thread has priority!)
        sem_wait(&condition_sem);
		//If I reach here, I have finished waiting :)
        condition_count--;
    }

    //This is the manual implementation of pthread_cond_signal() using semaphores
    void condition_post(sem_t &condition_sem, int &condition_count)
    {
		//If there are any threads waiting on the condition I want to post...
        if (condition_count > 0)
        {
			//...Then they have priority (they were waiting before me), I shall wait in the next_sem gang
            next_count++;
			//Post to the condition_sem gang so they can continue
            sem_post(&condition_sem);
			//Wait for someone to post to next_sem
            sem_wait(&next_sem);
            next_count--;
        }
    }

    void request_left_chopstick()
    {
        //A thread needs mutex access to enter any of this monitors' method!!!
        sem_wait(&mutex_sem);

        //Okay so we got mutex access...but what if there are less than 2 chopsticks available when I am requesting the left chopstick?...
        while(chopsticks_available < 2)
            //...Then wait for the 'at least two chopsticks available' semaphore per the waiter-implementation specifications!
            condition_wait(condition_can_get_2_sem, condition_can_get_2_count);

        //If we're here, then at least two chopsticks are available, use up one of them
        chopsticks_available--;

        if(chopsticks_available >= 1)
        {
            //If at least a chopstick remains, post to the 'at least one chopstick available' condition
            condition_post(condition_can_get_1_sem, condition_can_get_1_count);

             if(chopsticks_available >= 2)
             {
                //If at least two chopsticks remain, post to the 'at least two chopsticks available' condition
                condition_post(condition_can_get_2_sem, condition_can_get_2_count);
             }
        }

        //Threads waiting for next_sem are waiting INSIDE one of this monitor's methods...they get priority!
        if (next_count > 0)
            sem_post(&next_sem);
        //If no such threads exist... simply open up the general-access mutex!
        else
            sem_post(&mutex_sem);
    }

    void request_right_chopstick()
    {
        sem_wait(&mutex_sem);

        while (chopsticks_available < 1) {
            condition_wait(condition_can_get_1_sem, condition_can_get_1_count);
        }

        // Take a chopstick
        chopsticks_available--;

        if(chopsticks_available >= 1)
        {
            //If at least a chopstick remains, post to the 'at least one chopstick available' condition
            condition_post(condition_can_get_1_sem, condition_can_get_1_count);

             if(chopsticks_available >= 2)
             {
                //If at least two chopsticks remain, post to the 'at least two chopsticks available' condition
                condition_post(condition_can_get_2_sem, condition_can_get_2_count);
             }
        }

        // Let the next thread go
        if (next_count > 0)
            sem_post(&next_sem);
        else
            sem_post(&mutex_sem);
    }

    void return_chopsticks()
    {
        sem_wait(&mutex_sem);

        // Give back chopsticks
        chopsticks_available += 2;

        //We know there is at least one free chopstick now
        condition_post(condition_can_get_1_sem, condition_can_get_1_count);

        // If there are still at least two chopsticks available, post to condition_can_get_2
        if(chopsticks_available >= 2)
        {
            //If at least two chopsticks remain, post to the 'at least two chopsticks available' condition
            condition_post(condition_can_get_2_sem, condition_can_get_2_count);
        }

        // Let the next thread go
        if (next_count > 0)
            sem_post(&next_sem);
        else
            sem_post(&mutex_sem);
    }

};

struct WaiterMonitor waiter;
double timeArray[5];

//Function for the threads
void * thread_function(void * arg){

    int id = *((int*)arg);

    printf("created thread %d\n", id);
    srand(time(NULL) + id);

    double times[3];

    for(int i = 0; i < 3; i++)
    {
        printf("Philosopher %d is waiting on their semaphore...\n", id);
        sem_wait(&sems[id-1]);
        
        printf("Philosopher %d is hungry\n", id);

        // Start timing
        auto start = chrono::system_clock::now();

        waiter.request_left_chopstick();

        printf("Philosopher %d has picked up left chopstick\n", id);

        //Get the right chopstick
        waiter.request_right_chopstick();

        printf("Philosopher %d has picked up right chopstick\n", id);

        // End timing
        auto end = chrono::system_clock::now();

        printf("Philosopher %d is eating\n", id);

        chrono::duration<double> diff = end - start;
        times[i] = diff.count();
        printf("Philosopher %d waited %.5f seconds\n", id, diff.count());

        //Eat
        sleep(5);

        printf("Philosopher %d is done eating\n", id);

        //Return our chopsticks
        waiter.return_chopsticks();

        printf("Philosopher %d is thinking\n", id);
        sleep(2);

        // Wait for the rest of the threads to finish before eating again
        // to prevent starvation
        printf("P %d is waiting on the barrier...\n", id);
        pthread_barrier_wait(&barrier);
        printf("P %d has passed the barrier\n", id);
   
    }

    double avgTime;
    avgTime = times[0] + times[1] + times[2];
    avgTime = avgTime / 3;

    printf("Philosopher %d has finished eating 3 times, with an average waiting time of %.2f seconds\n", id, avgTime);

    avgTimes[id-1] = avgTime;

    pthread_exit(NULL);
}

int main(int argc, char *argv[]){

    srand(time(NULL));

    int n;
    printf("Enter n: ");
    scanf("%d", &n);
    printf("\n");

    while (n < 5 || n > 10) {
        printf("Enter n between 5 and 10: ");
        scanf("%d", &n);
        printf("\n");
    }
   
    waiter.init(n);

    // Initialize the barrier to break for 6 threads
    pthread_barrier_init(&barrier, NULL, 6);

    // Create an array of binary semaphores and initialize them to 0
    sem_init(&sems[0], 0, 0);
    sem_init(&sems[1], 0, 0);
    sem_init(&sems[2], 0, 0);
    sem_init(&sems[3], 0, 0);
    sem_init(&sems[4], 0, 0);

    pthread_t tids[5];
    int ids[5];

    // Create the five philosophers
    for (int i = 1; i < 6; i++) {
        ids[i] = i;
        pthread_create(&tids[i], NULL, thread_function, (void*) &ids[i]);
    }

    vector<int> selectedPhilosophers;
    int num;
    int count = 0;

    sleep(3);

    // Main loop
    for (int i = 0; i < 3; i++) {

        // Loop for selecting philosophers randomly
        while (count < 5){
            num = rand() % 5;
            // If philosopher is not yet selected, then select it
            if (find(selectedPhilosophers.begin(), selectedPhilosophers.end(), num) == selectedPhilosophers.end()) {
                printf("Main thread selected philosopher %d\n", num + 1);
                selectedPhilosophers.push_back(num);
                sem_post(&sems[num]);
                count++;
            }

        }

        selectedPhilosophers.clear();
        count = 0;
        pthread_barrier_wait(&barrier);
    }

    // Join the five philosopher threads and destroy the other variables
    for (int j = 1; j < 6; j++) {
        pthread_join(tids[j], NULL);
        sem_destroy(&sems[j]);
    }
    waiter.destroy();
    pthread_barrier_destroy(&barrier);

    double totalAvg;

    for (int k = 0; k < 5; k++) {
        totalAvg += avgTimes[k];
    }

    totalAvg = totalAvg / 5;

    printf("\nTotal average waiting time: %.5f\n\n", totalAvg);

    return 0;
}