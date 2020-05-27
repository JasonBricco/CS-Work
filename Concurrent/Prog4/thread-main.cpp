// -----------------------------------------------------------
// NAME : Jason bricco                       User ID: jmbricco
// DUE DATE : 4/1/2020                                      
// PROGRAM ASSIGNMENT #4                                      
// FILE NAME : thread-main.cpp                                     
// PROGRAM PURPOSE :                                          
// Reads command line arguments and initializes the threads for
// simulating the Party Room problem.  
// -----------------------------------------------------------

#include <time.h>
#include <stdlib.h>
#include "thread.h"

// -----------------------------------------------------------
// FUNCTION  main                                 
//    receives input arguments and begins the threads for
//	  execution.
// PARAMETER USAGE :                                          
//    argc: the number of arguments passed to the program.
//	  argv: the arguments psased to the program.    
// FUNCTION CALLED :                                          
//    srand, atoi, SetData, Begin, Join                             
// -----------------------------------------------------------
int main(int argc, char** argv)
{
	// Initialize the random generation seed based on current time.
	srand(time(0));
	
	// Read command line input, setting default values as
	// appropriate.
	int checks = atoi(argv[1]);

	if (checks == 0)
		checks = 5;

	int maxInRoom = atoi(argv[2]);

	if (maxInRoom == 0)
		maxInRoom = 5;

	int totalStudents = atoi(argv[3]);

	if (totalStudents == 0)
		totalStudents = 10;

	// Initialize the room.
	Room* room = new Room();
	room->max = totalStudents;
	room->threadsLeft = totalStudents;
	
	Student students[MAX_STUDENTS] = {};

	// Create student threads and set the data they need.
	for (int i = 0; i < totalStudents; ++i)
	{
		students[i].SetData(room, i);
		students[i].Begin();
		PRINT("     Student %d starts\n", i);
	}
	
	// Create the landlord thread.
	Landlord landlord(students, totalStudents, room, checks, maxInRoom);
	landlord.Begin();

	// Wait on all threads to finish executing.
	for (int i = 0; i < totalStudents; ++i)
		students[i].Join();

	landlord.Join();
}
