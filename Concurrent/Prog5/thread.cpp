// -----------------------------------------------------------
// NAME : Jason Bricco                       User ID: jmbricco
// DUE DATE : 4/17/2020                                      
// PROGRAM ASSIGNMENT #5                                      
// FILE NAME : thread.cpp                                     
// PROGRAM PURPOSE :                                          
// Implements thread and monitor functionality to run the simulation.
// -----------------------------------------------------------

#include "thread.h"

// -----------------------------------------------------------
// FUNCTION  Simulation::AskQuestion                                 
//    an elf has a problem and asks Santa for help.
//	  Elves wait until Santa helps them.
// PARAMETER USAGE :                                          
//    id: id of the elf. 
// FUNCTION CALLED :                                          
//    PRINT (macro), Condition::Wait, Condition::Signal, 
//	  MonitorBegin, MonitorEnd                           
// -----------------------------------------------------------
void Simulation::AskQuestion(int id)
{
	MonitorBegin();

	PRINT("         Elf %d has a problem\n", id);

	// Elves wait in line if Santa is busy with reindeer/delivery
	// or if there are already 3 elves waiting for help.s
	while (elfCount == 3 || santaState != WAIT_NONE)
	{
		DEBUG_PRINT("         Elf %d is waiting in line for help\n", id);
		elfLineWait.Wait();

		if (finished)
			break;
	}

	if (!finished)
	{
		if (elfCount < 0 || elfCount >= 3)
			PRINT("ERROR: writing out of bounds of waitingElves.\n");

		waitingElves[elfCount++] = id;

		// If this is the third elf, wake Santa up.
		if (elfCount == 3)
		{
			PRINT("         Elves %d, %d, %d wake up Santa\n", waitingElves[0], waitingElves[1], waitingElves[2]);
			wakeSanta.Signal();

			// If Santa already helped these elves, then this wait is unnecessary.
			if (elfCount != 0)
			{
				DEBUG_PRINT("         Elf %d is waiting to be helped\n", id);
				waitForElves.Wait();
			}
			else DEBUG_PRINT("         Elf %d would have waited, but Santa helped it already\n", id);
		}
		else
		{
			DEBUG_PRINT("         Elf %d is waiting to be helped\n", id);
			waitForElves.Wait();

			if (printReturn)
			{
				PRINT("         Elves %d, %d, %d return to work\n", waitingElves[0], waitingElves[1], waitingElves[2]);
				printReturn = false;
			}
		}
	}

	MonitorEnd();
}

// -----------------------------------------------------------
// FUNCTION  Elf::ThreadFunc                                 
//    elf simulation loop. Elves work until they encounter a problem, 
//    at which point they ask Santa for help.
// FUNCTION CALLED :                                          
//	  Thread::ThreadFunc, PRINT (macro), Delay, Simulation::AskQuestion             
// -----------------------------------------------------------
void Elf::ThreadFunc()
{
	Thread::ThreadFunc();

	PRINT("         Elf %d starts\n", id);

	while (!sim->finished) 
	{
		// Making toys.
		Delay();

		// Elf encounters a problem.
		sim->AskQuestion(id); 

		// Problem has been solved.
		Delay();
	}

	DEBUG_PRINT("         Elf %d exits\n", id);
}

// -----------------------------------------------------------
// FUNCTION  Simulation::ReindeerBack                                 
//    the reindeer returns back to the North Pole. The last one
//	  to return wakes up Santa.
// PARAMETER USAGE :                                          
//    reindeer: the reindeer instance that returned. 
// FUNCTION CALLED :                                          
//    PRINT (macro), Condition::Signal, MonitorBegin, MonitorEnd                           
// -----------------------------------------------------------
void Simulation::ReindeerBack(Reindeer* reindeer)
{
	MonitorBegin();

	PRINT("    Reindeer %d returns\n", reindeer->id);
	
	++reindeerCount;

	DEBUG_PRINT("    There are now %d reindeer back. Maximum: %d\n", reindeerCount, maxReindeer);

	// The last reindeer wakes up Santa.
	if (reindeerCount == maxReindeer)
	{
		PRINT("    The last reindeer %d wakes up Santa\n", reindeer->id);
		wakeSanta.Signal();
	}

	MonitorEnd();
}

