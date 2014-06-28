/*
 * file_utils.c : Contains functions to deal with files on the
 * local system.
 * Created for CSE 589 Spring 2014 Programming Assignment 1
 * 
 * @Author : Biplap Sarkar (biplapsa@buffalo.edu)
 *
 */ 

#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include "rfss.h"
#include "remote_command_processor.h"
#include "file_utils.h"

/* Buffer for file transfer */
static char filebuff[FILEBUFFLEN];

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
void fu_basename(char *path, char **base){
	char *ptr = strrchr(path, '/');
	if(ptr)
		strcpy(*base, ptr+1);
	else
		strcpy(*base, path);
}

/*
 * Returns the size of a file.
 * @arg path: relative or absolute path of the file
 * @return: file size of file in bytes if successful
 * or -1 in case of error
 */ 
int fu_getfilesize(char *path){
	struct stat st;
	int res = stat(path, &st);
	if(res == 0)
		return st.st_size;
	return res;
}

/*
 * Receives file from socket and writes into file
 * specified by filename.
 * @arg sockd : socket from which data is to be read
 * @arg filename : name of the file where data is to be written
 * @arg len : lenght of data in bytes
 * @return : returns 0 in case of success, -1 in case of error
 */ 
int fu_recvfile(int sockd, char *filename, int len){
	int fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	if(fd<0){
		printf("Error: File %s could not be created, make sure you have sufficient permission to create it\nrfss>");
		return -1;
	}
	int bytestorecv = len;
	int bytesread;
	int byteswritten;
	while(bytestorecv){
		if(bytestorecv > FILEBUFFLEN)
			bytesread = recv(sockd, filebuff, FILEBUFFLEN, 0);
		else
			bytesread = recv(sockd, filebuff, bytestorecv, 0);
		if(bytesread < 0){
			printf("Error: Communication error while receiving the file\nrfss>");
			close(fd);
			return -1;
		}
		byteswritten = write(fd, filebuff, bytesread);
		if(byteswritten < bytesread){
			printf("Error: Could not write the file, please make sure there is enough space\nrfss>");
			close(fd);
			return -1;
		} 
		bytestorecv = bytestorecv - byteswritten;
	}
	close(fd);
	return 0;
}

/*
 * Writes data into socket descriptor from file.
 * @arg sockd : socket descriptor where data is to be written
 * @arg path : file from which data is to be written
 * @arg len : length of data in bytes
 * @return : returns 0 in case of success, -1 in case of error
 */ 
int fu_sendfile(int sockd, char *path, int len){
	int fd = open(path, O_RDONLY);
	if(fd < 0){
		printf("Error: Could not open file %s. Make sure you have priviledges to read it\nrfss>",path);
		return -1;
	}
	int bytestosend = len;
	int bytessent;
	int bytesread;
	while(bytestosend){
		if(bytestosend > FILEBUFFLEN)
			bytesread = read(fd,filebuff,FILEBUFFLEN);
		else
			bytesread = read(fd,filebuff,bytestosend);
		bytessent = send(sockd,filebuff,bytesread,0);
		if(bytessent < 0){
			printf("Error: could not send the file due to connection problem\nrfss>");
			close(fd);
			return -1;
		}
		if(bytessent < bytesread){
			printf("Error: could not send the file, try again\nrfss>");
			close(fd);
			return -1;
		}
		bytestosend = bytestosend = bytessent;
	}
	close(fd);
	return 0;
}
