/*
 * remote_command_processor.c : Contains functions to respond to commands
 * sent by remote peers by sockets.
 * 
 * Created for CSE 589 Spring 2014 Programming Assignment 1
 * 
 * @Author : Biplap Sarkar (biplapsa@buffalo.edu)
 *
 */
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <time.h>
#include <stdlib.h>
#include "ft_stat.h"
#include "file_utils.h"
#include "rfss.h"
#include "rfss_node.h"
#include "remote_command_processor.h"

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
		rfss_add_connected_node(sockd, ip, hostname, port);
		send(sockd, resp, strlen(resp),0);
		printf("New peer %s has registered\nrfss>",hostname);
	}
}

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
void rem_connect(int sockd, char *ip, char *hostname, int port){
	char resp[BUFFLEN];
	memset(resp, 0, BUFFLEN);
	if(getmode() == SERVER){
		sprintf(resp, "Error: I am a server node. Connect command is invalid with me");
		send(sockd, resp, strlen(resp),0);
	}
	else if(connected_node_count >= MAX_CONNECTED_NODES){
		sprintf(resp, "Error: %s is already connected with %d peers",myhostname,connected_node_count);
		send(sockd, resp, strlen(resp),0);
		close(sockd);
		FD_CLR(sockd, &fds);
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
void rem_node_join(int sockd, char *ip, char *hostname, int port){
	rfss_add_global_node(ip, hostname, port);
	printf("New node %s with ip %s listening to port %d joined the network\nrfss>",hostname, ip, port);
}

/* 
 * Function to handle notification of a node leaving from network.
 * This notification is sent by the server node.
 * 
 * @arg ip: IP of the exiting node
 * @arg port: Port at which the exiting node was listening for connections.
 * @return void
 */
void rem_node_leave(char *ip, int port){
	//rfss_remove_global_node(sockd, ip);
	struct rfss_node *nd = global_node_list;
	struct rfss_node *tmp;
	if(strcmp(ip,nd->ip)==0 && nd->port ==  port){
		global_node_list = global_node_list->next;
		free(nd->ip);
		free(nd->hostname);
		free(nd);
	}
	while(nd->next){
		if(strcmp(nd->next->ip, ip) == 0 && nd->next->port == port)
			break;
		nd = nd->next;
	}
	if(nd->next){
		tmp = nd->next;
		nd->next = tmp->next;
		free(tmp->ip);
		free(tmp->hostname);
		free(tmp);
	}
	printf("Node %s left from the network\nrfss>",ip);
}

/*
 * Function to handle TERMINATE request from a connected client node.
 * This function is available to nodes running as clients only.
 * 
 * @arg sockd: Socket descriptor of the tcp connection.
 * @arg ip: IP of the remote node.
 * @return void
 */
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

/*
 * Function to handle EXIT command from a node in network.
 * This function is available to server only.
 * 
 * @arg sockd: Socket descriptor of the exiting node.
 * @return void
 */
void rem_exit(int sockd){
	int connid = rfss_getcon(sockd);
	struct rfss_node *exitnd = rfss_getnodebyconn(connid);
	char resp[BUFFLEN];
	memset(resp, 0, BUFFLEN);
	/*if(getmode() == CLIENT){
		sprintf(resp, "Error: Exit command can be issued only to server");
		send(sockd, resp, strlen(resp),0);
	}*/
	if(getmode() == SERVER){
		printf("Peer %s left the network\nrfss>",exitnd->hostname);
		sprintf(resp,"leave:%s:%d",exitnd->ip,exitnd->port);
		close(sockd);
		FD_CLR(sockd, &fds);
		rfss_remove_connection_node(connid);
		broadcast_msg(resp);
	}
}

/* 
 * Function to handle invalid command sent by socket.
 * 
 * @arg sockd: Socket descriptor of the tcp connection.
 */
void rem_invalid_command(int sockd){
	char resp[BUFFLEN];
	memset(resp, 0 , BUFFLEN);
	sprintf(resp, "Error: Invalid command");
}

/* 
 * Function to handle UPLOAD command sent by client.
 * 
 * @arg sockd: Socket descriptor of the tcp connection.
 * @arg file: name of the file which is uploaded by remote client.
 * @arg len: lenght of the file in bytes
 * @return void
 */
void rem_upload(int sockd, char *file, int len){
	char *uploadack = "uploadack";
	send(sockd, uploadack, strlen(uploadack),0);
	int connid = rfss_getcon(sockd);
	struct rfss_node *nd = rfss_getnodebyconn(connid);
	printf("File upload of %s intitiated from %s\nrfss>",file,nd->hostname);
	struct timespec starttime, endtime;
	clock_gettime(CLOCK_REALTIME, &starttime);
	if(fu_recvfile(sockd, file, len)==0){
		clock_gettime(CLOCK_REALTIME, &endtime);
		double timediff = (double)(endtime.tv_sec - starttime.tv_sec) +
						(double)(endtime.tv_nsec - starttime.tv_nsec)/1E9 ;
		double speed = (len * 8)/timediff;
		printf("File %s transferred successfully\n",file);
		printf("%s -> %s, File Size: %d, Time Taken: %f seconds, Rx Rate %.2f bits/second\nrfss>",nd->hostname,myhostname,len,timediff,speed);
		//printf("File upload of %s successful\nrfss>",file);
	}
	else
		printf("File upload failed\nrfss>");
}

/*
 * Reads data from a socket descriptor and writes into
 * file associated with the connection in which data download task is pending.
 * 
 * @arg sockd: Socket descriptor of the tcp connection.
 * @return void
 */
void rem_download_data(int sockd){
	char buff[FILEBUFFLEN];
	memset(buff,0,FILEBUFFLEN);
	int connection = rfss_getcon(sockd);
	struct rfss_node *nd = rfss_getnodebyconn(connection);
	if(nd->ft_list->bytes_received == 0){
		nd->ft_list->fd = open(nd->ft_list->file, O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		clock_gettime(CLOCK_REALTIME, &(nd->ft_list->starttime));
	}
	int bytesread = 0;
	int bytesremaining = nd->ft_list->bytes_total - nd->ft_list->bytes_received;
	if(bytesremaining > FILEBUFFLEN)
		bytesread = recv(sockd, buff, FILEBUFFLEN, 0);
	else
		bytesread = recv(sockd, buff, bytesremaining, 0);
	int byteswritten = write(nd->ft_list->fd, buff, bytesread);
	if(byteswritten != bytesread){
		printf("Communication error while file transfer, terminating connection\nrfss>");
		close(sockd);
		FD_CLR(sockd, &fds);
		rfss_remove_connection_node(connection);
		return;
	}
	nd->ft_list->bytes_received = nd->ft_list->bytes_received + byteswritten;
	if(nd->ft_list->bytes_received == nd->ft_list->bytes_total){
		clock_gettime(CLOCK_REALTIME, &(nd->ft_list->endtime));
		double timediff = (double)(nd->ft_list->endtime.tv_sec - nd->ft_list->starttime.tv_sec) +
						(double)(nd->ft_list->endtime.tv_nsec - nd->ft_list->starttime.tv_nsec)/1E9 ;
		double speed = (nd->ft_list->bytes_total * 8)/timediff;
		printf("Download of file %s from host %s completed\n",nd->ft_list->file,nd->hostname);
		printf("%s -> %s, File size:%d bytes, Time Taken: %f seconds, Rx Rate: %.2f bits/second\n", nd->hostname,myhostname,nd->ft_list->bytes_total,timediff, speed);
		struct ft_stat *ptr = nd->ft_list;
		nd->ft_list = nd->ft_list->next;
		free(ptr->file);
		free(ptr);
		if(nd->ft_list == NULL){
			downloadqcount--;
			nd->status = COMMAND;
		}
		if(downloadqcount==0)
			printf("rfss>");
	}
}
/*
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
	struct timespec starttime, endtime;
	if(strcmp(downloadack,"downloadack")==0){
		printf("File download for %s intitiated from remote peer\nrfss>",file);
		clock_gettime(CLOCK_REALTIME, &starttime);
		if(fu_sendfile(sockd, file, filesize)==0){
			clock_gettime(CLOCK_REALTIME, &endtime);
			double timediff = (double)(endtime.tv_sec - starttime.tv_sec) +
						(double)(endtime.tv_nsec - starttime.tv_nsec)/1E9 ;
			double speed = (filesize * 8)/timediff;
			printf("File %s transferred in %f seconds with speed %fbps\nrfss>",file,timediff,speed);
		}
		else
			printf("File download failed\nrfss>");
	}
}*/

/*
 * This function reads raw data from remote clients via
 * sockets, parses them into commands and invokes specific
 * routine/function to handle those commands.
 * 
 * @arg sockd: Socket descriptor of the tcp connection
 * @return void
 */
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
		char *resp = (char *)malloc(sizeof(char)*BUFFLEN);
		memset(resp,0,BUFFLEN);
		strcpy(resp,"downloadack");
		struct ft_stat *file_list = NULL;
		struct ft_stat *ptr = file_list;
		while(1){
			arg = strtok(NULL,":");
			if(arg == NULL){
				break;
			}
			else{
				int filesize = fu_getfilesize(arg);
				char _filesize[20];
				if(filesize < 0){
					memset(resp,0,BUFFLEN);
					sprintf(resp,"Error: File %s does not exist",arg);
					send(sockd,resp,strlen(resp),0);
					return;
				}
				else{
					struct ft_stat *stat = (struct ft_stat *)malloc(sizeof(struct ft_stat));
					stat->file = (char *)malloc(sizeof(char)*BUFFLEN);
					memset(stat->file,0,BUFFLEN);
					strcpy(stat->file,arg);
					stat->bytes_total = filesize;
					stat->next = NULL;
					if(file_list == NULL){
						file_list = stat;
						ptr=stat;
					}
					else{
						ptr->next = stat;
						ptr=ptr->next;
					}
					sprintf(_filesize,"%d",filesize);
					strcat(resp,":");
					strcat(resp,_filesize);
				}
			}
		}
		send(sockd,resp,strlen(resp),0);
		memset(resp,0,BUFFLEN);
		recv(sockd,resp,BUFFLEN,0);
		if(strcmp(resp,"downloadack")==0){
			//printf("File download for intitiated from remote peer\nrfss>",file);
			ptr = file_list;
			struct timespec starttime, endtime;
			while(ptr){
				printf("File transfer of %s to remote peer %s initiated\nrfss>",ptr->file,nd->hostname);
				clock_gettime(CLOCK_REALTIME, &starttime);
				if(fu_sendfile(sockd,ptr->file, ptr->bytes_total)==0){
					clock_gettime(CLOCK_REALTIME, &endtime);
					double timediff = (double)(endtime.tv_sec - starttime.tv_sec) +
						(double)(endtime.tv_nsec - starttime.tv_nsec)/1E9 ;
					double speed = (ptr->bytes_total * 8)/timediff;
					printf("File %s transferred successfully\n",ptr->file);
					printf("%s -> %s File Size: %d, Time Taken: %f seconds, Tx Rate: %.2f bits/second\nrfss>",myhostname,nd->hostname,ptr->bytes_total,timediff,speed);
				}
				else
					printf("File %s transfer failed",ptr->file);
				file_list = file_list->next;
				free(ptr->file);
				free(ptr);
				ptr=file_list;
			}
		/*
		if(fu_sendfile(sockd, file, filesize)==0)
			printf("File download successfull\nrfss>");
		else
			printf("File download failed\nrfss>");
		*/
		}
		//arg = strtok(NULL, ":");
		//sprintf(filename,"%s",arg);
		//rem_download(sockd, filename);
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
		char *ip = strtok(NULL, ":");
		char *_port = strtok(NULL, ":");
		//sprintf(remip,"%s",arg);
		rem_node_leave(ip, atoi(_port));
	}
	else if(strcasecmp(req,"exit")==0){
		rem_exit(sockd);
	}
	else if(strcasecmp(req,"error")==0){
		//arg = strtok(NULL, ":");
		printf("%s\nrfss>",command);
	}
	else{
		rem_invalid_command(sockd);
	}
}
