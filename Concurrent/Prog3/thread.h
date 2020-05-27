// -----------------------------------------------------------
// NAME : Jason bricco                       User ID: jmbricco
// DUE DATE : 03/18/2020                                      
// PROGRAM ASSIGNMENT #3                                      
// FILE NAME : thread.h                                       
// PROGRAM PURPOSE :                                          
// Defines the SwapThread class and a helper PRINT macro.     
// -----------------------------------------------------------

#include <cstdio>
#include <unistd.h>

#include "ThreadClass.h"

// Helper printing macro for convenience.
#define PRINT(...) \
{ \
    char buf[256]; \
    sprintf(buf, __VA_ARGS__); \
    write(1, buf, strlen(buf)); \
}

// The thread class. Contains information for the
// thread to do its work.
class SwapThread : public Thread
{
private:
    int id;
    int* x;
    int slot;
    bool* swapped;

    void ThreadFunc();

public:
    void RunTask(int id, int* nums, int slt, bool* swp);
};
