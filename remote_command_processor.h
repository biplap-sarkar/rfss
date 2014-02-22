#ifndef REMOTE_COMMAND_PROCESSOR_H
#define REMOTE_COMMAND_PROCESSOR_H
void rem_process_command(int sockd);
void rem_connect(int sockd, char *ip, char *hostname, int port);
void rem_register(int sockd, char *ip, char *hostname, int port);
void rem_terminate(int sockd, char *ip);
void rem_upload(int sockd, char *file, int len);
void rem_download(int sockd, char *file);
void rem_invalid_command(int sockd);
void rem_node_join(int sockd, char *ip, char *hostname, int port);
void rem_node_leave(int sockd, char *ip);
#endif
