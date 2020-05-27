// -----------------------------------------------------------
// NAME : Jason bricco                       User ID: jmbricco
// DUE DATE : 4/1/2020                                      
// PROGRAM ASSIGNMENT #4                                      
// FILE NAME : thread-support.cpp                                     
// PROGRAM PURPOSE :                                          
// Contains the GoToParty and CheckRoom functions.
// -----------------------------------------------------------

#include "thread.h"

// -----------------------------------------------------------
// FUNCTION  Landlord::CheckRoom                                 
//    the landlord checks the room, preventing others from
//    entering or leaving. If there are too many students,
//    every student will be kicked out. On the final check,
//    the room is cleared and closed.
// PARAMETER USAGE :                                          
//    check: the check the landlord is currently performing.  
// FUNCTION CALLED :                                          
//    HighPriorityLock, Delay, PRINT (macro), Clear, Close, 
//    HighPriorityUnlock, WaitForThreads                            
// -----------------------------------------------------------
void Landlord::CheckRoom(int check)
{
	room->HighPriorityLock();

	// Block the room to prevent entering while in here.
	room->state = ROOM_BLOCKED;

	// Allow some time for the landlord to do the check.
	Delay();

	PRINT("The landlord checks the room the %d-th time\n", check + 1);

	int size = room->size;

	if (size == 0)
		PRINT("The landlord finds the room has no students and leaves\n")
	else
	{
		if (size > maxInRoom)
		{
			PRINT("The landlord finds %d students in the room and breaks up the party\n");
			PRINT("The landlord finishes checking and forces everyone to leave\n");

			// Release the lock, allowing students to operate again.
			// However, they won't be able to enter since the room is blocked.
			// Then wait for all students to leave.
			room->HighPriorityUnlock();

			// Wait until everyone is gone.
			room->waitForEmpty.Wait();

			room->HighPriorityLock();
			PRINT("The landlord leaves the room and the room is empty\n");
		}
		else PRINT("The landlord finds there are %d students in the room (condition being good) and leaves.\n", size);
	}

	bool final = check == checks - 1;
	room->state = final ? ROOM_CLOSED : ROOM_OPEN;

	// Release threads that tried to enter the room while it was blocked,
	// while the landlord gave up the lock to wait for students to leave.
	for (int i = 0; i < room->waitingForUnblockCount; ++i)
		room->waitingForUnblock.Signal();

	room->waitingForUnblockCount = 0;

	room->HighPriorityUnlock();
	
	if (final)
	{
		// Wait for all threads to finish executing before continuing.
		room->waitForFinish.Wait();
		PRINT("After checking the room %d times, the landlord retires and is on vacation\n", checks);
	}
}

// -----------------------------------------------------------
// FUNCTION  Student::GoToParty                                 
//    the student goes to the party by adding itself to the room, 
//    waits for a while, then leaves.  
// FUNCTION CALLED :                                          
//    AddStudent, Delay, RemoveStudent                         
// -----------------------------------------------------------
void Student::GoToParty()
{
	room->AddStudent(this);
	Delay();
	room->RemoveStudent(this);
}
