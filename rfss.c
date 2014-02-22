#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <arpa/inet.h>
#include <string.h>
#include "rfss_node.h"
#include "file_utils.h"
#include "rfss.h"

static int sockd;
fd_set fds;
int maxfd = 0;
int downloadqcount = 0;
static int connected_node_count = 0;
static struct sockaddr_in serv_addr;
static struct sockaddr_in client_addr[10];
static int client_count = 0;
static int mode;
static char ip[20];
static char myhostname[BUFFLEN];
static int port;
struct rfss_node *connected_node_list = NULL;
struct rfss_node *global_node_list = NULL;

struct socklist {
	int sockd;
	struct socklist *next;
} *socketlist;

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
		/*
		while(socketlist){
			FD_SET(socketlist->sockd, &fds);
			socketlist = socketlist->next;
		}*/
		//FD_SET(sockd, &fds);
		select(maxfd+1, &tfds, NULL, NULL, NULL);
		if(FD_ISSET(STDIN, &tfds)){
			rfss_process_ui_command();
			continue;
			//FD_CLR(STDIN, &fds);
			//FD_SET(STDIN, &fds);
		}
		if(FD_ISSET(sockd, &tfds)){
			// connect request from a client
			accept_client();
			continue;
			//FD_CLR(sockd, &fds);
			//FD_SET(sockd, &fds);
		}
		for(fdi=0;fdi<=maxfd;fdi++){
			if(fdi==STDIN || fdi==sockd)
				continue;
			if(FD_ISSET(fdi, &tfds)){
				// handle command from a connected client
				rem_process_command(fdi);
				continue;
				//FD_CLR(fdi, &fds);
				//FD_SET(fdi, &fds);
				//process_remote_command(fdi);
			}
		}
	}
}
void add_into_socketlist(int socketd){
	while(socketlist->next){
		socketlist = socketlist->next;
	}
	struct socklist *new = (struct socklist *)malloc(sizeof(struct socklist));
	new->sockd = socketd;
	new->next = NULL;
	socketlist->next = new;
}
void accept_client(){
	int clientlen = sizeof(client_addr[client_count]);
	int clientsoc = accept(sockd, (struct sockaddr *)&client_addr[client_count], &clientlen);
	FD_SET(clientsoc, &fds);
	if(clientsoc > maxfd)
		maxfd = clientsoc;
	add_into_socketlist(clientsoc);
	clientsoc = clientsoc + 1;
}
void rfss_process_ui_command(){
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
			printf("Error: Invalid register command. Usage: connect <destination> <serverport>\nrfss>");
			return;
		}
		char *endptr;
		int porti = (int) strtol(serverport, &endptr, 10);
		if(*endptr != 0){
			printf("Error: Invalid register command. Invalid port.\nrfss>");
			return;
		}
		if(rfss_connect(dest, porti)<0){
			printf("Error: Registeration failed\nrfss>");
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
		printf("%s received\nrfss>",cmd);
}
void rfss_exit(){
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

struct rfss_node * rfss_findglobalnodebyip(char *nodeip){
	struct rfss_node *ptr = global_node_list;
	while(ptr){
		if(strcmp(ptr->ip,nodeip)==0)
			return ptr;
		ptr = ptr->next;
	}
	return NULL;
}

struct rfss_node * rfss_findglobalnodebyhostname(char *hostname){
	struct rfss_node *ptr = global_node_list;
	while(ptr){
		if(strcmp(ptr->hostname,hostname)==0)
			return ptr;
		ptr = ptr->next;
	}
	return NULL;
}
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
	rfss_download(connid, file);
	while(1){
		_connid = strtok_r(NULL," ",&saveptr);
		if(_connid == NULL)
			break;
		connid = atoi(_connid);
		file = strtok_r(NULL," ",&saveptr);
		if(file == NULL)
			break;
		rfss_download(connid, file);
	}
}

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
void rfss_upload(int connid, char *file){
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
	sprintf(uploadreq,"upload:%s:%d",file,filesize);
	send(sockd, uploadreq, strlen(uploadreq), 0);
	memset(uploadreq, 0, BUFFLEN);
	recv(sockd, uploadreq, BUFFLEN, 0);
	if(strcmp(uploadreq,"uploadack")!=0){
		printf("Error: Cannot upload file. Remote peer not ready\nrfss>");
		return;
	}
	fu_sendfile(sockd, file, filesize);
	printf("File %s successfully uploaded\nrfss>",file);
}

void rfss_list(){
	if(connected_node_list == NULL){
		printf("No hosts connected\nrfss>");
		return;
	}
	struct rfss_node *ptr;
	printf("id:Hostname\tIP adddress\tPort No\n");
	int id;
	for(ptr=connected_node_list,id=1;ptr;ptr=ptr->next,id++){
		printf("%d : %s\t%s\t%d\n",id,ptr->hostname,ptr->ip,ptr->port);
	}
	printf("rfss>");
}

int rfss_connect(char *dest, int port){
	struct sockaddr_in destaddr;
	memset(&destaddr, 0, sizeof(destaddr));
	destaddr.sin_family = AF_INET;
	destaddr.sin_port = htons(port);
	int isvalidip = inet_pton(AF_INET, dest, &(destaddr.sin_addr));
	char connectmsg[BUFFLEN];
	memset(connectmsg, 0, BUFFLEN);
	struct rfss_node *remote_node;
	if(connected_node_count >= 4){
		printf("rfss_connect(): already connected with 4 nodes, cannot connect more\nrfss>");
		return -1;
	}
	if(!isvalidip){
		remote_node = rfss_findglobalnodebyhostname(dest);
		if(remote_node == NULL){
			printf("rfss_connect(): %s is not a valid destination\nrfss>");
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
	}
	else{
		printf("Error connecting %s: %s\nrfss",remote_node->hostname,connectack);
	}
}

int rfss_register(char *serverip, int serverport){
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
	inet_pton(AF_INET,serverip,&serveraddr.sin_addr);
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
		printf("register(): Register failed. Response from server: %s\n",registermsg);
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
	while(1){
		entry = strtok_r(NULL,":",&saveptr);
		if(entry == NULL)
			break;
		ip_entry = strtok(entry,";");
		host_entry = strtok(NULL,";");
		port_entry = strtok(NULL,";");
		rfss_add_global_node(ip_entry,host_entry,atoi(port_entry));
	}
	free(hostname);
	return 0;
}

void broadcast_msg(char *msg){
	struct rfss_node *ptr;
	for(ptr=connected_node_list;ptr;ptr=ptr->next){
		send(ptr->sockd, msg, strlen(msg), 0);
		//send_msg(ptr->ip,ptr->port, msg);
	}
}

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

int getconnectioncount(){
	return connected_node_count;
}
