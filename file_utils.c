#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include "rfss.h"
#include "remote_command_processor.h"
#include "file_utils.h"

static char filebuff[FILEBUFFLEN];
void fu_basename(char *path, char **base){
	char *ptr = strrchr(path, '/');
	if(ptr)
		strcpy(*base, path+1);
}

int fu_getfilesize(char *path){
	struct stat st;
	int res = stat(path, &st);
	if(res == 0)
		return st.st_size;
	return res;
}

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

int fu_sendfile(int sockd, char *path, int len){
	/*char *filename = (char *)malloc(sizeof(char)*BUFFLEN);
	memset(filename, 0, BUFFLEN);
	fu_basename(path, &filename);
	if(strlen(filename)==0){
		printf("Error: file %s cannot be uploaded, make sure it exists\nrfss>",path);
		return -1;
	}*/
	int fd = open(path, O_RDONLY);
	if(fd < 0){
		printf("Error: Could not open file %s. Make sure you have priviledges to read it\nrfss>",path);
		return -1;
	}
	/*char uploadheader[BUFFLEN];
	memset(uploadheader, 0, BUFFLEN);
	sprintf(uploadheader,"upload:%s:%d\n",filename,len);
	send(sockd, uploadheader, strlen(uploadheader), 0);*/
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
