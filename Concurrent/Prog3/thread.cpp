// -----------------------------------------------------------
// NAME : Jason bricco                       User ID: jmbricco
// DUE DATE : 03/18/2020                                      
// PROGRAM ASSIGNMENT #3                                      
// FILE NAME : thread.cpp                                     
// PROGRAM PURPOSE :                                          
// Contains code that will be run by the background threads.  
// -----------------------------------------------------------

#include <cstdio>
#include <string.h>

#include "thread.h"

// -----------------------------------------------------------
// FUNCTION  SwapThread::RunTask                                 
//     assigns values to the thread class and begins its
//     execution.
// PARAMETER USAGE :                                          
//    id: the id of the thread.
//    x: the array to sort.
//    slot: the index in the array this thread will start at
//    for swapping.
//    swapped: set to true if a swap occurs.      
// FUNCTION CALLED :                                          
//    sprintf, write, strlen (from PRINT), Begin                             
// -----------------------------------------------------------
void SwapThread::RunTask(int id, int* x, int slot, bool* swapped)
{
	this->id = id;
	this->x = x;
	this->slot = slot;
	this->swapped = swapped;

	PRINT("        Thread %d Created\n", id);

	// Actually begin the thread via ThreadMentor.
	Begin();
}

// -----------------------------------------------------------
// FUNCTION  SwapThread::ThreadFunc                                 
//     executed on a thread. Performs swapping between the
//	   value at slot and slot - 1 if necessary.    
// FUNCTION CALLED :                                          
//    sprintf, write, strlen (from PRINT), Thread::ThreadFunc, 
//    std::swap                             
// -----------------------------------------------------------
void SwapThread::ThreadFunc()
{
	Thread::ThreadFunc();

	PRINT("        Thread %d compares x[%d] and x[%d]\n", id, slot - 1, slot);

	// If the item to the left is larger, swap it to fix its order.
	if (x[slot - 1] > x[slot])
	{
		std::swap(x[slot - 1], x[slot]);
		PRINT("        Thread %d swaps x[%d] and x[%d]\n", id, slot - 1, slot);
		*swapped = true;
	}

	PRINT("        Thread %d exits\n", id);
}
