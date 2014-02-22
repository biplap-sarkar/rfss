#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#define FILEBUFFLEN 4096

void fu_basename(char *path, char **base);
int fu_getfilesize(char *path);
int fu_recvfile(int sockd, char *filename, int len);
int fu_sendfile(int sockd, char *path, int len);


#endif
