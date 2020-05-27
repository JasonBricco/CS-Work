/* ----------------------------------------------------------- */
/* NAME : Jason bricco                       User ID: jmbricco */
/* DUE DATE : 02/24/2020                                       */
/* PROGRAM ASSIGNMENT #2                                       */
/* FILE NAME : main.c                                          */
/* PROGRAM PURPOSE :                                           */
/* Allocates all shared memory needed by child processes. Then */
/* it spawns a child process to perform quicksort and one to   */
/* merge two input arrays into a final array.         	       */
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
/* FUNCTION  BeginMerge  									   */
/*     Creates a new process to run the merge program.         */
/* PARAMETER USAGE :                                           */
/*    id_x: the memory id for the first input array.           */
/*	  id_y: the memory id for the second input array.		   */
/*	  id_out: the memory id for the output array.              */
/* FUNCTION CALLED :                                           */
/*    fork, sprintf, execvp, write, strerror, strlen		   */
/* ----------------------------------------------------------- */
static void BeginMerge(int id_x, int id_y, int id_out)
{
	char out[256];

	/* Array of strings to use as arguments to the new process. */
	char* mergeArgs[5];

	pid_t ms_id = fork();

	if (ms_id == 0)
	{	
		char idStrX[32];
		char idStrY[32];
		char idStrOut[32];

		/* Format ids into strings. */
		sprintf(idStrX, "%d", id_x);
		sprintf(idStrY, "%d", id_y);
		sprintf(idStrOut, "%d", id_out);

		/* Arg1 must be the program name, and the last arg must be NULL 
		so that the new process knows where the arguments end. */
		mergeArgs[0] = "merge";
		mergeArgs[1] = idStrX;
		mergeArgs[2] = idStrY;
		mergeArgs[3] = idStrOut;
		mergeArgs[4] = NULL;

		/* Create a new process to run merge, replacing this process. */
		if (execvp("./merge", mergeArgs) < 0) 
		{
			sprintf(out, "Failed to execute merge program. Error: %s\n", strerror(errno));
			write(2, out, strlen(out));
		}
	}
	else if (ms_id < 0)
	{
		sprintf(out, "Failed to create child process for merge. Error: %s\n", strerror(errno));
		write(2, out, strlen(out));
	}
}

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
/* FUNCTION  AllocSharedMemory  							   */
/*     Allocates and attaches a shared memory segment.         */
/* PARAMETER USAGE :                                           */
/*    str: a label specifying the "purpose" of this memory.    */
/*	  size: size in bytes of the memory to allocate.	       */
/*	  key_id: the key character to use when allocating memory. */
/*    mem_id: stores the memory id used for attaching.         */
/* FUNCTION CALLED :                                           */
/*    ftok, sprintf, write, strlen, shmget, strerror, shmat	   */
/* ----------------------------------------------------------- */
static int* AllocSharedMemory(char* str, int size, char key_id, int* mem_id)
{
	char out[256];
	int* data;

	/* Create a key using the current argument and an id character. */
	key_t key = ftok("./", key_id);

	sprintf(out, "*** MAIN: %s shared memory key = %d\n", str, key);
	write(1, out, strlen(out));

	/* Allocate shared memory of the given size. */
	*mem_id = shmget(key, size, IPC_CREAT | 0666);

	if (*mem_id == -1)
	{
		sprintf(out, "Failed to allocate shared memory. ID: %c. Size: %d. Error: %s\n", key_id, size, strerror(errno));
		write(2, out, strlen(out));
		return NULL;
	}

	sprintf(out, "*** MAIN: %s shared memory created.\n", str);
	write(1, out, strlen(out));

	/* Attach the shared memory and store the resulting id. */
	data = (int*)shmat(*mem_id, NULL, 0);

	if (data == (void*)-1)
	{
		sprintf(out, "Failed to attach shared memory. ID: %c. Error: %s\n", key_id, strerror(errno));
		write(2, out, strlen(out));
		return NULL;
	}

	sprintf(out, "*** MAIN: %s shared memory attached and is ready to use.\n\n", str);
	write(1, out, strlen(out));

	return data;
}

/* ----------------------------------------------------------- */
/* FUNCTION  GetSharedArray  							       */
/*     Reads an array from the input file into shared memory.  */
/* PARAMETER USAGE :                                           */
/*    str: a label specifying the "purpose" of this memory.    */
/*	  file: the file to read data from.                	       */
/*	  key_id: the key to use when allocating memory.           */
/*    mem_id: stores the memory id used for attaching.         */
/* FUNCTION CALLED :                                           */
/*    fscanf, AllocSharedMemory                           	   */
/* ----------------------------------------------------------- */
static int* GetSharedArray(char* str, FILE* file, char key_id, int* mem_id)
{
	int i;
	int* data;
	int count;

	/* Read the array size from the file. */
	fscanf(file, "%d", &count);

	/* Allocate enough space for count integers + an extra integer to store the array size. */
	data = AllocSharedMemory(str, count * sizeof(int) + sizeof(int), key_id, mem_id);

	if (data == NULL)
		return NULL;

	data[0] = count;

	/* Read array data into the shared memory. */
	for (i = 1; i < count + 1; ++i)
		fscanf(file, "%d", &data[i]);

	return data;
}

