/*
 * remote_command_processor.h : Contains functions to respond to commands
 * sent by remote peers by sockets.
 * 
 * Created for CSE 589 Spring 2014 Programming Assignment 1
 * 
 * @Author : Biplap Sarkar (biplapsa@buffalo.edu)
 *
 */
#ifndef REMOTE_COMMAND_PROCESSOR_H
#define REMOTE_COMMAND_PROCESSOR_H

/*
 * This function reads raw data from remote clients via
 * sockets, parses them into commands and invokes specific
 * routine/function to handle those commands.
 * 
 * @arg sockd: Socket descriptor of the tcp connection
 * @return void
 */
void rem_process_command(int sockd);


/*
 * Connects with a node which requested with CONNECT command.
 * This function is available only to nodes started in client mode.
 * 
 * @arg sockd: Socket descriptor of the tcp connection.
 * @arg ip: IP of the remote node interested to connect.
 * @arg hostname: Hostname of the remote node interesed to connect.
 * @arg port: Port at which the connection is established.
 * @return void
 */
void rem_connect(int sockd, char *ip, char *hostname, int port);

/*
 * Registers a new node in the network. This function is available
 * only to server node.
 * 
 * @arg sockd: Socket descriptor for the tcp connection.
 * @arg ip: IP of the new node interested to register.
 * @arg hostname: hostname of the new node interested to register.
 * @arg port: Port at which new node is listening for connections.
 * @return void
 */
void rem_register(int sockd, char *ip, char *hostname, int port);

/*
 * Function to handle TERMINATE request from a connected client node.
 * This function is available to nodes running as clients only.
 * 
 * @arg sockd: Socket descriptor of the tcp connection.
 * @arg ip: IP of the remote node.
 * @return void
 */
void rem_terminate(int sockd, char *ip);

/* 
 * Function to handle UPLOAD command sent by client.
 * 
 * @arg sockd: Socket descriptor of the tcp connection.
 * @arg file: name of the file which is uploaded by remote client.
 * @arg len: lenght of the file in bytes
 * @return void
 */
void rem_upload(int sockd, char *file, int len);
//void rem_download(int sockd, char *file);

/*
 * Reads data from a socket descriptor and writes into
 * file associated with the connection in which data download task is pending.
 * 
 * @arg sockd: Socket descriptor of the tcp connection.
 * @return void
 */
void rem_download_data(int sockd);
void rem_invalid_command(int sockd);

/* 
 * Function to handle notification of a new node joining into network.
 * This notification is sent by the server node.
 * 
 * @arg sockd: Socket descriptor of the tcp connection with server
 * @arg ip: IP of the new node
 * @arg hostname: Hostname of the new node
 * @arg port: Port at which new node is listening for connections.
 * @return void
 */
void rem_node_join(int sockd, char *ip, char *hostname, int port);

/* 
 * Function to handle notification of a node leaving from network.
 * This notification is sent by the server node.
 * 
 * @arg ip: IP of the exiting node
 * @arg port: Port at which the exiting node was listening for connections.
 * @return void
 */
void rem_node_leave(char *ip, int port);

/*
 * Function to handle EXIT command from a node in network.
 * This function is available to server only.
 * 
 * @arg sockd: Socket descriptor of the exiting node.
 * @return void
 */
void rem_exit(int sockd);
#endif