// -----------------------------------------------------------
// FUNCTION  Simulation::WaitOthers                                 
//    upon returning, reindeer wait for the other reindeer to
//	  be ready to attach to the sleigh.
// PARAMETER USAGE :                                          
//    id: id of the reindeer. 
// FUNCTION CALLED :                                          
//    PRINT (macro), Condition::Wait, Condition::Signal, 
//	  MonitorBegin, MonitorEnd                           
// -----------------------------------------------------------
void Simulation::WaitOthers(int id)
{
	MonitorBegin();

	if (reindeerCount < maxReindeer)
	{
		DEBUG_PRINT("    Reindeer %d is waiting for the other reindeer\n", id);
		waitForReindeer.Wait();
	}
	else
	{
		DEBUG_PRINT("    Last reindeer (%d) arrived. All waiting reindeer are freed\n", id);

		// Last reindeer arrived - all waiting reindeer are freed.
		for (int i = 0; i < reindeerCount - 1; ++i)
			waitForReindeer.Signal();
	}

	MonitorEnd();
}

// -----------------------------------------------------------
// FUNCTION  Simulation::WaitSleigh                                 
//    reindeer wait for Santa to attach them to the sleigh.
// PARAMETER USAGE :                                          
//    id: id of the reindeer.
// FUNCTION CALLED :                                          
//    PRINT (macro), Condition::Wait, Condition::Signal, 
//	  MonitorBegin, MonitorEnd                           
// -----------------------------------------------------------
void Simulation::WaitSleigh(int id)
{
	MonitorBegin();
	++sleighCount;

	if (sleighCount < maxReindeer)
	{
		DEBUG_PRINT("    Reindeer %d is ready to be attached to the sleigh\n", id);
		waitSleigh.Wait();
	}
	else
	{
		// Last reindeer arrived, signal that reindeer are ready.
		if (santaState == WAIT_SLEIGH)
		{
			DEBUG_PRINT("    The last reindeer (%d) came to wait for the sleigh. Santa is already waiting, so signal him\n", id);
			sleighReady.Signal();
		}
		else
		{
			DEBUG_PRINT("    The last reindeer (%d) came to wait for the sleigh.\n", id);
			waitSleigh.Wait();
		}
	}

	MonitorEnd();
}

// -----------------------------------------------------------
// FUNCTION  Simulation::FlyOff                                 
//    reindeeer are ready to fly off and are waiting on Santa.
// PARAMETER USAGE :                                          
//    id: id of the reindeer.
// FUNCTION CALLED :                                          
//    PRINT (macro), Condition::Wait, Condition::Signal, 
//	  MonitorBegin, MonitorEnd                           
// -----------------------------------------------------------
void Simulation::FlyOff(int id)
{
	MonitorBegin();
	++readyToFlyCount;

	if (readyToFlyCount < maxReindeer)
	{
		++flyWaitCount;
		DEBUG_PRINT("    Reindeer %d is ready to fly off\n", id);
		flyWait.Wait();
	}
	else
	{
		if (santaState == WAIT_FLY)
		{
			DEBUG_PRINT("    Last reindeer (%d) is ready to fly off. Santa is waiting, so signal him\n", id);
			flyReady.Signal();

			// If the delay from delivering somehow occurred
			// before this reindeer waited here, then Santa 
			// will be waiting on this condition variable. Signal
			// him to continue. Otherwise, wait for delivery to finish.
			if (santaState == WAIT_FLY_COMPLETE)
			{
				DEBUG_PRINT("    Last reindeer (%d) was too slow to arrive. Santa is waiting, so signaling\n", id);
				flyComplete.Signal();
			}
			else 
			{
				++flyWaitCount;
				DEBUG_PRINT("    Last reindeer (%d) that signaled santa now waits for fly off\n", id);
				flyWait.Wait();
			}
		}
		else 
		{
			++flyWaitCount;
			DEBUG_PRINT("    Last reindeer (%d) waits for fly off\n", id);
			flyWait.Wait();
		}
	}

	MonitorEnd();
}

