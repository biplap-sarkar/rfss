#ifndef RFSS_NODE_H
#define RFSS_NODE_H

struct rfss_node{
	char *hostname;
	char *ip;
	int port;
	int sockd;
	struct rfss_node *next;
};
/*
struct rfss_node_list{
	struct rfss_node *node;
	struct rfss_node_list *next;
};*/

#endif
