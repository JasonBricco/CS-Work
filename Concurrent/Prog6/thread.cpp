// -----------------------------------------------------------
// NAME : Jason Bricco                       User ID: jmbricco
// DUE DATE : 4/27/2020                                      
// PROGRAM ASSIGNMENT #6                                   
// FILE NAME : thread.cpp                                   
// PROGRAM PURPOSE :                                          
// Defines thread functionality and channel usage.
// -----------------------------------------------------------

#include "thread.h"

#define ARR_SIZE 128
static int primes[ARR_SIZE] = {};

PrimeThread::PrimeThread(int id, int slot) : SieveThread(id), slot(slot)
{
	UserDefinedThreadID = id;
	primes[slot] = id;
}

// -----------------------------------------------------------
// FUNCTION  MasterThread::ThreadFunc                              
//    Master thread's functionality. The master thread sends
//	  integers through its channel to the first prime thread,
//	  and prints the final prime numbers at the end.
// FUNCTION CALLED :                                          
//    Thread::ThreadFunc, PRINT (macro), SynOneToOneChannel::Send,
//	  Thread::Join                          
// -----------------------------------------------------------
void MasterThread::ThreadFunc()
{
	Thread::ThreadFunc();

	PRINT("Master starts\n");

	int n = 3;

	// Send integers incrementally through the channel
	// connected to P2 until we reach the limit given as program input.
	while (n <= max)
	{
		PRINT("Master sends %d to P2\n", n);

		channel->Send(&n, sizeof(int));
		++n;
	}

	// Finished sending input, so send END.
	PRINT("Master sends END\n");

	n = END;
	channel->Send(&n, sizeof(int));

	// Effectively waits for all other threads to finih.
	next->Join();

	PRINT("Master prints the complete result:\n  ");

	// Print accumulated prime numbers.
	for (int i = 0; i < ARR_SIZE; ++i)
	{
		if (primes[i] == 0)
			break;

		PRINT("%d ", primes[i]);
	}

	PRINT("\nMaster terminates\n");
}

// -----------------------------------------------------------
// FUNCTION  PrimeThread::ThreadFunc                              
//    Prime threads receive numbers, determine if they're possibly
//    primes or not, and create a new thread, pass the number along,
//    or ignore it depending.
// FUNCTION CALLED :                                          
//    Thread::ThreadFunc, PRINT (macro), SynOneToOneChannel::Send,
//	  Thread::Join, SynOneToOneChannel::Receive, Thread::Begin                     
// -----------------------------------------------------------
void PrimeThread::ThreadFunc()
{
	Thread::ThreadFunc();

	// Number of spaces to include in print statements.
	int spaces = 2 + (slot * 2);

	PRINT("%*cP%d starts and memorizes %d\n", spaces, ' ', id, id);

	while (true)
	{
		// Receive the number from the previous thread (or master thread),
		// blocking until it is available.
		int number;
		prev->channel->Receive(&number, sizeof(int));

		if (number != END)
			PRINT("%*cP%d receives %d\n", spaces, ' ', id, number)
		else PRINT("%*cP%d receives END\n", spaces, ' ', id);

		if (number == END)
		{
			// On END, exit the loop. If this is not the last thread,
			// pass END along to the next threads.
			if (channel != NULL)
			{
				channel->Send(&number, sizeof(int));
				next->Join();
			}

			break;
		}
		else
		{
			if (number % id != 0)
			{
				// Number is potentially a prime, consider it.

				if (channel == NULL)
				{
					// If this is the last thread in line, then the number
					// is a prime. Create a new thread associated with it.
					PRINT("%*cP%d creates P%d\n", spaces, ' ', id, number);

					next = new PrimeThread(number, slot + 1);
					((PrimeThread*)next)->prev = this;

					char name[32];
					sprintf(name, "Thread%d", number);
					channel = new SynOneToOneChannel(name, id, number);

					next->Begin();
				}
				else 
				{
					// Pass the number to the next thread - we can't be sure it is a prime
					// until all threads have checked it.
					PRINT("%*cP%d sends %d to P%d\n", spaces, ' ', id, number, next->id);
					channel->Send(&number, sizeof(int));
				}
			}
			else 
			{
				// Number is not a prime, ignore it.
				PRINT("%*cP%d ignores %d\n", spaces, ' ', id, number);
			}
		}
	}
}
