// -----------------------------------------------------------
// NAME : Jason Bricco                       User ID: jmbricco
// DUE DATE : 4/27/2020                                      
// PROGRAM ASSIGNMENT #6                                   
// FILE NAME : thread-main.cpp                                   
// PROGRAM PURPOSE :                                          
// Defines the main function that receives input.
// -----------------------------------------------------------

#include "thread.h"

// -----------------------------------------------------------
// FUNCTION  main                                 
//    receives input arguments and begins the threads for
//	  execution.
// PARAMETER USAGE :                                          
//    argc: the number of arguments passed to the program.
//	  argv: the arguments psased to the program.    
// FUNCTION CALLED :                                          
//    atoi, Thread::Begin, Thread::Join                           
// -----------------------------------------------------------
int main(int argc, char** argv)
{
	int n = 30;

	if (argc > 1)
		n = atoi(argv[1]);

	// Create beginning threads master and P2.
	PrimeThread prime(2, 0);
	MasterThread master(n);
	prime.prev = &master;
	master.next = &prime;

	prime.Begin();
	master.Begin();

	// Wait for threads to finish.
	master.Join();
	prime.Join();
}
