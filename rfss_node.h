#ifndef RFSS_NODE_H
#define RFSS_NODE_H
#include <time.h>

struct rfss_node{
	char *hostname;
	char *ip;
	int port;
	int sockd;
	int status;
	int ft_count;
	struct ft_stat *ft_list;
	/*
	char *file;
	int bytes_received;
	int bytes_total;
	int fd;
	struct timespec starttime;
	struct timespec endtime;*/
	struct rfss_node *next;
};
/*
struct rfss_node_list{
	struct rfss_node *node;
	struct rfss_node_list *next;
};*/

#endif
