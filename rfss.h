#ifndef RFSS_H
#define RFSS_H

#define STDIN 0
#define STDOUT 1
#define STDERR 2
#define SERVER 100
#define CLIENT 101
#define BUFFLEN 1024
#define COMMAND 11
#define DATA 12

extern struct rfss_node *connected_node_list ;
extern struct rfss_node *global_node_list ;
extern fd_set fds;
extern int maxfd;
extern int downloadqcount;

void rfss_display_help();
void rfss_display_ip();
void rfss_display_port();

void add_into_socketlist(int socketd);

int rfss_register(char *serverip, int port);
//int connect(char *destip, int port);
void list();
int terminate(int conn);
void rfss_exit();
int upload(int conn, char *filename);
int download(/*to be decided*/);
void rfss_display_creator();
int setup_listener(int port);
void rfss_process_command();
void rfss_process_ui_command();
int rfss_connect(char *dest, int port);
int rfss_add_global_node(char *ip, char *hostname, int port);
int rfss_remove_connection_node(int con);
int rfss_getcon(int sockd);
struct rfss_node * rfss_getnodebyconn(int con);
//int rfss_remove_connected_node(int clientsockd, char *ip);
int rfss_remove_global_node(int clientsockd, char *nodeip);
struct rfss_node * getnode(int connid);
void rfss_init_download(char *arglist);
int rfss_download(int connid, char *file);
void rfss_upload(int connid, char *file);
int rfss_terminate(int connection);
struct rfss_node * rfss_findglobalnodebyhostname(char *hostname);
struct rfss_node * rfss_findglobalnodebyip(char *nodeip);
//struct rfss_node * rfss_findnodebysockd(int sockd);
int rfss_add_connected_node(int clientsockd, char *ip, char *hostname, int port);
//void process_remote_command(int);
void accept_client();
void set_mode(int);
void extract_local_ip();
void rfss_list();
void broadcast_msg(char *msg);
void send_msg(char *ip, int port, char *msg);
int getconnectioncount();
int gethostnamebyip(char *ip, char **hostname);
int getmode();
int myiplookup();

#endif
