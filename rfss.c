/*
 * rfss.c : Contains functions to start the Remote File Sharing System
 * Server and provide the UI of the application multiplexed with socket
 * connections.
 * 
 * Created for CSE 589 Spring 2014 Programming Assignment 1
 * 
 * @Author : Biplap Sarkar (biplapsa@buffalo.edu)
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <arpa/inet.h>
#include <string.h>
#include "ft_stat.h"
#include "rfss_node.h"
#include "file_utils.h"
#include "rfss.h"

static int sockd;
fd_set fds;
int maxfd = 0;
int downloadqcount = 0;
int connected_node_count = 0;
static struct sockaddr_in serv_addr;
static struct sockaddr_in client_addr[10];
static int client_count = 0;
static int mode;
static char ip[20];
char myhostname[BUFFLEN];
static int port;
struct rfss_node *connected_node_list = NULL;
struct rfss_node *global_node_list = NULL;

/* List of sockets used to keep track of descriptors to listen to*/
struct socklist {
	int sockd;
	struct socklist *next;
} *socketlist;

/*
 * Displays help information
 */
void rfss_display_help(){
	printf("Available options:-\n");
	printf("1.) HELP\nUsage:- HELP\nDescription:- Display information about the available user interface options\n");
	printf("2.) MYIP\nUsage:- MYIP\nDescription:- Display the IP address of this process.\n ");
	printf("3.) MYPORT\nUsage:- MYPORT\nDescription:- Display the port on which this process is listening for incoming connections\n");
	printf("4.) REGISTER\nUsage:- REGISTER <server IP> <port_no>\nDescription:- This command is used by the client to register itself with the server\n");
	printf("5.) CONNECT\nUsage:- CONNECT <destination> <port no>\nDescription:- This command establishes a new TCP connection to the specified <destination> at the specified < port no>. The <destination> can either be an IP address or a hostname\n");
	printf("6.) LIST\nUsage:- LIST\nDescription:- Display a numbered list of all the connections this process is part of.\n");
	printf("7.) TERMINATE\nUsage:- TERMINATE <connection id>\nDescription:- This command will terminate the connection listed under the specified number when LIST is used to display all connections\n");
	printf("8.) EXIT\nUsage:- EXIT\nDescription:- Close all connections and terminate the process.\n");
	printf("9.) UPLOAD\nUsage:- UPLOAD <connection id> <file name>\nDescription:- Uploads <file name> to remote peer connected bye connection number <connection id>\n");
	printf("10.) DOWNLOAD\nUsage:- DOWNLOAD <connection id 1> <file1> [[<connection id 2> <file2>] [<connection id 3> <file3>]...]\n Description:- Download <file> from remote peery specified by <connection id>\n");
	printf("11.) CREATOR\nUsage:- CREATOR\nDescription:- Display information about the creator\nrfss>");
}

/*
 * Displays IP of the machine running this program
 */
void rfss_display_ip(){
	printf("%s\nrfss>",ip);
}

/*
 * Displays the port at which this program is listening for incoming
 * connections.
 */
void rfss_display_port(){
	printf("%d\nrfss>",port);
}

/*
 * Displays the information about the developer of this application.
 */
void rfss_display_creator(){
	printf("Created By :- Biplap Sarkar\n");
	printf("UBIT Name :- biplapsa\n");
	printf("Email :- biplapsa@buffalo.edu\nrfss>");
}


/*
 * This function starts listening for incoming connections in the port 
 * specified.
 * 
 * @arg _port: port at which the socket will listen to incoming connections
 * @return: 0 in case of success, terminates the process after displaying
 * error message in case of failure
 */
