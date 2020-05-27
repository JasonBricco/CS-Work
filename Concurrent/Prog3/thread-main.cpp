// -----------------------------------------------------------
// NAME : Jason bricco                       User ID: jmbricco
// DUE DATE : 03/18/2020                                       
// PROGRAM ASSIGNMENT #3                                       
// FILE NAME : thread-main.cpp                                 
// PROGRAM PURPOSE :                                           
// Performs the even-odd sorting algorithm using background    
// threads to perform each swap.                               
// ----------------------------------------------------------- 

#include <algorithm>
#include <cstring>
#include <stdint.h>

#include "thread.h"

#define EVEN 0
#define ODD 1

// -----------------------------------------------------------
// FUNCTION  BeginPass                                 
//     creates threads to perform an even or odd sorting pass.
// PARAMETER USAGE :                                          
//    x: the array to sort.                                 
//    arrCount: the number of items in 'arr'.                 
//    pass: whether it's an even or odd pass.                 
//    threadCount: the number of threads to create.           
//    swapped: set to true if a swap occurred.                
// FUNCTION CALLED :                                          
//    RunTask, Join                                           
// -----------------------------------------------------------
static void BeginPass(int* x, int arrCount, int pass, int threadCount, bool* swapped)
{
    // Temporarily allocate enough threads to perform the sort.
    SwapThread* threads = new SwapThread[threadCount]();

    for (int i = 0; i < threadCount; ++i)
    {
        int slot = 1 + pass + (i * 2);

        if (slot < arrCount)
            threads[i].RunTask(i, x, slot, swapped);
    }

    // Wait on all threads to complete in order to finish this pass.
    for (int i = 0; i < threadCount; ++i)
        threads[i].Join();

    delete[] threads;
}

// -----------------------------------------------------------
// FUNCTION  PrintArray                                 
//     prints the contents of an integer array, limiting
//     the number of prints to at most 20.
// PARAMETER USAGE :                                          
//    x: the array to print.
//    count: the number of items in the array.              
// FUNCTION CALLED :                                          
//    sprintf, write, strlen (from PRINT), std::min                                  
// -----------------------------------------------------------
static void PrintArray(int* x, int count)
{
    for (int i = 0; i < std::min(count, 20); ++i)
        PRINT("   %d ", x[i]);

    PRINT("\n");
}

// -----------------------------------------------------------
// FUNCTION  main                                 
//     performs the even-odd sorting algorithm on an array
//     read from stdin, using background threads for swapping.           
// FUNCTION CALLED :                                          
//    sprintf, write, strlen (from PRINT), fscanf, PrintArray,
//    BeginPass                              
// -----------------------------------------------------------
int main()
{
    PRINT("Concurrent Even-Odd Sort\n");

    int32_t count;

    if (fscanf(stdin, "%d", &count) != 1)
    {
        PRINT("Failed to read the data count.\n");
        return -1;
    }

    PRINT("Number of input data = %d\n", count);
    PRINT("Input array:\n");

    // Allocate the storage for the array to sort.
    // This will be freed when the program exits.
    int* x = new int[count];

    for (int i = 0; i < count; ++i)
    {
        if (fscanf(stdin, "%d", &x[i]) < 0)
        {
            PRINT("Failed to read an array item");
            return -1;
        }
    }

    PrintArray(x, count);

    int evenCount = count / 2;
    int oddCount = (count - 1) / 2;

    // Loop as long as the array is not swapped, performing
    // an even and then odd sorting pass on each iteration.
    int i = 0;
    while (i < count)
    {
        PRINT("Iteration %d:\n", i);

        bool swapped = false;

        PRINT("   Even Pass:\n");
        BeginPass(x, count, EVEN, evenCount, &swapped);

        PRINT("   Odd Pass:\n");
        BeginPass(x, count, ODD, oddCount, &swapped);

        PRINT("Result after iteration %d:\n", i);
        PrintArray(x, count);

        // No swaps occurred. We're done.
        if (!swapped)
            break;

        ++i;
    }

    PRINT("Final result after iteration %d:\n", i);
    PrintArray(x, count);
}
