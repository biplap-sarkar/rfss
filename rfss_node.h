/*
 * rfss_node.h : Data structure to represent a connection node.
 * Created for CSE 589 Spring 2014 Programming Assignment 1
 * 
 * @Author : Biplap Sarkar (biplapsa@buffalo.edu)
 *
 */ 
#ifndef RFSS_NODE_H
#define RFSS_NODE_H
#include <time.h>

/* Used to keep all information associated with a connection */
struct rfss_node{
	char *hostname;		// Hostname associated with the connection.
	char *ip;			// IP of the host associated with the connection.
	int port;			// Port at which connection is established.
	int sockd;			// Socket descriptor associated with the connection.
	int status;			// Status of the connection.
						// Possible states are COMMAND and DATA.
						// COMMAND state signifies that this connection is
						// currently used for transfer of commands between the nodes.
						// DATA state signifies that this cunnection is
						// currently used for transfer of file data.
						
	int ft_count;		// Count of file transfers pending with this connection.
	struct ft_stat *ft_list;	// List of pending file downloads.
	struct rfss_node *next;		// Pointer to next node in connection list
};


#endif