/* ----------------------------------------------------------- */
/* FUNCTION  PrintArray      							       */
/*     Helper function to print an array to console.           */
/* PARAMETER USAGE :                                           */
/*    arr: the array to print. Note: it is assumed the size    */
/*         of the array is stored in its first slot.           */
/* FUNCTION CALLED :                                           */
/*    sprintf, write, strlen                        	       */
/* ----------------------------------------------------------- */
static void PrintArray(int* arr)
{
	char out[256];
	int i;
	int count = arr[0];

	for (i = 1; i < count + 1; ++i)
	{
		sprintf(out, "%d ", arr[i]);
		write(1, out, strlen(out));
	}

	sprintf(out, "\n");
	write(1, out, strlen(out));
}

/* ----------------------------------------------------------- */
/* FUNCTION  DetachMemory      							       */
/*     Detaches shared memory and prints a success message.    */
/* PARAMETER USAGE :                                           */
/*    str: the memory label for printing purposes.             */
/*    mem: the pointer to the memory to detach.                */
/* FUNCTION CALLED :                                           */
/*    shmdt, sprintf, write, strlen                       	   */
/* ----------------------------------------------------------- */
static void DetachMemory(char* str, int* mem)
{
	char out[256];
	shmdt(mem);

	sprintf(out, "*** MAIN: %s shared memory successfully detached\n", str);
	write(1, out, strlen(out));
}

/* ----------------------------------------------------------- */
/* FUNCTION  RemoveMemory      							       */
/*     Frees allocated shared memory and prints.               */
/* PARAMETER USAGE :                                           */
/*    str: the memory label for printing purposes.             */
/*    id: the id of the memory to free.                        */
/* FUNCTION CALLED :                                           */
/*    shmctl, sprintf, write, strlen                       	   */
/* ----------------------------------------------------------- */
static void RemoveMemory(char* str, int id)
{
	char out[256];
	shmctl(id, IPC_RMID, NULL);

	sprintf(out, "*** MAIN: %s shared memory successfully removed\n", str);
	write(1, out, strlen(out));
}

/* ----------------------------------------------------------- */
/* FUNCTION  main            							       */
/*     Reads file input, allocates shared memory, and spawns   */
/*     processes for performing quicksort and merge.           */
/* PARAMETER USAGE :                                           */
/*    argc: the amount of arguments in argv.                   */
/*    argv: an array of string arguments passed to the program.*/
/* FUNCTION CALLED :                                           */
/*    fopen, sprintf, write, strlen, GetSharedArray,           */
/*    AllocSharedMemory, PrintArray, BeginQSort, BeginMerge,   */
/*    wait, DetachMemory, RemoveMemory                         */
/* ----------------------------------------------------------- */
int main(int argc, char** argv)
{
	int qsort_id, merge_id_x, merge_id_y, merge_id_out;
	int i;
	pid_t qs_id;
	char out[256];
	int* a, *x, *y, *merged;

	FILE* file = fopen(argv[1], "r");

	if (file == NULL)
	{
		sprintf(out, "Failed to open file %s", argv[1]);
		write(2, out, strlen(out));
		return -1;
	}

	sprintf(out, "Quicksort and Binary Merge with Multiple Processes:\n\n");
	write(1, out, strlen(out));

	/* Allocate shared memory for our child processes. */
	a = GetSharedArray("qsort", file, 'A', &qsort_id);
	x = GetSharedArray("merge_x", file, 'F', &merge_id_x);
	y = GetSharedArray("merge_y", file, 'P', &merge_id_y);

	if (a != NULL && x != NULL && y != NULL)
	{
		merged = AllocSharedMemory("merge_out", (x[0] + y[0]) * sizeof(int), 'X', &merge_id_out);

		if (merged != NULL)
		{
			sprintf(out, "Input array for qsort has %d elements:\n    ", a[0]);
			write(1, out, strlen(out));

			PrintArray(a);

			sprintf(out, "\nInput array x[] for merge has %d elements:\n    ", x[0]);
			write(1, out, strlen(out));

			PrintArray(x);

			sprintf(out, "\nInput array y[] for merge has %d elements:\n    ", y[0]);
			write(1, out, strlen(out));

			PrintArray(y);

			sprintf(out, "\n");
			write(1, out, strlen(out));

			sprintf(out, "*** MAIN: about to spawn the process for qsort.\n");
			write(1, out, strlen(out));

			/* Begin the qsort process, using 1 as the left bound because 0 is our array size. */
			BeginQSort(qsort_id, 1, a[0]);

			sprintf(out, "*** MAIN: about to spawn the process for merge\n");
			write(1, out, strlen(out));

			/* Begin the merge process. */
			BeginMerge(merge_id_x, merge_id_y, merge_id_out);

			/* Block until both the qsort and merge processes finish. */
			for (i = 0; i < 2; ++i)
			{
				int status;
				wait(&status);
			}

			sprintf(out, "*** MAIN: sorted array by qsort:\n    ");
			write(1, out, strlen(out));

			PrintArray(a);

			sprintf(out, "*** MAIN: merged array:\n    ");
			write(1, out, strlen(out));

			for (i = 0; i < x[0] + y[0]; ++i)
			{
				sprintf(out, "%d ", merged[i]);
				write(1, out, strlen(out));
			}

			sprintf(out, "\n\n");
			write(1, out, strlen(out));
		}
	}

	/* Detach all shared memory. */
	DetachMemory("qsort", a);
	DetachMemory("merge_x", x);
	DetachMemory("merge_y", y);
	DetachMemory("merge_out", merged);

	sprintf(out, "\n");
	write(1, out, strlen(out));

	/* Free all shared memory. */
	RemoveMemory("qsort", qsort_id);
	RemoveMemory("merge_x", merge_id_x);
	RemoveMemory("merge_y", merge_id_y);
	RemoveMemory("merge_out", merge_id_out);
}
