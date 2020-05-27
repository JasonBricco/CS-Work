// -----------------------------------------------------------
// NAME : Jason Bricco                       User ID: jmbricco
// DUE DATE : 4/17/2020                                      
// PROGRAM ASSIGNMENT #5                                      
// FILE NAME : thread.h                                     
// PROGRAM PURPOSE :                                          
// Defines thread classes and the monitor used for simulation.
// -----------------------------------------------------------

#include <stdio.h>
#include <string.h>

#include "ThreadClass.h"

// If 1, then DEBUG_PRINT will be enabled.
// Otherwise, it will be compiled out.
#define DEBUG_TEXT 0

// Helper printing macro for convenience.
#define PRINT(...) \
{ \
    char buf[256]; \
    sprintf(buf, __VA_ARGS__); \
    write(1, buf, strlen(buf)); \
}

#if DEBUG_TEXT
#define DEBUG_PRINT(...) PRINT(__VA_ARGS__)
#else
#define DEBUG_PRINT
#endif

struct Elf;
struct Reindeer;

// State for Santa to be in while handling
// reindeer and delivering presents.
enum SantaState
{
	WAIT_NONE,
	WAIT_BUSY,
	WAIT_SLEIGH,
	WAIT_FLY,
	WAIT_FLY_COMPLETE
};

// The monitor responsible for ensuring the
// simulation works correctly.
struct Simulation : public Monitor
{
	// Elves waiting for their problem to be solved.
	int elfCount, maxElves;
	int waitingElves[3];

	int reindeerCount, maxReindeer;

	// Used to wait until all reindeer return from vacation.
	Condition waitForReindeer;

	// The number of reindeer waiting to be attached to the sleigh.
	// When waitSleigh is signaled, the reindeer are released to continue on.
	int sleighCount;
	Condition waitSleigh;

	// Santa waits until all reindeer are ready to be attached to the 
	// sleigh. When this is signaled, he continues on.
	Condition sleighReady;

	// Used to wait for a group of 3 elves.
	Condition waitForElves, elfLineWait;

	// Signaled to wake santa up.
	Condition wakeSanta;

	// When signaled, Santa and the reindeer are ready to fly off.
	Condition flyWait, flyReady, flyComplete;
	int readyToFlyCount, flyWaitCount;

	SantaState santaState;

	bool finished;
	bool printReturn;

	// Reindeer functions.
	void ReindeerBack(Reindeer* reindeer);
	void WaitOthers(int id);
	void WaitSleigh(int id);
	void FlyOff(int id);

	// Elf functions.
	void AskQuestion(int id);

	// Santa functions.
	void Sleep();
	void AttachSleigh();
	void PrepareForFly();
	bool ReindeerReady();
	void ReleaseReindeer();
	void ReleaseElves();
	bool ElfHelpNeeded();

	void ResetValues();

	// Constructor.
	Simulation() : waitForElves("ElfWait"), waitForReindeer("ReindeerWait"), wakeSanta("WakeSanta"), flyWait("FlyWait"), 
				   flyReady("FlyReady"), flyComplete("FlyComplete"), waitSleigh("SleighWait"), sleighReady("SleighReady"), 
				   elfLineWait("ElfLine"), Monitor("Monitor", HOARE)
	{
		finished = false;
		elfCount = 0;
		ResetValues();
	}
};

struct Santa : public Thread
{
	int deliverCount, maxDeliver;
	Simulation* sim;

	void ThreadFunc();
};

struct Reindeer : public Thread
{
	int id;
	Simulation* sim;

	void ThreadFunc();

	Reindeer(Simulation* sim, int id) : sim(sim), id(id) {}
};

struct Elf : public Thread
{
	int id;
	Simulation* sim;

	void ThreadFunc();

	Elf(Simulation* sim, int id) : sim(sim), id(id) {}
};
