/* ----------------------------------------------------------- */
/* NAME : Jason bricco                       User ID: jmbricco */
/* DUE DATE : 02/12/2020                                       */
/* PROGRAM ASSIGNMENT #1                                       */
/* FILE NAME : prog1.c                                         */
/* PROGRAM PURPOSE :                                           */
/* 	Runs four processes concurrently, each receiving an        */
/*	integer to use as input  for its function. Functions are   */
/*  (1) computing a fibonnaci number, (2) computing buffon's   */
/*	need probability, (3) integrating sine, and (4) 		   */
/*	approximating the value of e.            				   */
/* ----------------------------------------------------------- */

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

#define PI 3.14159265358979323846

/* ----------------------------------------------------------- */
/* FUNCTION  ComputeFib  									   */
/*     Computes the n-th fibonnaci number using recursion.     */
/* PARAMETER USAGE :                                           */
/*    n: the fibonnaci number to compute.                      */
/* FUNCTION CALLED :                                           */
/*    ComputeFib(long n)                                       */
/* ----------------------------------------------------------- */
static long ComputeFib(long n)
{
	/* n == 0, 1, or 2 are base cases. We can return a result. */
	if (n == 0)
		return 0;

	if (n == 1 || n == 2)
		return 1;

	/* Recursively compute the previous values to add. */
	return ComputeFib(n - 1) + ComputeFib(n - 2);
}

/* Helper: Returns a random value between 0 and 1. */
static double Random01()
{
	return rand() / (double)RAND_MAX;
}

/* ----------------------------------------------------------- */
/* FUNCTION  BuffonNeedle  									   */
/*     Computes the probability of needles crossing a line     */
/*     when r needles are thrown.						       */
/* PARAMETER USAGE :                                           */
/*    r: the number of needles to throw.                       */
/* FUNCTION CALLED :                                           */
/*    Random01()                                               */
/* ----------------------------------------------------------- */
static double BuffonNeedle(int r)
{
	double t = 0.0;
	int i;

	for (i = 0; i < r; ++i)
	{
		/* d: Distance from one tip of the needle to the 
		lower dividing line.
		a: the angle between the needle and dividing line. */
		double d = Random01();
		double a = Random01() * (PI * 2.0);

		/* Probability of the needle crossing. */
		double prob = d + sin(a);

		if (prob < 0.0 || prob > 1.0)
			t += 1.0;
	}

	return r != 0 ? t / r : 0.0;
}

/* ----------------------------------------------------------- */
/* FUNCTION  IntegrateSine  								   */
/*     Computes the integration of sine between 0 and pi.      */
/* PARAMETER USAGE :                                           */
/*    s: the number of points in the rectangle to check.       */
/* FUNCTION CALLED :                                           */
/*    Random01()                                               */
/* ----------------------------------------------------------- */
static double IntegrateSine(int s)
{
	int t = 0;
	int i;
	char buf[128];

	for (i = 0; i < s; ++i)
	{
		double a = Random01() * PI;
		double b = Random01();

		/* if b <= sin(a) then this point is under the sine curve. */
		if (b <= sin(a))
			t += 1;
	}

	sprintf(buf, "            Total Hit %d\n", t);
	write(1, buf, strlen(buf));

	return s != 0 ? ((double)t / s) * PI : 0.0;
}

/* ----------------------------------------------------------- */
/* FUNCTION  ApproxE  								           */
/*     Approximates the value of e.                            */
/* PARAMETER USAGE :                                           */
/*    m: the maximum exponent to be used.                      */
/* ----------------------------------------------------------- */
static void ApproxE(long unsigned int m)
{
	unsigned long int i;
	int count;
	char buf[128];

	/* Actual value of e. */
	double e = exp(1.0);

	for (i = 1; i <= m;)
	{
		/* Computes an approximation for e using the given exponent i. */
		double approx = pow(1.0 + (1.0 / i), (double)i);
		printf("FOR I: %18lu, %.25f\n", i, 1.0 + (1.0 / i));
		sprintf(buf, "     %18lu     %.15f     %.15f\n", i, approx, fabs(approx - e));
		write(1, buf, strlen(buf));

		if (i < 10) ++i;
		else if (i == 10) i = 16;
		else i = i * 2;
	}
}

