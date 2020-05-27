// -----------------------------------------------------------
// NAME : Jason Bricco                       User ID: jmbricco
// DUE DATE : 4/27/2020                                      
// PROGRAM ASSIGNMENT #6                                   
// FILE NAME : thread.h                                     
// PROGRAM PURPOSE :                                          
// Defines thread classes.
// -----------------------------------------------------------

#include <stdio.h>
#include <string.h>

#include "ThreadClass.h"

// Helper printing macro for convenience.
#define PRINT(...) \
{ \
    char buf[256]; \
    sprintf(buf, __VA_ARGS__); \
    write(1, buf, strlen(buf)); \
}

#define END -1

struct SieveThread : public Thread
{
	// Prime number the thread "memorizes".
	int id;

	// Next thread in line.
	SieveThread* next;

	// Channel to communicate between this thread and the next thread.
	SynOneToOneChannel* channel;

	SieveThread(int id) : id(id), channel(NULL) {}
};

struct PrimeThread : public SieveThread
{
	SieveThread* prev;

	// Slot in the global primes array this
	// thread writes to.
	int slot;
	
	PrimeThread(int id, int slot);

	void ThreadFunc();
};

struct MasterThread : public SieveThread
{
	// Max number to send through the sieve.
	int max;

	MasterThread(int max) : SieveThread(0), max(max)
	{
		UserDefinedThreadID = 0;
		channel = new SynOneToOneChannel("Master", 0, 2);
	}

	void ThreadFunc();
};
