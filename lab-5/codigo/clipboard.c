#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "clipboard.h"

 
int main(){
	char file_name[100];
	
	sprintf(file_name, "./%s", OUTBOUND_FIFO);
	unlink(file_name);
	if(mkfifo(file_name, 0666)==-1){
		printf("Error creating out fifo\n");
		exit(-1);
	}
	int fifo_out = open(file_name, O_RDWR);
	if(fifo_out == -1){
		printf("Error opening in fifo\n");
		exit(-1);
	}
	
	
	
	sprintf(file_name, "./%s", INBOUND_FIFO);
	unlink(file_name);
	if(mkfifo(file_name, 0666)==-1){
		printf("Error creating in fifo\n");
		exit(-1);
	}
	int fifo_in = open(file_name, O_RDWR);
	if(fifo_in == -1){
		printf("Error opening in fifo\n");
		exit(-1);
	}

	//criar FIFOS
	
	
	//abrir FIFOS
	char data[10][10];
	Mensagem aux;
	char *msg = malloc(sizeof(Mensagem));
	int len_data;
	while(1){
		printf(".\n");
		read(fifo_in, msg, sizeof(Mensagem));
		memcpy(&aux,msg,sizeof(Mensagem));
		if(aux.oper==0) //Copy
		{
			strcpy(aux.dados,data[aux.region]);
			memcpy(msg,&aux,sizeof(Mensagem));
			write(fifo_out,msg, sizeof(aux));
			printf("Client copied %s from region %d\n",data[aux.region],aux.region);
		}
		else if (aux.oper ==1 ) //Paste
		{
			strcpy(data[aux.region],aux.dados);
			printf("Client pasted %s to region %d\n",data[aux.region],aux.region);
		}
	}
		
	exit(0);
	
}
