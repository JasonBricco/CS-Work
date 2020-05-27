// -----------------------------------------------------------
// NAME : Jason bricco                       User ID: jmbricco
// DUE DATE : 4/1/2020                                      
// PROGRAM ASSIGNMENT #4                                      
// FILE NAME : thread.cpp                                     
// PROGRAM PURPOSE :                                          
// Contains code for the threads to run, as well as room
// functionality.  
// -----------------------------------------------------------

#include "thread.h"

// -----------------------------------------------------------
// FUNCTION  Landlord::ThreadFunc                                 
//    landlord's thread code. He periodically checks the 
//	  room to ensure it is kept in order.
// FUNCTION CALLED :                                          
//    Thread::ThreadFunc, Delay, CheckRoom                            
// -----------------------------------------------------------
void Landlord::ThreadFunc()
{
	Thread::ThreadFunc();

	for (int i = 0; i < checks; ++i)
	{
		Delay();
	    CheckRoom(i);
	    Delay();  
	}
}

// -----------------------------------------------------------
// FUNCTION  Student::ThreadFunc                                 
//    student's thread code. Students try to go to (or leave)
//	  the party.
// FUNCTION CALLED :                                          
//    Thread::ThreadFunc, Delay, GoToParty, DecrementThreadCount                            
// -----------------------------------------------------------
void Student::ThreadFunc()
{
	Thread::ThreadFunc();

	while (room->state != ROOM_CLOSED) 
	{
		Delay();
		GoToParty();
		Delay();
	}

	// Student thread has finished.
	room->DecrementThreadCount();
}

// -----------------------------------------------------------
// FUNCTION  Room::LowPriorityLock                                 
//    performs a low priority lock using mutexes. This lock
//	  works in a way such that a high priority lock will 
//	  have execution priority.
// FUNCTION CALLED :                                          
//    Lock, Unlock                           
// -----------------------------------------------------------
void Room::LowPriorityLock()
{
	// Only one student can get past the lowPriorityLock.
	// nextLock ensures the high priority task gets a chance
	// to access the data, given the mutex is not fair.
	// dataLock protects the actual array modification.
	lowPriorityLock.Lock();
	nextLock.Lock();
	dataLock.Lock();
	nextLock.Unlock();
}

// -----------------------------------------------------------
// FUNCTION  Room::LowPriorityUnlock                                 
//    performs a low priority unlock using mutexes.   
// FUNCTION CALLED :                                          
//    Unlock                            
// -----------------------------------------------------------
void Room::LowPriorityUnlock()
{
	dataLock.Unlock();
	lowPriorityLock.Unlock();
}

// -----------------------------------------------------------
// FUNCTION  Room::HighPriorityLock                                 
//    performs a priority lock using mutexes. This will ensure
//	  that if a low priority thread and a high priority thread
//	  both try to lock, this will go first.  
// FUNCTION CALLED :                                          
//    Lock, Unlock                           
// -----------------------------------------------------------
void Room::HighPriorityLock()
{
	nextLock.Lock();
	dataLock.Lock();
	nextLock.Unlock();
}

// -----------------------------------------------------------
// FUNCTION  Room::HighPriorityUnlock                                 
//    performs a high priority unlock.
// FUNCTION CALLED :                                          
//    Unlock                            
// -----------------------------------------------------------
void Room::HighPriorityUnlock()
{
	dataLock.Unlock();
}

// -----------------------------------------------------------
// FUNCTION  Room::DecrementThreadCount                                 
//    decrements the count of the running threads in a 
//	  protected manner.
// FUNCTION CALLED :                                          
//    Lock, Unlock                          
// -----------------------------------------------------------
void Room::DecrementThreadCount()
{
	countLock.Lock();
	--threadsLeft;
	countLock.Unlock();

	if (threadsLeft == 0)
		waitForFinish.Signal();
}

// -----------------------------------------------------------
// FUNCTION  Room::AddStudent                                 
//    attempts to add a student to the room. The student must
//	  wait if others are entering or leaving, or if the landlord
//    is in the room.
// PARAMETER USAGE :                                          
//    s: the student to add to the room.  
// FUNCTION CALLED :                                          
//    LowPriorityLock, PRINT (macro), LowPriorityUnlock, ID                             
// -----------------------------------------------------------
void Room::AddStudent(Student* s)
{
	if (state != ROOM_CLOSED)
	{
		PRINT("     Student %d waits to enter the room\n", s->ID());
		LowPriorityLock();

		// If the room is blocked, we must wait before entering.
		// Give up the lock so other students can leave.
		while (state == ROOM_BLOCKED)
		{
			++waitingForUnblockCount;
			LowPriorityUnlock();

			waitingForUnblock.Wait();

			// Re-acquire the lock before trying to enter the room.
			LowPriorityLock();

			// The room may have been blocked while this thread was stuck
			// trying to get the lock. If so, we continue this loop.
			// Otherwise we can move on with the lock. This happens if
			// the landlord comes back again quickly.
			if (state != ROOM_BLOCKED)
				break;
		}

		// Don't enter if the room is now closed.
		if (state != ROOM_CLOSED)
		{
			students[size++] = s;
			s->inRoom = true;
			PRINT("     Student %d enters the room (%d students in the room)\n", s->ID(), size);
		}

		LowPriorityUnlock();
	}
}

// -----------------------------------------------------------
// FUNCTION  Room::RemoveStudent                                 
//    attempts to remove a student from the room. The student
//	  must wait if others are entering or leaving, or if the 
//    landlord is in the room. If the student isn't in the room,
//    no removal will occur.
// PARAMETER USAGE :                                          
//    s: the student to remove from the room. 
// FUNCTION CALLED :                                          
//    PRINT (macro), LowPriorityLock, ID, LowPriorityUnlock                            
// -----------------------------------------------------------
void Room::RemoveStudent(Student* s)
{
	if (s->inRoom)
	{
		PRINT("     Student %d waits to leave the room\n", s->ID());
		LowPriorityLock();

		for (int i = 0; i < size; ++i)
		{
			if (students[i]->ID() == s->ID())
			{
				students[i] = students[--size];
				s->inRoom = false;
				PRINT("     Student %d leaves the room (%d students in the room)\n", s->ID(), size);

				// If the room is blocked, the landlord is in it waiting.
				// If the room is now empty, signal the semaphore.
				if (state == ROOM_BLOCKED && size == 0)
					waitForEmpty.Signal();

				break;
			}
		}

		LowPriorityUnlock();
	}
}
