#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <time.h>
#include <stdlib.h>
#include "rfss.h"
#include "rfss_node.h"
#include "remote_command_processor.h"

void rem_register(int sockd, char *ip, char *hostname, int port){
	char resp[BUFFLEN];
	memset(resp, 0, BUFFLEN);
	if(getmode() == CLIENT){
		sprintf(resp,"Error: I am not the server. Register command to me is invalid");
		send(sockd, resp, strlen(resp),0);
	}
	else{
		sprintf(resp,"join:%s:%s:%d",ip,hostname,port);
		broadcast_msg(resp);
		rfss_add_connected_node(sockd, ip, hostname, port);
		memset(resp, 0, BUFFLEN);
		sprintf(resp,"registerack");
		struct rfss_node *ptr = connected_node_list;
		char _port[10];
		while(ptr){
			strcat(resp,":");
			strcat(resp,ptr->ip);
			strcat(resp,";");
			strcat(resp,ptr->hostname);
			strcat(resp,";");
			memset(_port,0,10);
			sprintf(_port,"%d",ptr->port);
			strcat(resp,_port);
			ptr=ptr->next;
		}
		send(sockd, resp, strlen(resp),0);
		printf("New peer %s has registered\nrfss>",hostname);
	}
}

void rem_connect(int sockd, char *ip, char *hostname, int port){
	char resp[BUFFLEN];
	memset(resp, 0, BUFFLEN);
	if(getmode() == SERVER){
		sprintf(resp, "Error: I am a server node. Connect command is invalid with me");
		send(sockd, resp, strlen(resp),0);
	}
	else{
		FD_SET(sockd, &fds);
		if(sockd > maxfd)
			maxfd = sockd;
		add_into_socketlist(sockd);
		rfss_add_connected_node(sockd, ip, hostname, port);
		sprintf(resp,"connectack");
		send(sockd, resp, strlen(resp),0);
		printf("New peer %s connected with you\nrfss>",hostname);
	}
}

void rem_node_join(int sockd, char *ip, char *hostname, int port){
	rfss_add_global_node(ip, hostname, port);
	printf("New node %s with ip %s listening to port %d joined the network\nrfss>",hostname, ip, port);
}

void rem_node_leave(int sockd, char *ip){
	rfss_remove_global_node(sockd, ip);
	printf("Node %s left from the network\nrfss>");
}

void rem_terminate(int sockd, char *ip){
	char resp[BUFFLEN];
	memset(resp, 0, BUFFLEN);
	if(getmode() == SERVER){
		sprintf(resp, "Error: Terminate request cannot be issued to server");
		send(sockd, resp, strlen(resp),0);
	}
	else{
		close(sockd);
		FD_CLR(sockd, &fds);
		int connection = rfss_getcon(sockd);
		struct rfss_node *nd = rfss_getnodebyconn(connection);
		printf("Connection with node %s terminated\nrfss>",nd->hostname);
		rfss_remove_connection_node(connection);
		//rfss_remove_connected_node(sockd, ip);
	}
}

void rem_exit(int sockd, char *ip){
	char resp[BUFFLEN];
	memset(resp, 0, BUFFLEN);
	if(getmode() == CLIENT){
		sprintf(resp, "Error: Exit command can be issued only to server");
		send(sockd, resp, strlen(resp),0);
	}
	else{
		rfss_remove_global_node(sockd, ip);
		sprintf(resp,"leave:%s",ip);
		broadcast_msg(resp);
	}
}

void rem_invalid_command(int sockd){
	char resp[BUFFLEN];
	memset(resp, 0 , BUFFLEN);
	sprintf(resp, "Error: Invalid command");
}

