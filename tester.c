#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "rfss.h"
int main(int argc, char **argv){
	if(argc != 3){
		printf("Usage: rfss mode<s|c> port\n");
		exit(1);
	}
	if(strcmp(argv[1],"c")==0)
		set_mode(CLIENT);
	else if(strcmp(argv[1],"s")==0)
		set_mode(SERVER);
	else{
		printf("Invalid mode: mode can take value of s or c only\n");
		exit(1);
	}
	char *endptr;
	int port = (int) strtol(argv[2], &endptr, 10);
	if(*endptr != 0){
		printf("Invalid port\n");
		exit(1);
	}
	setup_listener(port);
	setbuf(stdout, NULL);
	//accept_client();
	process_command();
}
