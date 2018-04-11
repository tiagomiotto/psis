
#include "clipboard.h"

 
int main(){
	struct sockaddr_in their_addr;
	socklen_t addr_size;
	struct addrinfo hints, *res,*p;
	int my_fd, new_fd,aux2;
	int client;
	char data[10][10];
	Mensagem aux;
	char *msg = malloc(sizeof(Mensagem));
	int len_data;
	char buff[10];

	//Using Unix sockets
	if((my_fd=socket(AF_UNIX,SOCK_STREAM,0))<0){
		printf("Socket unsuccessful\n");
		return -1;
	}
	
	struct sockaddr_un addr;
	memset(&addr,0,sizeof(addr));
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, SOCK_PATH);
		unlink(SOCK_PATH);
		
		
	if((bind (my_fd, (struct sockaddr*)&addr, sizeof(struct sockaddr)))<0){
		printf("Bind unsuccessful\n");
		unlink(SOCK_PATH);
		free(msg);
		close(my_fd);
		return -1;
	}


	if((listen(my_fd, MAX_CALLS)) == -1){
		perror("listen");
		exit(1);
	}

	printf("Server: Connect to me \n");
	
	//Connect it
	while(1){

	addr_size= sizeof(their_addr);
	new_fd=accept(my_fd, NULL,NULL);
	if((new_fd)==-1){
		perror("accept");
		continue;
	}

	//inet_ntop(AF_INET, &(their_addr.sin_addr),client,sizeof(client));
	client++;
	printf("Server: My friend is %d online \n", client );

	
	//Talk to me

	
	while(1){ //Achar o signal, dar catch, e quebrar esse ciclo SIGPIPE
	if((recv(new_fd, msg, sizeof(Mensagem),0))==0) {
		printf("My client disconnected, waiting for a new one\n");
		break;
	}

	memcpy(&aux,msg,sizeof(Mensagem));
		if(aux.oper==0) //Copy
		{

			if((strcmp(data[aux.region],""))==0 ||(strcmp(data[aux.region],"\n"))==0  ){
				//In case the region is empty
			strcpy(aux.dados,"erro");
			memcpy(msg,&aux,sizeof(Mensagem));

			if((send(new_fd,msg, sizeof(aux),0))==-1){
			perror("send"); //Do I need the number of bytes?
			printf("My client %d disconnected\n", client);
			break;
			}	
			printf("My friend %d tried to copy from region %d, but it's empty\n",client,aux.region);
		}				
			
			
			else{
				
			strcpy(aux.dados,data[aux.region]);
			memcpy(msg,&aux,sizeof(Mensagem));

			if((send(new_fd,msg, sizeof(aux),0))==-1){
			perror("send"); //Do I need the number of bytes?
			printf("My client disconnected\n");
			break;
			}
			printf("My friend %d copied %s from region %d\n",client,data[aux.region],aux.region);
		}
			}
		else if (aux.oper ==1 ) //Paste
		{	
			strcpy(data[aux.region],aux.dados);
			printf("My friend %d pasted %s to region %d\n",client,data[aux.region],aux.region);
		}
		
	}

	close(new_fd);
}
	unlink(addr.sun_path);
	close(my_fd); //I don't want to listen anymore
	free(msg);
	exit(0);
	
}

