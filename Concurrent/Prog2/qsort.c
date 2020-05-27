/* ----------------------------------------------------------- */
/* NAME : Jason bricco                       User ID: jmbricco */
/* DUE DATE : 02/24/2020                                       */
/* PROGRAM ASSIGNMENT #2                                       */
/* FILE NAME : qsort.c                                         */
/* PROGRAM PURPOSE :                                           */
/* Performs a recursive-like quicksort algorithm but creates   */
/* a new child process for each "recursive" call instead of    */
/* calling a function. Sorts the input provided by shared      */
/* memory.   				                                   */
/* ----------------------------------------------------------- */

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

/* ----------------------------------------------------------- */
/* FUNCTION  BeginQSort  									   */
/*     Creates a new process to run the qsort program.         */
/* PARAMETER USAGE :                                           */
/*    mem_id: the memory id for the array to sort.             */
/*	  left: the left bound of the array section to sort.	   */
/*	  right: the right bound of the array section to sort.     */
/* FUNCTION CALLED :                                           */
/*    fork, sprintf, execvp, write, exit, strlen		       */
/* ----------------------------------------------------------- */
static void BeginQSort(int mem_id, int left, int right)
{
	char out[256];

	/* Array of strings to use as arguments to the new process. */
	char* qsortArgs[5];

	pid_t qs_id = fork();

	if (qs_id == 0)
	{	
		char rightStr[32];
		char leftStr[32];
		char idStr[32];

		/* Format ids into strings. */
		sprintf(rightStr, "%d", right);
		sprintf(leftStr, "%d", left);
		sprintf(idStr, "%d", mem_id);

		/* Arg1 must be the program name, and the last arg must be NULL 
		so that the new process knows where the arguments end. */
		qsortArgs[0] = "qsort";
		qsortArgs[1] = leftStr;
		qsortArgs[2] = rightStr;
		qsortArgs[3] = idStr;
		qsortArgs[4] = NULL;

		if (execvp("./qsort", qsortArgs) < 0) 
		{
			sprintf(out, "Failed to execute qsort program. Error: %s\n", strerror(errno));
			write(2, out, strlen(out));
			exit(1);
		}
	}
	else if (qs_id < 0)
	{
		sprintf(out, "Failed to create child process for qsort. Error: %s\n", strerror(errno));
		write(2, out, strlen(out));
	}
}

/* ----------------------------------------------------------- */
/* FUNCTION  Swap       									   */
/*     Helper function to swap two integers.                   */
/* PARAMETER USAGE :                                           */
/*    a: the integer to swap with b.                           */
/*	  b: the integer to swap with a.                     	   */
/* ----------------------------------------------------------- */
static void Swap(int* a, int* b)
{
	int temp = *b;
	*b = *a;
	*a = temp;
}

/* ----------------------------------------------------------- */
/* FUNCTION  Partition  									   */
/*     Partitions the array such that values to the left of    */
/*     the pivot are smaller than it, and values to the right  */
/*     are larger than it.                                     */
/* PARAMETER USAGE :                                           */
/*    a: the array to partition.                               */
/*	  left: the left bound of the array section to process.	   */
/*	  right: the right bound of the array section to process.  */
/* FUNCTION CALLED :                                           */
/*    sprintf, write, getpid, Swap	                 	       */
/* ----------------------------------------------------------- */
static int Partition(int* a, int left, int right)
{
	char out[256];
	int i, j;

	/* Set the pivot point to the right of the array. */
    int pivot = a[right];  

    sprintf(out, "   ### Q-PROC(%d): pivot element is a[%d] = %d\n", getpid(), right, a[right]);
    write(1, out, strlen(out));
 
    i = (left - 1);

    /* Swap values to ensure values left of the pivot are smaller
    and right of the pivot are larger. */
    for (j = left; j <= right - 1; ++j)
    {
        if (a[j] < pivot)
        {
            ++i;
            Swap(&a[i], &a[j]);
        }
    }

    Swap(&a[i + 1], &a[right]);

    return i + 1;
}

/* ----------------------------------------------------------- */
/* FUNCTION  PrintSection  									   */
/*     Prints a section of an array to an output array.        */
/* PARAMETER USAGE :                                           */
/*    data: the array from which to print.                     */
/*	  out: the array to print to.                              */
/*	  left: the left bound of the array section to print.      */
/*	  right: the right bound of the array section to print.    */
/* FUNCTION CALLED :                                           */
/*    sprintf, write, strlen                      		       */
/* ----------------------------------------------------------- */
static void PrintSection(int* data, char* out, int left, int right)
{
	int i;
	sprintf(out, "%s       ", out);

	for (i = left; i <= right; ++i)
		sprintf(out, "%s%d ", out, data[i]);

	sprintf(out, "%s\n", out);
	write(1, out, strlen(out));
}

/* ----------------------------------------------------------- */
/* FUNCTION  main  									           */
/*     Performs quicksort "recursively" by creating child      */
/*     processes instead of calling the same function.
/* PARAMETER USAGE :                                           */
/*    argc: the amount of arguments in argv.                   */
/*    argv: an array of string arguments passed to the program.*/
/* FUNCTION CALLED :                                           */
/*    atoi, shmat, sprintf, write, strlen, exit, getpid,       */
/*    strerror, PrintSection, Partition, BeginQSort, wait,     */
/*    shmdt                                                    */
/* ----------------------------------------------------------- */
int main(int argc, char** argv)
{
	char out[256];
	int left, right, m_id, i;
	int* data;

	left = atoi(argv[1]);
	right = atoi(argv[2]);
	m_id = atoi(argv[3]);

	if (left < right)
	{
		int m;
		
		/* Attach the memory of the array to sort. */
		data = (int*)shmat(m_id, NULL, 0);

		if (data == (void*)-1)
		{
			sprintf(out, "Failed to attach shared memory. Error: %s\n", strerror(errno));
			write(2, out, strlen(out));
			exit(-1);
		}

		sprintf(out, "   ### Q-PROC(%d): entering with a[%d..%d]\n", getpid(), left, right);
		PrintSection(data, out, left, right);

		m = Partition(data, left, right);

		/* Create child processes to sort each side of the array around the pivot. */
		BeginQSort(m_id, left, m - 1);
		BeginQSort(m_id, m + 1, right);

		/* Wait for both child processes to sort before continuing. */
		for (i = 0; i < 2; ++i)
		{
			int status;
			wait(&status);
		}

		sprintf(out, "   ### Q-PROC(%d): section a[%d..%d] sorted\n", getpid(), left, right);
		PrintSection(data, out, left, right);

		sprintf(out, "   ### Q-PROC(%d): exits\n", getpid());
		write(1, out, strlen(out));

		/* We no longer need the shared memory in this process, so detach it. */
		shmdt(data);
	}
}
