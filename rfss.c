#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include "rfss_node.h"
#include "rfss.h"

static int sockd;
static fd_set fds;
static int maxfd = 0;
static struct sockaddr_in serv_addr;
static struct sockaddr_in client_addr[10];
static int client_count = 0;
static int mode;
static char ip[20];
static int port;
struct rfss_node *node_list = NULL;

void rfss_display_help(){
	printf("I am helpless right now\nrfss>");
}

void rfss_display_ip(){
	printf("%s\nrfss>",ip);
}

void rfss_display_port(){
	printf("%d\nrfss>",port);
}

void rfss_display_creator(){
	printf("Cursing me won't make your life better\nrfss>");
}

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
	//extract_local_ip();
	myiplookup();
}
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
}
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
}

void process_command(){
	int fdi;
	printf("rfss>");
	while(1){
		FD_ZERO(&fds);
		FD_SET(STDIN, &fds);
		FD_SET(sockd, &fds);
		maxfd = sockd;
		select(maxfd+1, &fds, NULL, NULL, NULL);
		if(FD_ISSET(STDIN, &fds))
			process_ui_command();
		if(FD_ISSET(sockd, &fds)){
			// connect request from a client
			accept_client();
		}
		for(fdi=0;fdi<=maxfd;fdi++){
			if(fdi==STDIN || fdi==sockd)
				continue;
			if(FD_ISSET(fdi, &fds)){
				// handle command from a connected client
				process_remote_command(fdi);
			}
		}
	}
}
void accept_client(){
	int clientlen = sizeof(client_addr[client_count]);
	int clientsoc = accept(sockd, (struct sockaddr *)&client_addr[client_count], &clientlen);
	FD_SET(clientsoc, &fds);
	if(clientsoc > maxfd)
		maxfd = clientsoc;
	clientsoc = clientsoc + 1;
}
void process_ui_command(){
	char cmdstr[100];
	char cmd[100];
	fgets(cmdstr, 99, stdin);
	int len = strlen(cmdstr);
	cmdstr[len-1] = '\0';
	strcpy(cmd, cmdstr);
	char *command = strtok(cmdstr," ");
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
	else if(strcasecmp(command,"list")==0){
		rfss_list();
	}
	else
		printf("%s received\nrfss>",cmd);
}
void rfss_exit(){
	exit(0);
}
void process_remote_command(int clientsockd){
	char cmdstr[100];
	char remip[50];
	char remport[20];
	memset(cmdstr,0,100);
	recv(clientsockd, cmdstr, 100, 0);
	char *command = strtok(cmdstr, " ");
	if(strcasecmp(command, "register")==0){
		if(mode==CLIENT){
			memset(cmdstr, 0 , 100);
			strcpy(cmdstr, "I am not the server");
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
		rfss_add_node(clientsockd, remip, atoi(remport));
		strcpy(cmdstr,"register ok");
		send(clientsockd, cmdstr, strlen(cmdstr), 0);
		//close(clientsockd);
		printf("New peer with ip %s registered\nrfss>", remip);
	}
	else{
		close(clientsockd);
		printf("%s received from remote client\nrfss>", cmdstr);
	}
}

int rfss_add_node(int clientsockd, char *ip, int port){
	struct rfss_node *new_node = (struct rfss_node *)malloc(sizeof(struct rfss_node));
	new_node->ip = (char *)malloc(sizeof(char)*50);
	new_node->hostname = (char *)malloc(sizeof(char)*100);
	memset(new_node->ip, 0, 50);
	strcpy(new_node->ip, ip);
	new_node->port = port;
	new_node->sockd = clientsockd;
	gethostnamebyip(ip, &(new_node->hostname));
	new_node->next = NULL;
	if(node_list == NULL)
		node_list = new_node;
	else{
		struct rfss_node *ptr;
		for(ptr=node_list; ptr->next; ptr=ptr->next){
			continue;
		}
		ptr->next = new_node;
	}
	return 0;
}

int gethostnamebyip(char *ip, char **hostname){
	struct sockaddr_in address;
	//struct hostent ret;
	//struct hostent *result;
	char buf[100];
	memset(buf,0,100);
	//int err;
	//address.s_addr=inet_addr(ip);
	inet_pton(AF_INET,ip,&address.sin_addr);
	address.sin_family = AF_INET;
	
	int res = getnameinfo((struct sockaddr *)&address, sizeof(address), buf, sizeof(buf), NULL, 0, 0);
	if(res == 0)
		strcpy(*hostname, buf);
	else
		printf("%s",gai_strerror(res));
	/*
	int res=gethostbyaddr_r((void *) &address,
                        sizeof(struct in_addr), AF_INET,
                        &ret, buf, 100, &result, &err);
    if(res == 0){
		strcpy(*hostname, ret.h_name);
	} */
	return res;
}

void set_mode(int modearg){
	mode = modearg;
}

void rfss_list(){
	if(node_list == NULL){
		printf("No hosts connected\nrfss>");
		return;
	}
	struct rfss_node *ptr;
	printf("id:Hostname\tIP adddress\tPort No\n");
	int id;
	for(ptr=node_list,id=1;ptr;ptr=ptr->next,id++){
		printf("%d : %s\t%s\t%d\n",id,ptr->hostname,ptr->ip,ptr->port);
	}
	printf("rfss>");
}

int rfss_register(char *serverip, int serverport){
	char registermsg[50];
	sprintf(registermsg, "register %s %d",ip,port);
	int len = strlen(registermsg);
	struct sockaddr_in serveraddr;
	struct sockaddr_in myaddr;
	int clientsockd;
	memset(&serveraddr,0,sizeof(struct sockaddr_in));
	memset(&myaddr,0,sizeof(struct sockaddr_in));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(serverport);
	inet_pton(AF_INET,serverip,&serveraddr.sin_addr);
	if((clientsockd = socket(AF_INET, SOCK_STREAM, 0))<0){
		perror("register(): socket creation error\n");
		return -1;
	}
	if(connect(clientsockd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0){
		perror("register(): socket connect error\n");
		return -1;;
	}
	if(send(clientsockd, registermsg, len, 0) < 0){
		perror("register(): unable to send message to server\n");
		return -1;
	}
	memset(registermsg, 0, len);
	if(recv(clientsockd, registermsg, 50, 0) < 0){
		perror("register(): error while receiving message from server\n");
		return -1;
	}
	if(strcasecmp(registermsg, "register ok")!=0){
		printf("register(): Register failed. Response from server: %s\n",registermsg);
		return -1;
	}
	rfss_add_node(clientsockd, serverip, serverport);
	return 0;
}

