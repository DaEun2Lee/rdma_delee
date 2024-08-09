#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

//This part records time
//struct timeval  tv;
//double start, end;

//FILE *fptr;
//double record_time_file(FILE *fptr, char *message);
//void execution_time(FILE *fptr, double start, double end, char *message);


double record_time_file(FILE *fptr, char *message)
{
	//This func reords time in file and returns measurement time.
	struct timeval tv;
	double c_time;

	//Get current time
	gettimeofday(&tv, NULL);
	c_time = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000;

	fprintf(fptr, "%s: %f sec\n", message, c_time);
	printf("%s: %f sec\n", message, c_time);

	return c_time;
}

void execution_time(FILE *fptr, double start, double end, char *message)
{

	double e_time = (end-start) / 1000;

        fprintf(fptr, "%s: %f sec\n", message, e_time);
        printf("%s: %f sec\n", message, e_time);
}

#endif // COMMON_H
