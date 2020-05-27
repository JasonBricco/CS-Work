/* ----------------------------------------------------------- */
/* NAME : Jason bricco                       User ID: jmbricco */
/* DUE DATE : 02/24/2020                                       */
/* PROGRAM ASSIGNMENT #2                                       */
/* FILE NAME : merge.c                                         */
/* PROGRAM PURPOSE :                                           */
/* Takes two input arrays via shared memory. It then spawns    */
/* a child process for each element of those arrays, in order  */
/* to place them into a sorted position in the shared memory   */
/* output array          				                       */
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
/* FUNCTION  main  									           */
/*     merges two input arrays from shared memory into an      */
/*     output array also in shared memory, in sorted order.    */
/* PARAMETER USAGE :                                           */
/*    argc: the amount of arguments in argv.                   */
/*    argv: an array of string arguments passed to the program.*/
/* FUNCTION CALLED :                                           */
/*    atoi, shmat, fork, sprintf, write, strlen, exit,         */
/*    getpid, strerror, shmdt, wait
/* ----------------------------------------------------------- */
int main(int argc, char** argv)
{
	char out[256];
	int* x, *y, *merged, *cur, *other;
	char* curName, *otherName;
	int countX, countY, otherCount, index;

	int m_id_x = atoi(argv[1]);
	int m_id_y = atoi(argv[2]);
	int m_id_out = atoi(argv[3]);

	/* Attach the input arrays in order to gain access to them. */
	x = (int*)shmat(m_id_x, NULL, 0);
	y = (int*)shmat(m_id_y, NULL, 0);

	countX = x[0];
	countY = y[0];

	++x;
	++y;

	/* Tracks information about which array we're currently working with.
	This makes it easier to reuse the same code for both input arrays. */
	cur = x;
	curName = "x";
	otherName = "y";
	other = y;
	otherCount = countY;
	index = 0;

	while (1)
	{
		/* Create a child process to place a single array element. */
		if (fork() == 0)
		{
			int l, r, k;

			/* Get access to the output array to write into. */
			merged = (int*)shmat(m_id_out, NULL, 0);

			if (merged == (void*)-1)
			{
				sprintf(out, "Failed to attach shared memory. Error: %s\n", strerror(errno));
				write(2, out, strlen(out));
				exit(-1);
			}

			sprintf(out, "      $$$ M-PROC(%d): handling %s[%d] = %d\n", getpid(), curName, index, cur[index]);
			write(1, out, strlen(out));

			/* This element is smaller than all elements in the other array, and so all elements of that array
			will come after this one. We can simply use the index we're at now. */
			if (cur[index] < other[0])
			{
				sprintf(out, "      $$$ M-PROC(%d): %s[%d] = %d is found to be smaller than %s[0] = %d\n", getpid(), curName, index, cur[index], otherName, other[0]);
				write(1, out, strlen(out));

      			sprintf(out, "      $$$ M-PROC(%d): about to write %s[%d] = %d into position %d of the output array\n", getpid(), curName, index, cur[index], index);
      			write(1, out, strlen(out));

				merged[index] = cur[index];
				shmdt(merged);
				exit(0);
			}

			/* This element is larger than all elements in the other array, and so all elements of that array
			will come before this one. We add our index to the count of that array. */
			if (cur[index] > other[otherCount - 1])
			{
				sprintf(out, "      $$$ M-PROC(%d): %s[%d] = %d is found to be larger than %s[%d] = %d\n", getpid(), curName, index, cur[index], otherName, otherCount - 1, other[otherCount - 1]);
				write(1, out, strlen(out));

      			sprintf(out, "      $$$ M-PROC(%d): about to write %s[%d] = %d into position %d of the output array\n", getpid(), curName, index, cur[index], index + otherCount);
      			write(1, out, strlen(out));

				merged[index + otherCount] = cur[index];
				shmdt(merged);
				exit(0);
			}

			l = 0;
			r = otherCount - 1;

			/* Perform a modified binary search to figure out where this element should go,
			since it is somewhere among elements of the other array. */
			while (l <= r) 
		    { 
		        k = (l + r) / 2;
		  
		        if (other[k] < cur[index])
					l = k + 1; 
		        else r = k - 1;
		    }

		    /* Figure out if this element should go to the right or left of the found location
		    by comparing with the value at location k. */
		    if (cur[index] < other[k])
		    {
		    	sprintf(out, "      $$$ M-PROC(%d): %s[%d] = %d is found between %s[%d] = %d and %s[%d] = %d\n", getpid(), curName, index, cur[index], otherName, k - 1, other[k - 1], otherName, k, other[k]);
		    	write(1, out, strlen(out));

		    	sprintf(out, "      $$$ M-PROC(%d): about to write %s[%d] = %d into position %d of the output array\n", getpid(), curName, index, cur[index], index + k);
		    	write(1, out, strlen(out));

				merged[index + k] = cur[index];
		    }
		    else 
		    {
		    	sprintf(out, "      $$$ M-PROC(%d): %s[%d] = %d is found between %s[%d] = %d and %s[%d] = %d\n", getpid(), curName, index, cur[index], otherName, k, other[k], otherName, k + 1, other[k + 1]);
		    	write(1, out, strlen(out));

		    	sprintf(out, "      $$$ M-PROC(%d): about to write %s[%d] = %d into position %d of the output array\n", getpid(), curName, index, cur[index], index + k + 1);
		    	write(1, out, strlen(out));

		    	merged[index + k + 1] = cur[index];
		    }
		   
		   	/* Once an element is assigned, this process no longer needs the memory
		   	and should detach it. */
		    shmdt(merged);
		    exit(0);
		}
		else
		{
			++index;

			if (cur == x)
			{
				if (index >= countX)
				{
					/* We reached the end of the first input array.
					Switch to the second. */
					cur = y;
					curName = "y";
					otherName = "x";
					other = x;
					otherCount = countX;
					index = 0;
				}
			}
			else
			{
				/* We reached the end of the second input array. Get out of here. */
				if (index >= countY)
					break;
			}
		}
	}

	/* Wait for every child process to finish placing its element before continuing.
	There's one child process for every item in each array. */
	for (index = 0; index < countX + countY; ++index)
	{
		int status;
		wait(&status);
	}

	/* We're done reading from the input arrays, so detach them. */
	shmdt(x);
	shmdt(y);
}
