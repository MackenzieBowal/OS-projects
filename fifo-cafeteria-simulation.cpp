
/* Assignment 4 - CPSC 457
Mackenzie Bowal - 30096631
FIFO Program
I used Alex's method of solving the consumer-stuck-in-monitor problem
*/

#include <iostream>
#include <pthread.h>  
#include <string> 
#include <unistd.h>   
#include <semaphore.h>
#include <deque>
#include <chrono>
#include <fstream>
#include <vector>
#include <string>
#include <mutex>
#include <math.h>

using namespace std;
// Number of consumers
#define N 4

// Global variables for producer to use
vector<int> studentArrivalTimes;
vector<int> studentEatingTimes;
// Not used
int quantum;

// Changed to true by the producer thread when it's finished adding customers
bool finishedAddingToQueue = false;
// Set when it's finished reading the file
int numTotalCustomers = 0;
// Incremented by each consumer thread when they finish serving a customer
int numCustomersFinished = 0;
std::mutex numLock; // To avoid race conditions when incrementing the above

auto startTime = chrono::system_clock::now();

struct Customer
{
    int id;
    int eating_time_left;
    int arrival_time;
    bool fake_customer;
};

struct QueueMonitor
{
    //Students in the queue
    std::deque<Customer*> customers;

    //Mutex-semaphore used to restrict threads entering a method in this monitor
    //Keep in mind many threads may be inside a method in this monitor, but at most ONE should be executing (the rest should be waiting)
    sem_t mutex_sem;
    //Mutex-semaphore and counter used to keep track of how many threads are waiting inside a method in this monitor
    sem_t next_sem;
    int next_count;

    //Semaphore and counter to keep track of threads waiting for the 'at least one customer available' condition
        //These should be the table threads waiting on a customer
    sem_t condition_nonempty_sem;
    int condition_nonempty_count;

    void init()
    {
        customers = std::deque<Customer*>();

        //Mutex to gain access to (any) method in this monitor is initialized to 1
        sem_init(&mutex_sem, 0, 1);
        //'Next' semaphore - the semaphore for threads waiting inside a method of this monitor - is initialized to 0 (meaning no threads are initially in a method, as expected)
        sem_init(&next_sem, 0, 0);
        next_count = 0;

        //Condition semaphores are intialized to 0 (meaning no threads are initially waiting on this condition, as is expected)
        sem_init(&condition_nonempty_sem, 0, 0);
        condition_nonempty_count = 0;
    }

    void destroy()
    {
        sem_destroy(&mutex_sem);
        sem_destroy(&condition_nonempty_sem);
    }

    //This is the manual implementation of pthread_cond_wait() using semaphores
    void condition_wait(sem_t &condition_sem, int &condition_count)
    {
        condition_count++;
        if (next_count > 0)
            sem_post(&next_sem);
        else
            sem_post(&mutex_sem);
        sem_wait(&condition_sem);
        condition_count--;
    }

    //This is the manual implementation of pthread_cond_signal() using semaphores
    void condition_post(sem_t &condition_sem, int &condition_count)
    {
        if (condition_count > 0)
        {
            next_count++;
            sem_post(&condition_sem);
            sem_wait(&next_sem);
            next_count--;
        }
    }

    Customer *get_customer()
    {
        //A thread needs mutex access to enter any of this monitors' method!!!
        sem_wait(&mutex_sem);

        //Okay so we got mutex access...but what if the queue is empty?
        while(customers.size() < 1)
            //...Then wait for the 'at least one chopsticks available' semaphore per the waiter-implementation specifications!
            condition_wait(condition_nonempty_sem, condition_nonempty_count);

        //If we're here, then at least one customer is in the queue. Get it.
        Customer *c = customers.front();
        //Now remove it from the queue (this only removes it from the queue - the customer data, currently pointed at by c, will still be there)
        customers.pop_front();
        Customer customer = *c;
        if (!customer.fake_customer) {
            printf("Sit %d\n", customer.id);
        }

        //If at this point there is at least one customer...
        if(customers.size() >= 1)
        {
            //...post to the 'at least one customer' condition
            condition_post(condition_nonempty_sem, condition_nonempty_count);
        }

        //Threads waiting for next_sem are waiting INSIDE one of this monitor's methods...they get priority!
        if (next_count > 0)
            sem_post(&next_sem);
        //If no such threads exist... simply open up the general-access mutex!
        else
            sem_post(&mutex_sem);

        return c;
    }

    void add_customer(Customer* c)
    {
        //A thread needs mutex access to enter any of this monitors' method!!!
        sem_wait(&mutex_sem);

        //Add the customerm to the queue
        customers.push_back(c);
        Customer customer = *c;

        if (!customer.fake_customer) {
            printf("Arrive %d\n", customer.id);
        }

        //Post to the nonempty queue condition
        condition_post(condition_nonempty_sem, condition_nonempty_count);

        //Threads waiting for next_sem are waiting INSIDE one of this monitor's methods...they get priority!
        if (next_count > 0)
            sem_post(&next_sem);
        //If no such threads exist... simply open up the general-access mutex!
        else
            sem_post(&mutex_sem);
    }

