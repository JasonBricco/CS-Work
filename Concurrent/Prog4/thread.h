// -----------------------------------------------------------
// NAME : Jason bricco                       User ID: jmbricco
// DUE DATE : 4/1/2020                                      
// PROGRAM ASSIGNMENT #4                                      
// FILE NAME : thread.h                                     
// PROGRAM PURPOSE :                                          
// Defines the Student/Landlord threads and the Room structure.
// -----------------------------------------------------------

#include <stdio.h>
#include <string.h>
#include "ThreadClass.h"

#define MAX_STUDENTS 30

// Helper printing macro for convenience.
#define PRINT(...) \
{ \
    char buf[256]; \
    sprintf(buf, __VA_ARGS__); \
    write(1, buf, strlen(buf)); \
}

class Student;

enum RoomState
{
	ROOM_OPEN,
	ROOM_BLOCKED,
	ROOM_CLOSED
};

// Represents the party room. It has an array of students 
// along with a count of how many are in it. It also
// defines mutexes for thread protection.
struct Room
{
	int max;
	int size;
	Student* students[MAX_STUDENTS];

	// If closed, no students can enter.
	RoomState state;

	// The number of student threads still executing.
	int threadsLeft;
	Mutex countLock;

	// Count of threads waiting to be added to the room
	// while the room is blocked (due to the landlord).
	// The semaphore is used to control this.
	int waitingForUnblockCount;
	Semaphore waitingForUnblock;

	// Locks for a priority system.
	// lowPriorityLock is used by student threads to
	// ensure only one can modify the room array at once.
	// nextLock is used by both the students and landlords
	// before locking the dataLock, and dataLock protects
	// the array's access.
	Mutex lowPriorityLock;
	Mutex nextLock;
	Mutex dataLock;

	// Used to allow the landlord to wait for the room to
	// be empty, and for all threads to finish.
	Semaphore waitForEmpty;
	Semaphore waitForFinish;

	void LowPriorityLock();
	void LowPriorityUnlock();
	void HighPriorityLock();
	void HighPriorityUnlock();

	void DecrementThreadCount();
	void WaitForThreads();

	void AddStudent(Student* s);
	void RemoveStudent(Student* s);
	void Clear();
	void WaitForTermination();

	void Close();
};

class Student : public Thread
{
	int id;
	Room* room;

	void GoToParty();
    void ThreadFunc();

public:
	bool inRoom;

	// Returns the student's ID (getter).
	inline int ID()
	{
		return id;
	}

	// Sets data the student will need,
	// as if in a constructor.
	inline void SetData(Room* r, int i)
	{
		id = i;
		room = r;
	}
};

class Landlord : public Thread
{
	Student* students;
	int totalStudents;

	Room* room;

	// The number of checks to be performed.
	int checks;

	// The max number of students who can be in the room.
	int maxInRoom;

	void CheckRoom(int check);
    void ThreadFunc();

public:
	Landlord(Student* list, int tot, Room* r, int m, int n) : students(list), totalStudents(tot), room(r), checks(m), maxInRoom(n) {}
};