void rem_upload(int sockd, char *file, int len){
	char *uploadack = "uploadack";
	send(sockd, uploadack, strlen(uploadack),0);
	printf("File upload of %s intitiated from remote peer\nrfss>",file);
	if(fu_recvfile(sockd, file, len)==0)
		printf("File upload of %s successful\nrfss>",file);
	else
		printf("File upload failed\nrfss>");
}
void rem_download_data(int sockd){
	char buff[BUFFLEN];
	memset(buff,0,BUFFLEN);
	int connection = rfss_getcon(sockd);
	struct rfss_node *nd = rfss_getnodebyconn(connection);
	if(nd->bytes_received == 0){
		nd->fd = open(nd->file, O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	}
	int bytesread = 0;
	int bytesremaining = nd->bytes_total - nd->bytes_received;
	if(bytesremaining > BUFFLEN)
		bytesread = recv(sockd, buff, BUFFLEN, 0);
	else
		bytesread = recv(sockd, buff, bytesremaining, 0);
	int byteswritten = write(nd->fd, buff, bytesread);
	if(byteswritten != bytesread){
		printf("Communicaton error while file transfer, terminating connection\nrfss>");
		close(sockd);
		FD_CLR(sockd, &fds);
		rfss_remove_connection_node(connection);
		return;
	}
	nd->bytes_received = nd->bytes_received + byteswritten;
	if(nd->bytes_received == nd->bytes_total){
		clock_gettime(CLOCK_REALTIME, &(nd->endtime));
		double timediff = (double)(nd->endtime.tv_sec - nd->starttime.tv_sec) +
						(double)(nd->endtime.tv_nsec - nd->starttime.tv_nsec)/1E9 ;
		double speed = (nd->bytes_total * 8)/timediff;
		downloadqcount--;
		printf("Download of file %s from host %s completed in %f seconds with rate of %f bits/seconds\n",nd->file, nd->hostname,timediff, speed);
		if(downloadqcount==0)
			printf("rfss>");
		nd->bytes_received = 0;
		nd->bytes_total = 0;
		nd->fd = -1;
		nd->status = COMMAND;
		free(nd->file);
	}
}
void rem_download(int sockd, char *file){
	int filesize = fu_getfilesize(file);
	char downloadack[BUFFLEN];
	memset(downloadack,0,BUFFLEN);
	if(filesize == -1){
		sprintf(downloadack,"Error: Invalid file");
		send(sockd, downloadack, strlen(downloadack), 0);
		return;
	}
	sprintf(downloadack,"downloadack:%d",filesize);
	send(sockd,downloadack, strlen(downloadack),0);
	memset(downloadack, 0, BUFFLEN);
	recv(sockd, downloadack, BUFFLEN, 0);
	if(strcmp(downloadack,"downloadack")==0){
		printf("File download for %s intitiated from remote peer\nrfss>",file);
		if(fu_sendfile(sockd, file, filesize)==0)
			printf("File download successfull\nrfss>");
		else
			printf("File download failed\nrfss>");
	}
}

void rem_process_command(int sockd){
	char command[BUFFLEN];
	char cmd[BUFFLEN];
	memset(command, 0, BUFFLEN);
	int connection = rfss_getcon(sockd);
	struct rfss_node *nd;
	if(connection > 0){
		nd = rfss_getnodebyconn(connection);
		if(nd->status == DATA){
			rem_download_data(sockd);
			return;
		}
	}
	if(recv(sockd, command, BUFFLEN, 0)<=0){
		printf("Connection with %s terminated\nrfss>",nd->hostname);
		FD_CLR(nd->sockd,&fds);
		close(nd->sockd);
		rfss_remove_connection_node(connection);
		//rfss_remove_connected_node(nd->sockd, nd->ip);
		return;
	}
	strcpy(cmd, command);
	char *req = strtok(cmd, ":");
	char *arg;
	char remip[20];
	char remport[10];
	char remhost[100];
	char filename[100];
	memset(remip, 0, 20);
	memset(remport, 0, 10);
	memset(remhost, 0, 100);
	memset(filename, 0 , 100);
	if(strcasecmp(req,"register")==0){
		arg = strtok(NULL, ":");
		sprintf(remip,"%s",arg);
		arg = strtok(NULL, ":");
		sprintf(remhost,"%s",arg);
		arg = strtok(NULL,":");
		sprintf(remport,"%s",arg);
		rem_register(sockd, remip, remhost, atoi(remport)); 
	}
	else if(strcasecmp(req,"connect")==0){
		arg = strtok(NULL, ":");
		sprintf(remip,"%s",arg);
		arg = strtok(NULL, ":");
		sprintf(remhost,"%s",arg);
		arg = strtok(NULL,":");
		sprintf(remport,"%s",arg);
		rem_connect(sockd, remip, remhost, atoi(remport)); 
	}
	else if(strcasecmp(req,"terminate")==0){
		arg = strtok(NULL, ":");
		sprintf(remip,"%s",arg);
		rem_terminate(sockd, remip);
	}
	else if(strcasecmp(req,"download")==0){
		arg = strtok(NULL, ":");
		sprintf(filename,"%s",arg);
		rem_download(sockd, filename);
	}
	else if(strcasecmp(req,"upload")==0){
		arg = strtok(NULL,":");
		sprintf(filename,"%s",arg);
		arg = strtok(NULL,":");
		rem_upload(sockd, filename, atoi(arg));
	}
	else if(strcasecmp(req,"join")==0){
		arg = strtok(NULL, ":");
		sprintf(remip,"%s",arg);
		arg = strtok(NULL, ":");
		sprintf(remhost,"%s",arg);
		arg = strtok(NULL,":");
		sprintf(remport,"%s",arg);
		rem_node_join(sockd, remip, remhost, atoi(remport)); 
	}
	else if(strcasecmp(req,"leave")==0){
		arg = strtok(NULL, ":");
		sprintf(remip,"%s",arg);
		rem_node_leave(sockd, remip);
	}
	else if(strcasecmp(req,"exit")==0){
		arg = strtok(NULL, ":");
		sprintf(remip,"%s",arg);
		rem_exit(sockd, remip);
	}
	else if(strcasecmp(req,"error")==0){
		arg = strtok(NULL, ":");
		printf("%s\nrfss>",arg);
	}
	else{
		rem_invalid_command(sockd);
	}
}
