/*
 * ft_stat.h : Structure to keep information about a single download
 * 
 * Created for CSE 589 Spring 2014 Programming Assignment 1
 * 
 * @Author : Biplap Sarkar (biplapsa@buffalo.edu)
 *
 */
 
#ifndef FT_STAT_H
#define FT_STAT_H
#include <time.h>

/*
 * Structure to keep information about a single download
 */
struct ft_stat {
	char *file;				// Name of file to be downloaded
	int fd;					// File descriptor
	int bytes_received;		// Bytes receieved till now
	int bytes_total;		// Total bytes to be received	
	struct timespec starttime;	// Start time of download
	struct timespec endtime;	// End time of download
	struct ft_stat *next;		// Pointer to next download in the connection
};

#endif


	
