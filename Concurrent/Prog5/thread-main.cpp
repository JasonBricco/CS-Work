// -----------------------------------------------------------
// NAME : Jason Bricco                       User ID: jmbricco
// DUE DATE : 4/17/2020                                      
// PROGRAM ASSIGNMENT #5                                      
// FILE NAME : thread-main.cpp                                     
// PROGRAM PURPOSE :                                          
// Defines the program entry, reads input, and begins threads.
// -----------------------------------------------------------

#include "thread.h"

// -----------------------------------------------------------
// FUNCTION  main                               
//    takes input from the command line and begins threads.
// FUNCTION CALLED :                                          
//    PRINT (macro), atoi, Thread::Begin, Thread::Join                         
// -----------------------------------------------------------
int main(int argc, char** argv)
{
	if (argc != 4)
	{
		PRINT("Invalid number of arguments!\n");
		return -1;
	}

	// Read input arguments from the command line.
	int elfCount = atoi(argv[1]);

	if (elfCount == 0)
		elfCount = 7;

	int reindeerCount = atoi(argv[2]);

	if (reindeerCount == 0)
		reindeerCount = 9;

	int deliverCount = atoi(argv[3]);

	if (deliverCount == 0)
		deliverCount = 5;

	// Create the monitor and threads.
	Simulation sim;
	sim.maxReindeer = reindeerCount;
	sim.maxElves = elfCount;

	Elf** elves = new Elf*[elfCount];
	Reindeer** reindeer = new Reindeer*[reindeerCount];

	Santa santa;
	santa.sim = &sim;
	santa.deliverCount = 0;
	santa.maxDeliver = deliverCount;

	// Begin the threads.
	santa.Begin();

	for (int i = 0; i < reindeerCount; ++i)
	{
		reindeer[i] = new Reindeer(&sim, i);
		reindeer[i]->Begin();
	}

	for (int i = 0; i < elfCount; ++i)
	{
		elves[i] = new Elf(&sim, i);
		elves[i]->Begin();
	}

	// Wait for threads to finish before exiting.
	for (int i = 0; i < elfCount; ++i)
		elves[i]->Join();

	for (int i = 0; i < reindeerCount; ++i)
		reindeer[i]->Join();

	santa.Join();
}