/* ----------------------------------------------------------- */
/* FUNCTION  main  								   			   */
/*     Creates four child processes to run ComputeFib,         */
/*		BuffonNeedle, IntegrateSine, and ApproxE, using the    */
/*		provided command line arguments.                       */
/* PARAMETER USAGE :                                           */
/*    argc: the number of command line arguments.              */
/* 	  argv: the list of command line arguments.                */
/* FUNCTION CALLED :                                           */
/*    ComputeFib(long n)									   */
/*	  BuffonNeedle(int r)                                      */
/*    IntegrateSine(int s)                                     */
/*    ApproxE(long unsigned int m)                             */
/* ----------------------------------------------------------- */
int main(int argc, char** argv)
{
	pid_t id;
	int childWait;
	char buf[128];

	/* Convert command line arguments into integers. */
	long unsigned int m = atol(argv[1]);
	long n = atol(argv[2]);
	int r = atoi(argv[3]);
	int s = atoi(argv[4]);

	sprintf(buf, "Main Process Started\n");

	sprintf(buf, "Fibonacci Number %ld\n", n);
	write(1, buf, strlen(buf));

	sprintf(buf, "Buffon's Needle Iterations = %d\n", r);
	write(1, buf, strlen(buf));

	sprintf(buf, "Integration Iterations     = %d\n", s);
	write(1, buf, strlen(buf));

	sprintf(buf, "Approx. e Iterations       = %17lu\n", m);
	write(1, buf, strlen(buf));

	/* Begin fibonnaci child process. */
	if ((id = fork()) == 0)
	{
		long result;

		sprintf(buf, "      Fibonacci Process Started\n");
		write(1, buf, strlen(buf));

		sprintf(buf, "      Input Number %ld\n", n);
		write(1, buf, strlen(buf));

		result = ComputeFib(n);

		sprintf(buf, "      Fibonacci Number f(%ld) is %ld\n", n, result);
		write(1, buf, strlen(buf));

		sprintf(buf, "      Fibonacci Process Exits\n");
		write(1, buf, strlen(buf));

		_exit(0);
	}
	else
	{
		sprintf(buf, "Fibonacci Process Created\n");
		write(1, buf, strlen(buf));
	}

	/* Begin Buffon's needle child process. */
	if ((id = fork()) == 0)
	{
		double result;

		sprintf(buf, "         Buffon's Needle Process Started\n");
		write(1, buf, strlen(buf));

		sprintf(buf, "         Input Number %d\n", r);
		write(1, buf, strlen(buf));

		result = BuffonNeedle(r);

		sprintf(buf, "         Estimated Probability is %.5f\n", result);
		write(1, buf, strlen(buf));

		sprintf(buf, "         Buffon's Needle Process Exits\n");
		write(1, buf, strlen(buf));

		_exit(0);
	}
	else
	{
		sprintf(buf, "Buffon's Needle Process Created\n");
		write(1, buf, strlen(buf));
	}

	/* Begin integrate sine child process. */
	if ((id = fork()) == 0)
	{
		double result;

		sprintf(buf, "            Integration Process Started\n");
		write(1, buf, strlen(buf));

		sprintf(buf, "            Input Number %d\n", s);
		write(1, buf, strlen(buf));

		result = IntegrateSine(s);

		sprintf(buf, "            Estimated Area is %f\n", result);
		write(1, buf, strlen(buf));

		sprintf(buf, "            Integration Process Exits\n");
		write(1, buf, strlen(buf));

		_exit(0);
	}
	else
	{
		sprintf(buf, "Integration Process Created\n");
		write(1, buf, strlen(buf));
	}

	/* Begin approximate e child process. */
	if ((id = fork()) == 0)
	{
		double result;

		sprintf(buf, "   Approximation of e Process Started\n");
		write(1, buf, strlen(buf));

		sprintf(buf, "   Maximum of the Exponent %18lu\n", m);
		write(1, buf, strlen(buf));

		ApproxE(m);
		_exit(0);
	}
	else
	{
		sprintf(buf, "Approximation of e Process Created\n");
		write(1, buf, strlen(buf));
	}

	sprintf(buf, "Main Process Waits\n");
	write(1, buf, strlen(buf));

	/* Wait for four processes to finish (the four we started). */
	for (childWait = 0; childWait < 4; ++childWait)
	{
		int status;
		wait(&status);
	}

	sprintf(buf, "Main Process Exits\n");
	write(1, buf, strlen(buf));
}