int setup_listener(int _port){
	port = _port;
	if((sockd = socket(AF_INET, SOCK_STREAM, 0))<0){
		perror("Couldn't create socket to listen\n");
		exit(1);
	}
	memset((void *)&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	if (bind(sockd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		perror("Socket bind error\n");
		exit(1);
	}
	listen(sockd, 1);
	myiplookup();
	return 0;
}

/*
 * This function determines the IP of the machine running this program
 * by making connection to Google's public DNS
 * 
 * @return: returns 0 in case of success, terminates the process after
 * displaying error message in case of failure
 */
int myiplookup(){
	int clientsockd;
	struct sockaddr_in google_addr;
	struct sockaddr_in myaddr;
	memset(&google_addr,0,sizeof(struct sockaddr_in));
	memset(&myaddr,0,sizeof(struct sockaddr_in));
	google_addr.sin_family = AF_INET;
	google_addr.sin_port = htons(53);
	inet_pton(AF_INET,"8.8.8.8",&google_addr.sin_addr);
	if((clientsockd = socket(AF_INET, SOCK_STREAM, 0))<0){
		perror("myiplookup(): socket creation error\n");
		exit(1);
	}
	if(connect(clientsockd, (struct sockaddr *)&google_addr, sizeof(google_addr)) < 0){
		perror("myiplookup(): socket connect error");
		exit(1);
	}
	int socklen = sizeof(myaddr);
	if(getsockname(clientsockd, (struct sockaddr *)&myaddr, &socklen) < 0){
		perror("myiplookup(): socket error");
		exit(1);
	}
	inet_ntop(AF_INET, &(myaddr.sin_addr), ip, INET_ADDRSTRLEN);
	return 0;
}

/*
void extract_local_ip(){
	struct sockaddr_in address;
	socklen_t len;
	char host[20];
	memset(&address, 0, sizeof(struct sockaddr_in));
	len = sizeof(struct sockaddr_in);

	if (getsockname(sockd, (struct sockaddr*)&address, &len) == 0) {

		if(getnameinfo((const struct sockaddr*)&address, len,
				host, 20,
				NULL, 0, 0) == 0) {
			strcpy(ip,host);
		}
	}
	else
		strcpy(ip,"localhost");
}*/

/*
 * This function multiplexes the UI commands with the incoming socket
 * commands using select and invokes the relevent processor function
 */
void rfss_process_command(){
	int fdi;
	fd_set tfds;
	printf("rfss>");
	maxfd = sockd;
	socketlist = (struct socklist *)malloc(sizeof(struct socklist));
	socketlist->sockd = sockd;
	socketlist->next = NULL;
	FD_ZERO(&fds);
	FD_SET(STDIN, &fds);
	FD_SET(sockd, &fds);
	maxfd = sockd;
	while(1){
		FD_ZERO(&tfds);
		tfds = fds;
		select(maxfd+1, &tfds, NULL, NULL, NULL);
		if(FD_ISSET(STDIN, &tfds)){
			rfss_process_ui_command();
		}
		if(FD_ISSET(sockd, &tfds)){
			// connect request from a client
			accept_client();
		}
		for(fdi=0;fdi<=maxfd;fdi++){
			if(fdi==STDIN || fdi==sockd)
				continue;
			if(FD_ISSET(fdi, &tfds)){
				// handle command from a connected client
				rem_process_command(fdi);
			}
		}
	}
}

/*
 * Insert a new socket in socket list
 * 
 * @arg socketd : socket to be inserted in the list
 * @return : void
 */
void add_into_socketlist(int socketd){
	while(socketlist->next){
		socketlist = socketlist->next;
	}
	struct socklist *new = (struct socklist *)malloc(sizeof(struct socklist));
	new->sockd = socketd;
	new->next = NULL;
	socketlist->next = new;
}

/*
 * Accepts a tcp connection from a remote client
 */
void accept_client(){
	int clientlen = sizeof(client_addr[client_count]);
	int clientsoc = accept(sockd, (struct sockaddr *)&client_addr[client_count], &clientlen);
	FD_SET(clientsoc, &fds);
	if(clientsoc > maxfd)
		maxfd = clientsoc;
	add_into_socketlist(clientsoc);
	clientsoc = clientsoc + 1;
}

/*
 * This function reads commands from UI and invokes the relevent function
 * to process it
 */
void rfss_process_ui_command(){
	char cmdstr[100];
	char cmd[100];
	fgets(cmdstr, 99, stdin);
	int len = strlen(cmdstr);
	cmdstr[len-1] = '\0';
	strcpy(cmd, cmdstr);
	char *command = strtok(cmdstr," ");
	if(command == NULL){
		printf("rfss>");
		return;
	}
	if(strcasecmp(command,"help")==0){
		if(strtok(NULL," ")!=NULL){
			printf("Error: Command %s does not take any parameter\nrfss>",command );
		}
		else
			rfss_display_help();
	}
	else if(strcasecmp(command,"myip")==0){
		if(strtok(NULL," ")!=NULL){
			printf("Error: Command %s does not take any parameter\nrfss>",command);
		}
		else
			rfss_display_ip();
	}
	else if(strcasecmp(command,"myport")==0){
		if(strtok(NULL," ")!=NULL){
			printf("Error: Command %s does not take any parameter\nrfss>",command);
		}
		else
			rfss_display_port();
	}
	else if(strcasecmp(command,"creator")==0){
		if(strtok(NULL," ")!=NULL){
			printf("Error: Command %s does not take any parameter\nrfss>",command);
		}
		else
			rfss_display_creator();
	}
	else if(strcasecmp(command,"exit")==0){
		if(strtok(NULL," ")!=NULL){
			printf("Error: Command %s does not take any parameter\nrfss>",command);
		}
		else
			rfss_exit();
	}
	else if(strcasecmp(command,"connect")==0){
		if(mode == SERVER){
			printf("Your are running in server mode and CONNECT command is not valid in this mode\nrfss>");
			return;
		}
		char *dest = strtok(NULL," ");
		if(dest == NULL){
			printf("Error: Invalid connect command. Usage: connect <destination> <serverport>\nrfss>");
			return;
		}
		char *serverport = strtok(NULL, " ");
		if(serverport == NULL){
			printf("Error: Invalid connect command. Usage: connect <destination> <serverport>\nrfss>");
			return;
		}
		char *endptr;
		int porti = (int) strtol(serverport, &endptr, 10);
		if(*endptr != 0){
			printf("Error: Invalid connect command. Invalid port.\nrfss>");
			return;
		}
		if(rfss_connect(dest, porti)<0){
			printf("Error: Connect failed\nrfss>");
			return;
		}
		printf("Sucessfully connected with the %s\nrfss>",dest);
	}
	else if(strcasecmp(command,"register")==0){
		if(mode==SERVER){
			printf("Your are running in server mode and REGISTER command is not valid in this mode\nrfss>");
			return;
		}
		char *serverip = strtok(NULL," ");
		if(serverip == NULL){
			printf("Error: Invalid register command. Usage: register <serverip> <serverport>\nrfss>");
			return;
		}
		char *serverport = strtok(NULL, " ");
		if(serverport == NULL){
			printf("Error: Invalid register command. Usage: register <serverip> <serverport>\nrfss>");
			return;
		}
		char *endptr;
		int porti = (int) strtol(serverport, &endptr, 10);
		if(*endptr != 0){
			printf("Error: Invalid register command. Invalid port.\nrfss>");
			return;
		}
		if(rfss_register(serverip, porti)<0){
			printf("Error: Registeration failed\nrfss>");
			return;
		}
		printf("Sucessfully registered with the server\nrfss>");
	}
	else if(strcasecmp(command,"terminate")==0){
		if(mode==SERVER){
			printf("Your are running in server mode and TERMINATE command is not valid in this mode\nrfss>");
			return;
		}
		else{
			char *connection = strtok(NULL, " ");
			if(connection == NULL){
				printf("Error: Invalid terminate command. Usage: terminate <connection no>\nrfss>");
				return;
			}
			rfss_terminate(atoi(connection));
		}
	}
	else if(strcasecmp(command,"upload")==0){
		if(mode==SERVER){
			printf("Your are running in server mode and UPLOAD command is not valid in this mode\nrfss>");
			return;
		}
		char *connid = strtok(NULL," ");
		char *file = strtok(NULL," ");
		rfss_upload(atoi(connid), file);
	}
	else if(strcasecmp(command,"download")==0){
		if(mode==SERVER){
			printf("Your are running in server mode and DOWNLOAD command is not valid in this mode\nrfss>");
			return;
		}
		int p = strlen("download");
		while(cmd[p]==' ')
			p++;
		//char *args = strtok(NULL," ");
		rfss_init_download((char *)(cmd)+p);
	}
	else if(strcasecmp(command,"list")==0){
		rfss_list();
	}
	else
		printf("Invalid command!!! Use help command to get a list of available commands\nrfss>");
}

/*
 * Implements the EXIT command from the UI
 */
void rfss_exit(){
	if(mode == CLIENT){
		char exitreq[BUFFLEN];
		memset(exitreq, 0, BUFFLEN);
		sprintf(exitreq,"exit");
		if(connected_node_count>0){
			struct rfss_node *servernd = rfss_getnodebyconn(1);
			send(servernd->sockd, exitreq, strlen(exitreq), 0); 
		}
	}
	exit(0);
}
/*
void process_remote_command(int clientsockd){
	char cmdstr[100];
	char cmd[100];
	char remip[50];
	char remport[20];
	memset(cmdstr,0,100);
	memset(cmd,0,100);
	recv(clientsockd, cmdstr, 100, 0);
	strcpy(cmd,cmdstr);
	char *command = strtok(cmdstr, " ");
	if(strcasecmp(command, "register")==0){
		if(mode==CLIENT){
			memset(cmdstr, 0 , 100);
			strcpy(cmdstr, "Error: I am not the server");
			send(clientsockd, cmdstr, strlen(cmdstr), 0);
			close(clientsockd);
			return;
		}
		char *_remip = strtok(NULL, " ");
		if(_remip==NULL){
			printf("process_remote_command(): Invalid register command\nrfss>");
			return;
		}
		strcpy(remip,_remip);
		char *_remport = strtok(NULL, " ");
		if(_remport == NULL){
			printf("process_remote_command(): Invalid register command\nrfss>");
			return;
		}
		strcpy(remport,_remport);
		memset(cmdstr, 0, 100);
		char con_notification[100];
		memset(con_notification,0,100);
		char *hostname = (char *)malloc(sizeof(char)*100);
		gethostnamebyip(remip, &hostname);
		sprintf(con_notification,"Add:%s:%s:%d",remip,hostname,remport);
		broadcast_msg(con_notification);
		rfss_add_node(clientsockd, remip, hostname, atoi(remport));
		free(hostname);
		strcpy(cmdstr,"register ok");
		send(clientsockd, cmdstr, strlen(cmdstr), 0);
		//close(clientsockd);
		printf("New peer with ip %s registered\nrfss>", remip);
	}
	else{
		//close(clientsockd);
		printf("%s\nrfss>", cmd);
	}
}
*/

/*
 * Returns the connection id for socket sockd in the connection list
 * 
 * @arg sockd: socket descriptor in the connection list
 * @return : connection id, if sockd is valid, -1 otherwise
 */
int rfss_getcon(int sockd){
	int con = 1;
	struct rfss_node *ptr = connected_node_list;
	while(ptr){
		if(ptr->sockd == sockd)
			return con;
		ptr = ptr->next;
		con++;
	}
	return -1;
}

/*
 * Removes a connection from connection list specified by con
 * 
 * @arg con: connection number to be removed
 * @return : 1 if connection is successfully removed, 0 otherwise
 */
int rfss_remove_connection_node(int con) {
	if(con > connected_node_count)
		return 0;
	if(connected_node_count == 0)
		return 0;
	struct rfss_node *tmp = NULL;
	struct rfss_node *ptr = connected_node_list;
	int count=1;
	if(con == 1){
		tmp = connected_node_list;
		connected_node_list = connected_node_list->next;
		free(tmp->ip);
		free(tmp->hostname);
		free(tmp);
		connected_node_count--;
		return 1;
	}
	while(count < con-1){
		ptr = ptr->next;
		count++;
	}
	tmp = ptr->next;
	ptr->next = tmp->next;
	free(tmp->ip);
	free(tmp->hostname);
	free(tmp);
	connected_node_count--;
	return 1;
}
/*
int rfss_remove_connected_node(int clientsockd, char *ip){
	struct rfss_node *ptr = connected_node_list;
	struct rfss_node *tmp;
	if(strcmp(ptr->ip,ip)==0){
		connected_node_list = ptr->next;
		close(ptr->sockd);
		free(ptr);
		return 0;
	}
	while(ptr->next){
		if(strcmp(ptr->next->ip,ip)==0){
			tmp = ptr->next;
			ptr->next = tmp->next;
			free(tmp);
			connected_node_count--;
			return 0;
		}
		ptr = ptr->next;
	}
	return 1;
}*/

/*
 * Removes an entry from the global list of nodes
 * 
 * @arg clientsockd: socket descriptor at which the node to be 
 * removed is connected
 * @arg nodeip: IP of the node to be removed
 * @return returns 1 if node is successfully removed, 0 otherwise
 */
int rfss_remove_global_node(int clientsockd, char *nodeip){
	int connection = rfss_getcon(clientsockd);
	rfss_remove_connection_node(connection);
	//rfss_remove_connected_node(clientsockd, nodeip);
	struct rfss_node *ptr = connected_node_list;
	struct rfss_node *tmp;
	if(strcmp(ptr->ip,ip)==0){
		connected_node_list = ptr->next;
		free(ptr);
		return 0;
	}
	while(ptr->next){
		if(strcmp(ptr->next->ip,nodeip)==0){
			tmp = ptr->next;
			ptr->next = tmp->next;
			free(tmp);
			return 0;
		}
		ptr = ptr->next;
	}
	return 1;
}

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
int rfss_add_connected_node(int clientsockd, char *ip, char *hostname, int port){
	struct rfss_node *new_node = (struct rfss_node *)malloc(sizeof(struct rfss_node));
	new_node->ip = (char *)malloc(sizeof(char)*20);
	new_node->hostname = (char *)malloc(sizeof(char)*100);
	memset(new_node->ip, 0, 20);
	strcpy(new_node->ip, ip);
	new_node->port = port;
	new_node->sockd = clientsockd;
	new_node->status = COMMAND;
	strcpy(new_node->hostname, hostname);
	new_node->next = NULL;
	if(connected_node_list == NULL)
		connected_node_list = new_node;
	else{
		struct rfss_node *ptr;
		for(ptr=connected_node_list; ptr->next; ptr=ptr->next){
			continue;
		}
		ptr->next = new_node;
	}
	connected_node_count++;
	return 0;
}

/* 
 * Implements TERMINATE command from the UI
 * 
 * @arg connection : Connection number which needs to be terminated
 * @return : 0 in case of success, -1 otherwise
 */
int rfss_terminate(int connection){
	int count = 0;
	char terminatereq[BUFFLEN];
	memset(terminatereq, 0 , BUFFLEN);
	if(connection > connected_node_count){
		printf("rfss_terminate(): Invalid connection\nrfss>");
		return -1;
	}
	if(connection == 1){
		printf("rfss_terminate(): Cannot issue terminate request to server\nrfss>");
		return -1;
	}
	struct rfss_node *nd = rfss_getnodebyconn(connection);
	if(nd == NULL){
		printf("rfss_terminate(): Invalid mapping\nrfss>");
		return -1;
	}
	sprintf(terminatereq,"terminate:%s",ip);
	send(nd->sockd, terminatereq, strlen(terminatereq), 0);
	printf("Connection with %s terminated\nrfss>",nd->hostname);
	FD_CLR(nd->sockd, &fds);
	close(nd->sockd);
	rfss_remove_connection_node(connection);
	return 0;
}

/*
 * Returns a node in global network list by ip
 * 
 * @arg nodeip: IP of the node
 * @return : A struct rfss_node type variable representing the node
 */
struct rfss_node * rfss_findglobalnodebyip(char *nodeip){
	struct rfss_node *ptr = global_node_list;
	while(ptr){
		if(strcmp(ptr->ip,nodeip)==0)
			return ptr;
		ptr = ptr->next;
	}
	return NULL;
}

/*
 * Returns a node in global network list by hostname
 * 
 * @arg hostname: Hostname of the node
 * @return : A struct rfss_node type variable representing the node
 */
struct rfss_node * rfss_findglobalnodebyhostname(char *hostname){
	struct rfss_node *ptr = global_node_list;
	while(ptr){
		if(strcmp(ptr->hostname,hostname)==0)
			return ptr;
		ptr = ptr->next;
	}
	return NULL;
}

/*
 * Adds a new node in the list of global nodes
 * 
 * @nodeip : IP of the new node
 * @hostname : Hostname of the new node
 * @port : port at which new node is listening
 * 
 * @return : returns 0 in case of success, -1 otherwise
 */ 
int rfss_add_global_node(char *nodeip, char *hostname, int port){
	struct rfss_node *new_node = (struct rfss_node *)malloc(sizeof(struct rfss_node));
	new_node->ip = (char *)malloc(sizeof(char)*20);
	new_node->hostname = (char *)malloc(sizeof(char)*100);
	memset(new_node->ip, 0, 20);
	strcpy(new_node->ip, nodeip);
	new_node->port = port;
	new_node->sockd = 0;
	strcpy(new_node->hostname, hostname);
	new_node->next = NULL;
	if(global_node_list == NULL)
		global_node_list = new_node;
	else{
		struct rfss_node *ptr;
		for(ptr=global_node_list; ptr->next; ptr=ptr->next){
			continue;
		}
		ptr->next = new_node;
	}
	return 0;
}

/*
 * Finds the hostname of a node from it's IP
 * 
 * @arg ip: IP of the node
 * @arg hostname: Argument to keep the hostname. It is assumed that
 * the caller function has already allocated memory for it.
 * 
 * @return: 0 in case of success, -1 otherwise
 */
int gethostnamebyip(char *ip, char **hostname){
	struct sockaddr_in address;
	char buf[100];
	memset(buf,0,100);
	inet_pton(AF_INET,ip,&address.sin_addr);
	address.sin_family = AF_INET;
	
	int res = getnameinfo((struct sockaddr *)&address, sizeof(address), buf, sizeof(buf), NULL, 0, 0);
	if(res == 0)
		strcpy(*hostname, buf);
	else
		printf("%s",gai_strerror(res));
	return res;
}

/*
 * Sets the mode at which this process is operating. Two possible modes
 * are CLIENT and SERVER
 * 
 * @arg modearg: mode of the application
 */
void set_mode(int modearg){
	mode = modearg;
}

/*
 * Returns a node from the list of connected nodes for a connection id
 * 
 * @arg connid: connection id
 * @return: Connection node against the id
 */
struct rfss_node * getnode(int connid){
	if(connid <= 0 || connid > connected_node_count)
		return NULL;
	struct rfss_node *ptr = connected_node_list;
	int count = 1;
	while(count < connid){
		ptr = ptr->next;
		count++;
	}
	return ptr;
}

/*
 * Initialises and initiates the download request to implement DOWNLOAD request
 * from UI
 * 
 * @arg arglist: download arguments (<connection id> <file name>)
 */
void rfss_init_download(char *arglist){
	char args[BUFFLEN];
	memset(args,0,BUFFLEN);
	strcpy(args,arglist);
	char *saveptr = (char *)malloc(sizeof(char)*BUFFLEN);
	memset(saveptr,0,BUFFLEN);
	int connid;
	char *_connid;
	char *file;
	_connid = strtok_r(args," ",&saveptr);
	connid = atoi(_connid);
	file = strtok_r(NULL," ",&saveptr);
	if(rfss_prepare_download(connid, file)<0){
		printf("Error initiating download\nrfss>");
		return;
	}
	while(1){
		_connid = strtok_r(NULL," ",&saveptr);
		if(_connid == NULL)
			break;
		connid = atoi(_connid);
		file = strtok_r(NULL," ",&saveptr);
		if(file == NULL)
			break;
		if(rfss_prepare_download(connid, file)<0){
			printf("Error initiating download\nrfss>");
			return;
		}
	}
	int i=0;
	for(i=2;i<=connected_node_count;i++){
		rfss_start_download(i);
	}
}

/*
 * Starts download for a given connection id by fetching the parameters
 * and changing the mode of the connection to DATA mode
 * 
 * @arg connid: connection id from which file is to be downloaded
 * @return 0 in case of successful download, -1 in case of failure
 */
int rfss_start_download(int connid){
	char downloadreq[BUFFLEN];
	char *saveptr = (char *)malloc(sizeof(char));
	memset(downloadreq,0,BUFFLEN);
	struct rfss_node *nd = rfss_getnodebyconn(connid);
	if(nd==NULL){
		printf("Error: No active nodes associated with connection %d\n",connid);
		return -1;
	}
	if(nd->ft_list == NULL)
		return 0;
	strcat(downloadreq,"download");
	struct ft_stat *ptr = nd->ft_list;
	while(ptr){
		strcat(downloadreq,":");
		strcat(downloadreq,ptr->file);
		ptr = ptr->next;
	}
	send(nd->sockd, downloadreq, strlen(downloadreq), 0);
	memset(downloadreq, 0, BUFFLEN);
	recv(nd->sockd, downloadreq, BUFFLEN, 0);
	char *resp = strtok_r(downloadreq,":",&saveptr);
	char *_filesize;
	if(strcasecmp(resp,"downloadack")==0){
		ptr = nd->ft_list;
		while(ptr){
			_filesize = strtok_r(NULL,":",&saveptr);
			if(_filesize == NULL){
				printf("Error: argument mismatch with server\nrfss>");
				return -1;
			}
			char *basename = (char *)malloc(sizeof(char)*BUFFLEN);
			memset(basename,0,BUFFLEN);
			fu_basename(ptr->file, &basename);
			strcpy(ptr->file, basename);
			free(basename);
			ptr->bytes_total = atoi(_filesize);
			ptr = ptr->next;
		}
		send(nd->sockd,"downloadack",strlen("downloadack"),0);
		nd->status = DATA;
		downloadqcount++;
	}
	else{
		resp = strtok_r(NULL,":",&saveptr);
		printf("Error: Response from server is %s\nrfss>",resp);
		nd->ft_list = NULL;
		return -1;
	}
	return 0;
}

/*
 * Prepares arguments for downloading a file from a particular connection.
 * 
 * @arg connid : connection id from which file is to be downloaded
 * @arg file : file which has to be downloaded
 * 
 * @return : 0 in case of success, -1 otherwise
 */
int rfss_prepare_download(int connid, char *file){
	if(connid < 2){
		printf("Error: cannot download from connection %d\n",connid);
		return -1;
	}
	struct rfss_node *nd = rfss_getnodebyconn(connid);
	if(nd == NULL){
		printf("Error: No active nodes associated with connection %d\n",connid);
		return -1;
	}
	/*
	int filesize = fu_getfilesize(file);
	if(filesize < 0){
		printf("Error: Could not get details of file %s, make sure it exists",file);
		return -1;
	}*/
	struct ft_stat *stat = (struct ft_stat *)malloc(sizeof(struct ft_stat));
	stat->file = (char *)malloc(sizeof(char)*BUFFLEN);
	memset(stat->file, 0, BUFFLEN);
	strcpy(stat->file, file);
	stat->fd = -1;
	stat->bytes_total = 0;
	stat->bytes_received = 0;
	stat->next = NULL;
	
	if(nd->ft_list == NULL)
		nd->ft_list = stat;
	else{
		struct ft_stat *ptr = nd->ft_list;
		while(ptr->next){
			ptr = ptr->next;
		}
		ptr->next = stat;
	}
	printf("Download of file %s from %s initiated\n",file,nd->hostname);
	return 0;
}
/*
int rfss_download(int connid, char *file){
	char downloadreq[BUFFLEN];
	memset(downloadreq, 0, BUFFLEN);
	struct rfss_node *nd = getnode(connid);
	if(nd == NULL){
		printf("Error: Invalid connection number\nrfss>");
		return -1;
	}
	int sockd = nd->sockd;
	sprintf(downloadreq,"download:%s",file);
	send(sockd,downloadreq,strlen(downloadreq),0);
	memset(downloadreq,0,BUFFLEN);
	recv(sockd,downloadreq,BUFFLEN,0);
	char *downloadack = strtok(downloadreq,":");
	if(strcmp(downloadack,"downloadack")!=0){
		printf("Error: File %s cannot be downloaded from server %s\nrfss>",file,nd->hostname);
		return -1;
	}
	char *_filesize = strtok(NULL,":");
	int filesize = atoi(_filesize);
	memset(downloadreq,0,BUFFLEN);
	sprintf(downloadreq,"downloadack");
	send(sockd, downloadreq, strlen(downloadreq), 0);
	nd->file = (char *)malloc(sizeof(char)*BUFFLEN);
	memset(nd->file, 0, BUFFLEN);
	strcpy(nd->file, file);
	nd->status = DATA;
	nd->bytes_received = 0;
	nd->bytes_total = filesize;
	clock_gettime(CLOCK_REALTIME, &(nd->starttime));
	downloadqcount++;
	//fu_recvfile(sockd, file, filesize);
	printf("File %s download from server %s initiated\n",file,nd->hostname);
	return 0;
}
*/

/*
 * Implements UPLOAD command of the UI
 * 
 * @arg connid: connection id
 * @arg file: File to be uploaded
 */
void rfss_upload(int connid, char *file){
	if(connid == 1){
		printf("Error: Cannot upload to server\nrfss>");
		return;
	}
	if(connid < 1 || connid > connected_node_count){
		printf("Error: Invalid connection number\nrfss>");
	}
	char uploadreq[BUFFLEN];
	memset(uploadreq, 0, BUFFLEN);
	struct rfss_node *nd = getnode(connid);
	if(nd == NULL){
		printf("Error: Invalid connection number\nrfss>");
		return;
	}
	int sockd = nd->sockd;
	int filesize = fu_getfilesize(file);
	if(filesize < 0){
		printf("Error: Cannot upload file %s, make sure it exists\nrfss>",file);
		return;
	}
	char *basename = (char *)malloc(sizeof(char)*BUFFLEN);
	memset(basename, 0, BUFFLEN);
	fu_basename(file,&basename);
	sprintf(uploadreq,"upload:%s:%d",basename,filesize);
	free(basename);
	send(sockd, uploadreq, strlen(uploadreq), 0);
	memset(uploadreq, 0, BUFFLEN);
	recv(sockd, uploadreq, BUFFLEN, 0);
	if(strcmp(uploadreq,"uploadack")!=0){
		printf("Error: Cannot upload file. Remote peer not ready\nrfss>");
		return;
	}
	struct timespec starttime, endtime;
	clock_gettime(CLOCK_REALTIME, &starttime);
	int res = fu_sendfile(sockd, file, filesize);
	clock_gettime(CLOCK_REALTIME, &endtime);
	double timediff = (double)(endtime.tv_sec - starttime.tv_sec) +
						(double)(endtime.tv_nsec - starttime.tv_nsec)/1E9 ;
	double speed = (filesize * 8)/timediff;
	if(res == 0){
		printf("File %s successfully uploaded to %s\n",file,nd->hostname);
		printf("%s -> %s, File Size: %d bytes, Time Taken: %f seconds, Tx Rate: %.2fbits/second\nrfss>",myhostname,nd->hostname,filesize,timediff,speed);
	}
	else{
		printf("Error while uploading\nrfss>");
	}
}

/*
 * Implements LIST command of the UI
 * Displays the list of connected nodes
 */
void rfss_list(){
	if(connected_node_list == NULL){
		printf("No hosts connected\nrfss>");
		return;
	}
	struct rfss_node *ptr;
	printf("   id:\t\t\tHostname\t\tIP adddress\tPort No\n");
	int id;
	for(ptr=connected_node_list,id=1;ptr;ptr=ptr->next,id++){
		printf("%5d : %30s %20s %10d\n",id,ptr->hostname,ptr->ip,ptr->port);
	}
	printf("rfss>");
}

/*
 * Check if this node is connected with node having specified IP
 * @arg ip : IP of remote node
 * @return : 0 if this node is not connected with node having ip IP,
 * 1 if it is connected
 */
int is_connected(char *ip){
	struct rfss_node *nd = connected_node_list;
	while(nd){
		if(strcmp(nd->ip,ip)==0)
			return 1;
		nd = nd->next;
	}
	return 0;
}
/*
 * Implements CONNECT command from UI
 * 
 * @arg dest: Destination address, can be either IP or hostname
 * @arg port: Port at which connection has to be established
 * @return : 0 in case of success, -1 otherwise
 */
int rfss_connect(char *dest, int port){
	if(strcmp(dest,myhostname)==0 || strcmp(dest,ip)==0 || strcmp(dest,"127.0.0.1")==0
		|| strcmp(dest,"localhost")==0){
		printf("Self connections not allowed\nrfss>");
		return -1;
	}
	struct sockaddr_in destaddr;
	memset(&destaddr, 0, sizeof(destaddr));
	destaddr.sin_family = AF_INET;
	destaddr.sin_port = htons(port);
	int isvalidip = inet_pton(AF_INET, dest, &(destaddr.sin_addr));
	char connectmsg[BUFFLEN];
	memset(connectmsg, 0, BUFFLEN);
	struct rfss_node *remote_node;
	if(connected_node_count >= MAX_CONNECTED_NODES){
		printf("rfss_connect(): already connected with 4 nodes, cannot connect more\nrfss>");
		return -1;
	}
	if(!isvalidip){
		remote_node = rfss_findglobalnodebyhostname(dest);
		if(remote_node == NULL){
			printf("rfss_connect(): %s is not a valid destination\nrfss>",dest);
			return -1;
		}
		inet_pton(AF_INET, remote_node->ip, &(destaddr.sin_addr));
	}
	else{
		remote_node = rfss_findglobalnodebyip(dest);
		if(remote_node == NULL){
			printf("rfss_connect(): %s is not a valid destination\nrfss>");
			return -1;
		}
	}
	if(is_connected(remote_node->ip)){
		printf("Already connected with %s, duplicate connections not allowed\nrfss>",remote_node->hostname);
		return -1;
	}
	int remsockd = socket(AF_INET, SOCK_STREAM, 0);
	connect(remsockd, (struct sockaddr *)&destaddr, sizeof(destaddr));
	sprintf(connectmsg,"connect:%s:%s:%d",ip,myhostname,port);
	send(remsockd,connectmsg,strlen(connectmsg),0);
	memset(connectmsg,0,BUFFLEN);
	recv(remsockd,connectmsg,BUFFLEN,0);
	char *connectack = strtok(connectmsg,":");
	if(strcmp(connectack,"connectack")==0){
		FD_SET(remsockd, &fds);
		if(remsockd > maxfd)
			maxfd = remsockd;
		add_into_socketlist(remsockd);
		rfss_add_connected_node(remsockd, remote_node->ip, remote_node->hostname, port);
		printf("Peer %s connected\nrfss>",remote_node->hostname);
		return 0;
	}
	else{
		connectack = strtok(NULL, ":");
		printf("Error connecting %s: %s\nrfss>",remote_node->hostname,connectack);
		return -1;
	}
}

/*
 * Implements REGISTER command from UI
 * 
 * @arg serverip: IP of server
 * @arg serverport : port at which server is listening for connections.
 * 
 * @return 0 in case of success, -1 otherwise
 */
int rfss_register(char *serverip, int serverport){
	if(strcmp(serverip,ip)==0 || strcmp(serverip,"127.0.0.1")==0){
		printf("Cannot register with self\nrfss>");
		return -1;
	}
	if(connected_node_count>0){
		printf("This node is already registered\nrfss>");
		return -1;
	}
	char registermsg[BUFFLEN];
	memset(registermsg,0,BUFFLEN);
	char *hostname = (char *)malloc(sizeof(char)*BUFFLEN);
	memset(hostname, 0, BUFFLEN);
	if(strlen(myhostname)==0){
		gethostnamebyip(ip,&hostname);
		strcpy(myhostname, hostname);
	}
	sprintf(registermsg, "register:%s:%s:%d",ip,myhostname,port);
	int len = strlen(registermsg);
	struct sockaddr_in serveraddr;
	struct sockaddr_in myaddr;
	int clientsockd;
	memset(&serveraddr,0,sizeof(struct sockaddr_in));
	memset(&myaddr,0,sizeof(struct sockaddr_in));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(serverport);
	int isvalidip = inet_pton(AF_INET,serverip,&serveraddr.sin_addr);
	if(isvalidip == 0){
		printf("Error: %s is not a valid ip. Usage: REGISTER <SERVERIP> <PORT>\nrfss>");
		return -1;
	}
	if((clientsockd = socket(AF_INET, SOCK_STREAM, 0))<0){
		perror("register(): socket creation error\n");
		free(hostname);
		return -1;
	}
	if(connect(clientsockd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0){
		perror("register(): socket connect error\n");
		free(hostname);
		return -1;;
	}
	if(send(clientsockd, registermsg, len, 0) < 0){
		perror("register(): unable to send message to server\n");
		free(hostname);
		return -1;
	}
	memset(registermsg, 0, len);
	if(recv(clientsockd, registermsg, BUFFLEN, 0) < 0){
		perror("register(): error while receiving message from server\n");
		free(hostname);
		return -1;
	}
	char *saveptr = (char *)malloc(sizeof(char)*BUFFLEN);
	memset(saveptr,0,BUFFLEN);
	char *response = strtok_r(registermsg,":",&saveptr);
	if(strcasecmp(response, "registerack")!=0){
		response = strtok_r(NULL,":",&saveptr);
		printf("register(): Register failed. Response from server: %s\n",response);
		free(hostname);
		return -1;
	}
	memset(hostname,0,BUFFLEN);
	gethostnamebyip(serverip, &hostname);
	FD_SET(clientsockd, &fds);
	if(clientsockd > maxfd)
		maxfd = clientsockd;
	rfss_add_connected_node(clientsockd, serverip, hostname, serverport);
	char *entry;
	char *ip_entry, *host_entry, *port_entry;
	printf("List of clients aready connected:-\n");
	printf("   S.No\t\t\tHostname\t\tIP\tPort\n");
	int serial=1;
	while(1){
		entry = strtok_r(NULL,":",&saveptr);
		if(entry == NULL)
			break;
		ip_entry = strtok(entry,";");
		host_entry = strtok(NULL,";");
		port_entry = strtok(NULL,";");
		rfss_add_global_node(ip_entry,host_entry,atoi(port_entry));
		printf("%5d %30s %20s %10s\n",serial,host_entry,ip_entry,port_entry);
		serial++;
	}
	if(serial == 1){
		printf("You are the first client to connect\n");
	}
	free(hostname);
	return 0;
}

/*
 * Broadcast message to all the connected nodes.
 * Uses by server to notify connected nodes about network updation.
 * 
 * @arg msg: message to be broadcasted
 */
void broadcast_msg(char *msg){
	struct rfss_node *ptr;
	for(ptr=connected_node_list;ptr;ptr=ptr->next){
		send(ptr->sockd, msg, strlen(msg), 0);
		//send_msg(ptr->ip,ptr->port, msg);
	}
}

/*
 * Sends message to a particular node
 * 
 * @arg ip : IP of the remote machine
 * @arg port : port of the remote machine
 * @arg msg : message to be sent
 */
void send_msg(char *ip, int port, char *msg){
	struct sockaddr_in remaddr;
	memset(&remaddr, 0, sizeof(remaddr));
	remaddr.sin_family = AF_INET;
	remaddr.sin_port = htons(port);
	inet_pton(AF_INET, ip, &(remaddr.sin_addr));
	int remsockd = socket(AF_INET, SOCK_STREAM, 0);
	connect(remsockd, (struct sockaddr *)&remaddr, sizeof(remaddr));
	if(send(remsockd, msg, strlen(msg), 0)<0){
		printf("Error: could not send the message %s",msg);
	}
	close(remsockd);
}

/*
 * Returns the mode at which this application is operating.
 * Can be either CLIENT or SERVER.
 * 
 * @return mode
 */
int getmode(){
	return mode;
}
/*
struct rfss_node * rfss_findnodebysockd(int sockd){
	struct rfss_node * nd = connected_node_list;
	while(nd){
		if(nd->sockd == sockd)
			return nd;
	}
	return NULL;
}*/

/*
 * Gets the connection node against a given connection id.
 * 
 * @arg con: connection id
 * @return : node against the connection id if it exists, NULL otherwise
 */
struct rfss_node * rfss_getnodebyconn(int con){
	if(con <= 0 || con > connected_node_count)
		return NULL;
	struct rfss_node *nd = connected_node_list;
	int count = 1;
	while(count < con){
		nd = nd->next;
		count++;
	}
	return nd;
}

/*
 * Returns the number of connections this node is connected to
 * 
 * @return : connection count
 */
int getconnectioncount(){
	return connected_node_count;
}
