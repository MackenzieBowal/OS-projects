//cite: https://www.geeksforgeeks.org/composite-number/
// A optimized school method based C++ program to check
// if a number is composite.
// #include <bits/stdc++.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <math.h>
#include <thread>
#include <time.h>
#include <mutex>
#include <iostream>
#include <vector>

#define ARR_SIZE 1000000

using namespace std;

vector<int> numbers;
int n;
int arraySize;
bool * results;

// Code given
bool isComposite(int n)
{
	// Corner cases
	if (n <= 1) return false;
	if (n <= 3) return false;

	// This is checked so that we can skip
	// middle five numbers in below loop
	if (n%2 == 0 || n%3 == 0) return true;

	for (int i=5; i*i<=n; i=i+6)
		if (n%i == 0 || n%(i+2) == 0)
		return true;

	return false;
}

/* Generates a random array
 of size ARR_SIZE, where every element
 is a 9-digit int.
*/
void generateArray() {

    int randomNum;
	srand(time(NULL));

	printf("Generating random array...\n");

	for (int i = 0; i < ARR_SIZE; i++) {
		// random 9-digit number
		randomNum = abs(100000000 + rand() % (999999999 - 100000000 + 1));
		numbers.push_back(randomNum);
	}

	printf("Size of array: %d\n", ARR_SIZE);

	return;
}

/*
Function executed by the computation threads. They loop through the array and
determine whether the numbers they are assigned are composite or not. The results
are stored in a global array.
*/
void * threadFunction(void * param) {
	int id = *(int *) (param);

	int index;
	int counter = 0;

	while (true) {
		// Indexes "leapfrog" over other threads
		index = n * counter + id;
		if (index >= numbers.size())
			break;
		// Determine composite or not and store in global array
		isComposite(numbers.at(index))? results[index] = true : results[index] = false;
		counter++;
	}

	pthread_exit(0);
}

/*
Main thread that creates the other threads, then calculates the proportions prime/composite
based on the thread results.
*/
int main(int argc, char * argv[])
{
	generateArray();

	// Get n from command line
	if (argc != 2) {
		printf("Enter n through command line\n");
		exit(0);
	}

	n = abs(stoi(argv[1]));

	arraySize = numbers.size();
	results = (bool *) malloc(arraySize);

	// Create n threads
	int threadIDs[n];
	pthread_t tid[n];
    pthread_attr_t attrs[n];

	for (int i = 0; i < n; i++) {
		threadIDs[i] = i;
		pthread_attr_init(&attrs[i]);
        pthread_create(&tid[i], &attrs[i], threadFunction, &threadIDs[i]);
	}

	for (int i = 0; i < n; i++) {
		pthread_join(tid[i], NULL);
	}

	int numPrime = 0;
	int numComp = 0;
	// print the results
	for (int i = 0; i < numbers.size(); i++) {
		//results[i] == true? cout << numbers.at(i) << " is a composite number" << endl : cout << numbers.at(i) << " is a prime number" << endl;
		if (results[i] == true)	{
			numComp++;
		}
		else {
			numPrime++;
		}
	}
	// Print proportions
	int total = numPrime + numComp;
	double propPrime = ((double) numPrime * 100) / ((double) total);
	double propComp = ((double) numComp * 100) / ((double) total);

	printf("\nProportion prime: %.2f%\nProportion composite: %.2f%\n\n", propPrime, propComp);

	return 0;
}