// -----------------------------------------------------------
// FUNCTION  Reindeer::ThreadFunc                                 
//    reindeer simulation loop. They return from vacation, wait on
//    each other, attach to the sleigh, and fly off.
// FUNCTION CALLED :                                          
//	  Thread::ThreadFunc, Simulation::ReindeerBack, Simulation::WaitOthers,
//    Simulation::WaitSleigh, Simulation::FlyOff, Delay, PRINT (macro)                
// -----------------------------------------------------------
void Reindeer::ThreadFunc()
{
	Thread::ThreadFunc();

	PRINT("    Reindeer %d starts\n", id);

	while (!sim->finished) 
	{
		// Tan on beaches.
		Delay();

		sim->ReindeerBack(this);
		sim->WaitOthers(id);

		// Wait to be attached to a sleigh.
		sim->WaitSleigh(id);

		// Go deliver toys.
		sim->FlyOff(id);
		
		// Go on vacation.
		Delay();                
	}

	DEBUG_PRINT("    Reindeer %d exits\n");
}

// -----------------------------------------------------------
// FUNCTION  Simulation::Sleep                               
//    Santa tries to sleep, but won't if there are tasks to attend to.
// FUNCTION CALLED :                                          
//    PRINT (macro), Condition::Wait, Condition::Signal, 
//	  MonitorBegin, MonitorEnd                           
// -----------------------------------------------------------
void Simulation::Sleep()
{
	MonitorBegin();

	if (reindeerCount == maxReindeer) { DEBUG_PRINT("Santa wants to sleep, but reindeer are ready. Santa doesn't sleep\n"); }
	else if (elfCount == 3) { DEBUG_PRINT("Santa wants to sleep, but elves need help. Santa doesn't sleep\n"); }
	else 
	{
		DEBUG_PRINT("Santa is now sleeping\n");
		wakeSanta.Wait();
	}

	MonitorEnd();
}

// -----------------------------------------------------------
// FUNCTION  Simulation::AttachSleigh                                 
//    Santa attaches the sleigh to the reindeer.
// FUNCTION CALLED :                                          
//    PRINT (macro), Condition::Wait, Condition::Signal, 
//	  MonitorBegin, MonitorEnd                           
// -----------------------------------------------------------
void Simulation::AttachSleigh()
{
	MonitorBegin();

	santaState = WAIT_BUSY;

	// Wait for reindeer to be ready to be attached to the sleigh.
	if (sleighCount < maxReindeer)
	{
		santaState = WAIT_SLEIGH;
		DEBUG_PRINT("Not all reindeer are ready for the sleigh. Santa waits.\n");
		sleighReady.Wait();
	}

	PRINT("Santa is preparing sleighs\n");

	// Allow all the reindeer to go fly off.
	for (int i = 0; i < maxReindeer; ++i)
		waitSleigh.Signal();

	MonitorEnd();
}

// -----------------------------------------------------------
// FUNCTION  Simulation::PrepareForFly                                
//    Santa makes sure all reindeer are ready to fly, before taking off.
// FUNCTION CALLED :                                          
//    PRINT (macro), Condition::Wait, MonitorBegin, MonitorEnd                           
// -----------------------------------------------------------
void Simulation::PrepareForFly()
{
	MonitorBegin();

	// Wait for reindeer to be ready to fly.
	if (readyToFlyCount < maxReindeer)
	{
		santaState = WAIT_FLY;
		DEBUG_PRINT("Not all reindeer are ready to fly off. Santa waits\n");
		flyReady.Wait();
	}

	MonitorEnd();
}

// -----------------------------------------------------------
// FUNCTION  Simulation::ReleaseReindeer                                 
//    delivery is complete. Reindeer are released back to the tropics.
//	  Santa retires after enough deliveries.
// FUNCTION CALLED :                                          
//    PRINT (macro), Condition::Wait, Condition::Signal, 
//	  MonitorBegin, MonitorEnd, Simulation::ResetValues                         
// -----------------------------------------------------------
void Simulation::ReleaseReindeer()
{
	MonitorBegin();

	if (flyWaitCount != maxReindeer)
	{
		santaState = WAIT_FLY_COMPLETE;
		DEBUG_PRINT("Santa is ready to release the reindeer, but not all reindeer are blocked on flyWait. Santa waits.\n");
		flyComplete.Wait();
	}

	ResetValues();

	DEBUG_PRINT("Santa releases all reindeer and delivery is complete\n");

	for (int i = 0; i < maxReindeer; ++i)
		flyWait.Signal();

	if (finished)
	{
		// Free up all blocked elves so that the program
		// can exit.
		for (int i = 0; i < 3; ++i)
			waitForElves.Signal();

		elfCount = 0;

		for (int i = 0; i < maxElves; ++i)
			elfLineWait.Signal();
	}

	MonitorEnd();
}

