/*
 * file_utils.h : Contains headers for functions to deal with 
 * files on the local system.
 * Created for CSE 589 Spring 2014 Programming Assignment 1
 * 
 * @Author : Biplap Sarkar (biplapsa@buffalo.edu)
 *
 */ 

#ifndef FILE_UTILS_H
#define FILE_UTILS_H

/* Buffer for file transfer */
#define FILEBUFFLEN 4096

/* 
 * Finds the base name of the file and populates
 * into base argument populated by the caller.
 * This function does not allocate any new memory
 * to put the result and assumes the caller
 * has already allocated memory in base argument.
 * 
 * @arg path : relative or absolute path of the file
 * @arg base : used to store the basename of the file
 * 
 */
void fu_basename(char *path, char **base);

/*
 * Returns the size of a file.
 * @arg path: relative or absolute path of the file
 * @return: file size of file in bytes if successful
 * or -1 in case of error
 */
int fu_getfilesize(char *path);

/*
 * Receives file from socket and writes into file
 * specified by filename.
 * @arg sockd : socket from which data is to be read
 * @arg filename : name of the file where data is to be written
 * @arg len : lenght of data in bytes
 * @return : returns 0 in case of success, -1 in case of error
 */ 
int fu_recvfile(int sockd, char *filename, int len);


/*
 * Writes data into socket descriptor from file.
 * @arg sockd : socket descriptor where data is to be written
 * @arg path : file from which data is to be written
 * @arg len : length of data in bytes
 * @return : returns 0 in case of success, -1 in case of error
 */
int fu_sendfile(int sockd, char *path, int len);


#endif