    void print_leave(int cID, int turnaroundTime, int waitingTime)
    {
        //A thread needs mutex access to enter any of this monitors' method!!!
        sem_wait(&mutex_sem);

        printf("Leave %d Turnaround %d Wait %d\n", cID, turnaroundTime, waitingTime);

        //Threads waiting for next_sem are waiting INSIDE one of this monitor's methods...they get priority!
        if (next_count > 0)
            sem_post(&next_sem);
        //If no such threads exist... simply open up the general-access mutex!
        else
            sem_post(&mutex_sem);
    }

};

struct QueueMonitor queue;

/*Function for the producer thread. There are 1 of these. It is in charge of adding customers to the end of the queue as they arrive.

*/
void *producer_function(void * arg){

    // Create an empty array of Customers
    Customer * customers = (Customer *) malloc(numTotalCustomers * sizeof(Customer));

    // Loop adding students to queue
    for (int m = 0; m < numTotalCustomers; m++) {

        // Wait for next customer to arrive
        if (studentArrivalTimes.at(m) != 0)
            sleep(studentArrivalTimes.at(m));

        // Calculate the current time
        auto now = chrono::system_clock::now();
        chrono::duration<double> diff = now - startTime;
        int currentTime = (int)round(diff.count());

        // Create customer
        customers[m].id = m;
        customers[m].eating_time_left = studentEatingTimes[m];
        customers[m].arrival_time = currentTime;
        customers[m].fake_customer = false;

        // Add customer to queue
        queue.add_customer(&customers[m]);
    }

    finishedAddingToQueue = true;
    // Wait till consumer threads have finished serving customers
    while (numCustomersFinished < numTotalCustomers);

    pthread_exit(0);
}


/* Function for the consumer/table thread. There are 4 of these. It is in charge of taking customers from the queue when the table is free

*/
void *consumer_function(void *arg){
    int myid = *(int *) arg;

    int turnaroundTime;
    int waitingTime;
    int finishTime;
    int currentTime;

    // Loop getting customers until they're all finished eating
    while (1) {

        Customer *cPtr = queue.get_customer();
        Customer cust = *cPtr;

        if (cust.fake_customer) {
            break;
        }

        sleep(cust.eating_time_left);

        // Find the current time
        auto now = chrono::system_clock::now();
        chrono::duration<double> diff = now - startTime;
        int currentTime = (int)round(diff.count());

        // Calculate TAT and WT
        finishTime = currentTime;
        turnaroundTime = finishTime - cust.arrival_time;
        waitingTime = turnaroundTime - cust.eating_time_left;

        // Update struct
        cust.eating_time_left = 0;

        // Print to output
        queue.print_leave(cust.id, turnaroundTime, waitingTime);

        // Increment how many customers have finished
        numLock.lock();
        numCustomersFinished++;
        numLock.unlock();
    }

    pthread_exit(0);
}

/*Function for the exit thread. 
*/
void *exit_function(void *arg){
    
    // Wait till producer is done, also handles preemption priority
    while (!finishedAddingToQueue);

    // Wait till consumer threads are done
    while (numCustomersFinished < numTotalCustomers);

    // Create N "fake" consumers and add them to the queue
    for (int p = 0; p < N; p++) {
        Customer fakeCustomer;
        fakeCustomer.id = 0;
        fakeCustomer.eating_time_left = 0;
        fakeCustomer.arrival_time = 0;
        fakeCustomer.fake_customer = true;
        queue.add_customer(&fakeCustomer);
    }

    pthread_exit(0);
}

/* Reads the file, stores the information in vectors for the producer to use, and creates the threads.
*/

int main()
{

    // File operations
    cout << "Enter file name: ";
    string fName;
    getline(cin, fName);
    fstream file;
    file.open(fName, fstream::in | fstream::out);
    int arrivalTime;
    int eatingTime;
    int i = 1;

    file >> quantum;
    string line;
    getline(file, line);
    int space;
    string first;
    string second;

    // Read from the file, storing info into the vectors
    while (!file.eof()) {
        getline(file, line);
        if (line.length() == 0) {
            break;
        }
        space = line.find(" ");
        first = line.substr(0, space);
        second = line.substr(space + 1);
        arrivalTime = stoi(first);
        eatingTime = stoi(second);

        studentEatingTimes.push_back(eatingTime);
        studentArrivalTimes.push_back(arrivalTime);
        i++;
    }

    file.close();

    numTotalCustomers = studentArrivalTimes.size();

    // Execute FIFO algorithm

    queue.init();

    // Create producer thread
    pthread_t producer_id;

    int result = pthread_create(&producer_id, NULL, producer_function, NULL);
    if (result != 0) {
        printf("Error creating producer thread\n");
    }

    // Create consumer threads
    pthread_t tids[N];
    pthread_attr_t attrs[N];
    
    int ids[4];
    for (int k = 0; k < N; k++) {
        ids[k] = k;
        pthread_attr_init(&attrs[k]);
        result = pthread_create(&tids[k], &attrs[k], consumer_function, &ids[k]);
        if (result != 0) {
            printf("Error creating consumer %d\n", ids[k]);
        }
    }

    // Create exit_thread
    pthread_t exit_id;
    result = pthread_create(&exit_id, NULL, exit_function, NULL);
    if (result != 0) {
        printf("Error creating exit thread\n");
    }

    // Join back with threads

    pthread_join(exit_id, NULL);

    pthread_join(producer_id, NULL);

    for (int j = 0; j < N; j++) {
        pthread_join(tids[j], NULL);
    }

    queue.destroy();

	return 1;
}