// -----------------------------------------------------------
// FUNCTION  Simulation::ReleaseElves                                 
//    releases elves after answering their questions.
// FUNCTION CALLED :                                          
//    PRINT (macro), Condition::Signal, MonitorBegin, MonitorEnd                           
// -----------------------------------------------------------
void Simulation::ReleaseElves()
{
	MonitorBegin();

	printReturn = true;

	// Allow the three elves being helped to continue on.
	for (int i = 0; i < 3; ++i)
		waitForElves.Signal();

	elfCount = 0;
	
	// Try to allow all waiting in line to continue. Some will be next
	// for being helped, others might stay in line.
	for (int i = 0; i < maxElves; ++i)
		elfLineWait.Signal();

	MonitorEnd();
}

// -----------------------------------------------------------
// FUNCTION  Simulation::ReindeerReady                                 
//    returns whether reindeer are ready to be attached to the sleigh.
// FUNCTION CALLED :                                          
//	  MonitorBegin, MonitorEnd                           
// -----------------------------------------------------------
bool Simulation::ReindeerReady()
{
	bool result;

	MonitorBegin();
	result = reindeerCount == maxReindeer;
	MonitorEnd();

	return result;
}

// -----------------------------------------------------------
// FUNCTION  Simulation::ElfHelpNeeded                                 
//    returns whether there is a group of elves waiting for help.
// FUNCTION CALLED :                                          
//	  MonitorBegin, MonitorEnd                           
// -----------------------------------------------------------
bool Simulation::ElfHelpNeeded()
{
	bool result;

	MonitorBegin();
	result = elfCount == 3;
	MonitorEnd();

	return result;
}

// -----------------------------------------------------------
// FUNCTION  Santa::ThreadFunc                                 
//    Santa's simulation loop. He sleeps and handles reindeer 
//    and elf tasks.
// FUNCTION CALLED :                                          
//	  Thread::ThreadFunc, Simulation::Sleep, Simulation::ReindeerReady,
//    Simulation::AttachSleigh, Simulation::PrepareForFly, PRINT (macro),
//    Delay, Simulation::ReleaseReindeer, Simulation::ElfHelpNeeded,
//    Simulation::ReleaseElves                        
// -----------------------------------------------------------
void Santa::ThreadFunc()
{
	Thread::ThreadFunc();

	PRINT("Santa thread starts\n");

	while (!sim->finished)
	{
		sim->Sleep();

		// If reindeer are waiting, get them ready for delivery.
		if (sim->ReindeerReady())
		{
			sim->AttachSleigh();
			sim->PrepareForFly();

			++deliverCount;
			PRINT("The team flies off (%d)!\n", deliverCount);

			// Delivering toys.
			Delay();

			if (deliverCount == maxDeliver)
				sim->finished = true;

			sim->ReleaseReindeer();
		}

		// Check elves second, if there are elves needing help.
		if (sim->ElfHelpNeeded())
		{
			int* waiting = sim->waitingElves;
			PRINT("Santa answers the question posted by elves %d, %d, %d\n", waiting[0], waiting[1], waiting[2]);
			Delay();
			sim->ReleaseElves();
		}
	}

	PRINT("After (%d) deliveries, Santa retires and is on vacation!\n", deliverCount);
}

// -----------------------------------------------------------
// FUNCTION  Simulation::ResetValues                                 
//    resets state values to their default values.                       
// -----------------------------------------------------------
void Simulation::ResetValues()
{
	reindeerCount = 0;
	sleighCount = 0;
	readyToFlyCount = 0;
	flyWaitCount = 0;
	santaState = WAIT_NONE;
}
