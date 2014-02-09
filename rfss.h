#ifndef RFSS_H
#define RFSS_H

#define STDIN 0
#define STDOUT 1
#define STDERR 2
#define SERVER 100
#define CLIENT 101

void rfss_help();
void rfss_myip();
void rfss_myport();
int rfss_register(char *serverip, int port);
//int connect(char *destip, int port);
void list();
int terminate(int conn);
void rfss_exit();
int upload(int conn, char *filename);
int download(/*to be decided*/);
void creator();
int setup_listener(int port);
void process_command();
void process_ui_command();
void process_remote_command(int);
void accept_client();
void set_mode(int);
void extract_local_ip();
void rfss_list();

#endif
