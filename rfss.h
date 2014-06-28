/*
 * rfss.c : Contains headers for functions to start the Remote File Sharing System
 * Server and provide the UI of the application multiplexed with socket
 * connections.
 * 
 * Created for CSE 589 Spring 2014 Programming Assignment 1
 * 
 * @Author : Biplap Sarkar (biplapsa@buffalo.edu)
 *
 */

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
#define MAX_CONNECTED_NODES 4

extern struct rfss_node *connected_node_list ;	// list of connected nodes
extern struct rfss_node *global_node_list ;		// list of all client nodes in network
extern fd_set fds;				// FD Set used in select
extern int maxfd;				// max file descriptor used in select
extern int downloadqcount;		// count of connections where download is active
extern int connected_node_count;	// count of connected nodes
extern char myhostname[BUFFLEN];	// buffer for message passing

/*
 * Displays help information
 */
void rfss_display_help();

/*
 * Displays IP of the machine running this program
 */
void rfss_display_ip();

/*
 * Displays the port at which this program is listening for incoming
 * connections.
 */
void rfss_display_port();

/*
 * Insert a new socket in socket list
 * 
 * @arg socketd : socket to be inserted in the list
 * @return : void
 */
void add_into_socketlist(int socketd);

/*
 * Implements REGISTER command from UI
 * 
 * @arg serverip: IP of server
 * @arg serverport : port at which server is listening for connections.
 * 
 * @return 0 in case of success, -1 otherwise
 */
int rfss_register(char *serverip, int port);

/*
 * Implements the EXIT command from the UI
 */
void rfss_exit();

/*
 * Displays the information about the developer of this application.
 */
void rfss_display_creator();

/*
 * This function starts listening for incoming connections in the port 
 * specified.
 * 
 * @arg _port: port at which the socket will listen to incoming connections
 * @return: 0 in case of success, terminates the process after displaying
 * error message in case of failure
 */
int setup_listener(int port);

/*
 * This function multiplexes the UI commands with the incoming socket
 * commands using select and invokes the relevent processor function
 */
void rfss_process_command();

/*
 * This function reads commands from UI and invokes the relevent function
 * to process it
 */
void rfss_process_ui_command();

/*
 * Implements CONNECT command from UI
 * 
 * @arg dest: Destination address, can be either IP or hostname
 * @arg port: Port at which connection has to be established
 * @return : 0 in case of success, -1 otherwise
 */
int rfss_connect(char *dest, int port);

/*
 * Adds a new node in the list of global nodes
 * 
 * @nodeip : IP of the new node
 * @hostname : Hostname of the new node
 * @port : port at which new node is listening
 * 
 * @return : returns 0 in case of success, -1 otherwise
 */ 
int rfss_add_global_node(char *ip, char *hostname, int port);

/*
 * Removes a connection from connection list specified by con
 * 
 * @arg con: connection number to be removed
 * @return : 1 if connection is successfully removed, 0 otherwise
 */
int rfss_remove_connection_node(int con);

/*
 * Returns the connection id for socket sockd in the connection list
 * 
 * @arg sockd: socket descriptor in the connection list
 * @return : connection id, if sockd is valid, -1 otherwise
 */
int rfss_getcon(int sockd);

/*
 * Gets the connection node against a given connection id.
 * 
 * @arg con: connection id
 * @return : node against the connection id if it exists, NULL otherwise
 */
struct rfss_node * rfss_getnodebyconn(int con);
//int rfss_remove_connected_node(int clientsockd, char *ip);

/*
 * Removes an entry from the global list of nodes
 * 
 * @arg clientsockd: socket descriptor at which the node to be 
 * removed is connected
 * @arg nodeip: IP of the node to be removed
 * @return returns 1 if node is successfully removed, 0 otherwise
 */
int rfss_remove_global_node(int clientsockd, char *nodeip);
struct rfss_node * getnode(int connid);

/*
 * Initialises and initiates the download request to implement DOWNLOAD request
 * from UI
 * 
 * @arg arglist: download arguments (<connection id> <file name>)
 */
void rfss_init_download(char *arglist);
//int rfss_download(int connid, char *file);

/*
 * Prepares arguments for downloading a file from a particular connection.
 * 
 * @arg connid : connection id from which file is to be downloaded
 * @arg file : file which has to be downloaded
 * 
 * @return : 0 in case of success, -1 otherwise
 */
int rfss_prepare_download(int connid, char *file);

/*
 * Starts download for a given connection id by fetching the parameters
 * and changing the mode of the connection to DATA mode
 * 
 * @arg connid: connection id from which file is to be downloaded
 * @return 0 in case of successful download, -1 in case of failure
 */
int rfss_start_download(int connid);

/*
 * Implements UPLOAD command of the UI
 * 
 * @arg connid: connection id
 * @arg file: File to be uploaded
 */
void rfss_upload(int connid, char *file);

/* 
 * Implements TERMINATE command from the UI
 * 
 * @arg connection : Connection number which needs to be terminated
 * @return : 0 in case of success, -1 otherwise
 */
int rfss_terminate(int connection);

/*
 * Returns a node in global network list by hostname
 * 
 * @arg hostname: Hostname of the node
 * @return : A struct rfss_node type variable representing the node
 */
struct rfss_node * rfss_findglobalnodebyhostname(char *hostname);

/*
 * Returns a node in global network list by ip
 * 
 * @arg nodeip: IP of the node
 * @return : A struct rfss_node type variable representing the node
 */
struct rfss_node * rfss_findglobalnodebyip(char *nodeip);

/*
 * Adds a new connection node in the list of connected nodes
 * 
 * @arg clientsockd: socket descriptor at which new node is connected
 * @ip : IP of the new node
 * @hostname : Hostname of the new node
 * @port : port at which new node is listening
 * 
 * @return : returns 0 in case of success, -1 otherwise
 */
int rfss_add_connected_node(int clientsockd, char *ip, char *hostname, int port);

/*
 * Accepts a tcp connection from a remote client
 */
void accept_client();

/*
 * Sets the mode at which this process is operating. Two possible modes
 * are CLIENT and SERVER
 * 
 * @arg modearg: mode of the application
 */
void set_mode(int);

/*
 * Implements LIST command of the UI
 * Displays the list of connected nodes
 */
void rfss_list();

/*
 * Broadcast message to all the connected nodes.
 * Uses by server to notify connected nodes about network updation.
 * 
 * @arg msg: message to be broadcasted
 */
void broadcast_msg(char *msg);


/*
 * Sends message to a particular node
 * 
 * @arg ip : IP of the remote machine
 * @arg port : port of the remote machine
 * @arg msg : message to be sent
 */
void send_msg(char *ip, int port, char *msg);

/*
 * Returns the number of connections this node is connected to
 * 
 * @return : connection count
 */
int getconnectioncount();

/*
 * Finds the hostname of a node from it's IP
 * 
 * @arg ip: IP of the node
 * @arg hostname: Argument to keep the hostname. It is assumed that
 * the caller function has already allocated memory for it.
 * 
 * @return: 0 in case of success, -1 otherwise
 */
int gethostnamebyip(char *ip, char **hostname);

/*
 * Returns the mode at which this application is operating.
 * Can be either CLIENT or SERVER.
 * 
 * @return mode
 */
int getmode();


/*
 * This function determines the IP of the machine running this program
 * by making connection to Google's public DNS
 * 
 * @return: returns 0 in case of success, terminates the process after
 * displaying error message in case of failure
 */
int myiplookup();

#endif
