#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <pthread.h>

#include "clipboard.h"


void ctrl_c_callback_handler(int signum){
	printf("Caught signal Ctr-C\n");
	unlink(SOCK_ADDRESS);
	exit(0);
}


Clipboard_struct clipboard; //actual clipboard
int ss_internet, port;
void * data = NULL;
int modo; //indica se é backup mode
struct sockaddr_in backup_addr;
struct sockaddr_un local_addr;
struct sockaddr_un client_addr;
socklen_t size_addr;
char ip[IPSIZE];

void * handleClient(void * p){
	int client_fd = *((int*)p);
	Message_struct messageReceived, messageSent;
	const int error = 0, success = 1;

	while(1){
		if(!read(client_fd, &messageReceived, sizeof(Message_struct)))
			break;
		
		//Reads the action that will take
	
		if(messageReceived.action == COPY) {
			int allowCopy;
			data = malloc(messageReceived.dataSize); // Allocs memory to store new data
			if(data == NULL) {
				printf("ERROR ALOCATING MEMORY\n");
				allowCopy = 0;
				write(client_fd, &allowCopy, sizeof(int));
			}
			else{
				allowCopy = 1;
				write(client_fd, &allowCopy, sizeof(int));
				read(client_fd, data , messageReceived.dataSize);
				clipboard.dataSize[messageReceived.region] = messageReceived.dataSize;
				clipboard.clipboard[messageReceived.region] = data;
				if(modo == BACKUPMODE){
					ss_internet = socket(AF_INET, SOCK_STREAM, 0);
					if(ss_internet == -1){
						perror("socket: ");
						return p;
					}
					if(-1 == connect(ss_internet, (const struct sockaddr *) &backup_addr, sizeof(backup_addr))){
						printf("Error connecting\n");
						return p;
					}
					printf("connected to backup\n");
					clipboard_copy(ss_internet, messageReceived.region, clipboard.clipboard[messageReceived.region], clipboard.dataSize[messageReceived.region]);	
					close(ss_internet);					
				}

			}


			printf("data stored in position %d: %s\n", messageReceived.region, (char *)clipboard.clipboard[messageReceived.region]);
		}
		else if(messageReceived.action == PASTE) {
		//printf("Received information - action: PASTE\n");
			printf("\nPASTE\n");
			// Confirms if the client has enough space to store the PASTE data
			if(messageReceived.dataSize < clipboard.dataSize[messageReceived.region]) {
				printf("%d size dataSize < %d\n", (int)messageReceived.dataSize, (int)clipboard.dataSize[messageReceived.region]);
				write(client_fd, &error, sizeof(int));
			} else if(clipboard.clipboard[messageReceived.region] == NULL){
				printf("Cliente tentou aceder a posição sem dados\n");
				write(client_fd, &error, sizeof(int));
			}
			else {
				printf("%d size dataSize 2\n", (int)messageReceived.dataSize);
				write(client_fd, &success, sizeof(int));

				// Loads the structure with the information to the client
				messageSent.action = PASTE;
				messageSent.dataSize = clipboard.dataSize[messageReceived.region];
				messageSent.region = messageReceived.region;

				write(client_fd, &messageSent, sizeof(Message_struct));

				// Sends the data to the client
				int numberOfBytesPaste = write(client_fd, clipboard.clipboard[messageSent.region], messageSent.dataSize);
			
				printf("Sent %d bytes - data: %s\n", numberOfBytesPaste, (char *)clipboard.clipboard[messageSent.region]);
			}
		}


		else {
			break;
		}
	}
	return p;
}

int main(int argc, char * argv[]){
	int i;



	
	pthread_t thread[1];
	
	if(argc == 1){
		modo = LOCAL;
	} else if(argc == 4){
		printf("%s\n", argv[1]);
		if(strcmp(argv[1], BACKUPFLAG) == 0){
			modo = BACKUPMODE;
			printf("modo backup\n");
			strcpy(ip, argv[2]);
			port = atoi(argv[3]);
			if(port > 64738 || port < 1024){
				printf("Valor do porto inválido\n");
				exit(-1);
			}
			
			
		} else{
			printf("Inicializa isto como deve ser");
			exit(0);
		}
	}
	else{
		printf("Inicializa isto como deve ser");
		exit(0);
	}
	
	
	// Atach the ctrl_c_callback_handler to the SIGINT signal
	signal(SIGINT, ctrl_c_callback_handler);

	unlink(SOCK_ADDRESS);
	int ss_unix = socket(AF_UNIX, SOCK_STREAM, 0);
	if (ss_unix == -1){
		perror("socket: ");
		exit(-1);
	}
	
	if(modo == BACKUPMODE){
		printf("BACKUPMODE\n");
		ss_internet = socket(AF_INET, SOCK_STREAM, 0);
		if(ss_internet == -1){
			perror("socket: ");
			exit(-1);
		}
		backup_addr.sin_family = AF_INET;
		backup_addr.sin_port= htons(port);
		inet_aton(ip, &backup_addr.sin_addr);
		
		if(-1 == connect(ss_internet, (const struct sockaddr *) &backup_addr, sizeof(backup_addr))){
			printf("Error connecting\n");
			exit(-1);
		}
		printf("connected to backup\n");
		if(clipboard_init(ss_internet, &clipboard) == 0){
			printf("Não consegui alocar a memória vinda do backup\n");
		} 
		for (i = 0; i < 10; i++){
			if(clipboard.dataSize[i] == 0) continue;
			printf("li do backup %d: %d, %s\n", i, (int)clipboard.dataSize[i], (char *) clipboard.clipboard[i]);
		}

	} else{
		for(i=0; i < NUMBEROFPOSITIONS; i++){
			clipboard.clipboard[i] = NULL;
			clipboard.dataSize[i] = 0;
		}
	}
	

	local_addr.sun_family = AF_UNIX;
	//sprintf(socket_name, "%s%s", "./", SOCK_ADDRESS); //Nome do socket
	//strcpy(local_addr.sun_path, socket_name);
	strcpy(local_addr.sun_path, SOCK_ADDRESS);
	
	int err = bind(ss_unix, (struct sockaddr *)&local_addr, sizeof(local_addr));
	if(err == -1) {
		perror("bind");
		exit(-1);
	}
	printf("Clipboard socket created and binded \n");
	
	if(listen(ss_unix, 5) == -1) {
		perror("listen");
		exit(-1);
	}

	


	while(1){
		
		printf("Clipboard %d Ready to accept connections\n", getpid());
		size_addr = sizeof(socklen_t);
		int client_fd = accept(ss_unix, (struct sockaddr *) & client_addr, &size_addr);
		if(client_fd == -1) {
			perror("accept");
			exit(-1);
		}
		
		printf("Clipboard: accepted connection\n");
		printf("client_fd: %d\n", client_fd);
		pthread_create(thread, NULL, handleClient, &client_fd);
		
	}
	exit(0);
	
}
