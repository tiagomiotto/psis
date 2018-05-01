
#include "clipboard.h"

int create_unix_sock(char* port);

 
int main(){
	
	//Unix sock
	int my_fd, new_fd;
	socklen_t addr_size;
	struct sockaddr_in their_addr;
	char client[INET6_ADDRSTRLEN];


	//Comunication variables
	char data[10][10];
	Mensagem aux;
	char *msg = malloc(sizeof(Mensagem));
	int len_data;
	char buff[10];
	int i;
	char port[10];
	srand(time(NULL));
	sprintf(port,"%d",(rand()%64738+1024));
	
	for(i=0;i<10;i++) //Start data as a clear array
	memset(&data[i],0,sizeof(data[i]));

	my_fd=create_unix_sock(port);
	
	if((listen(my_fd, MAX_CALLS)) == -1){
		perror("listen");
		exit(1);
	}

	printf("Server: Connect to me \n");
	
	//Connect it
	while(1){

	addr_size= sizeof(their_addr);
	new_fd=accept(my_fd, (struct sockaddr *)&their_addr,&addr_size);
	if((new_fd)==-1){
		perror("accept");
		continue;
	}

	//inet_ntop(AF_INET, &(their_addr.sin_addr),client,sizeof(client));
	printf("Server: My at client %s is online \n", client );

	//Talk to me
	while(1){ //Guardar o tamanho do buffer aqui, trocar os dados de uma string pra uma struct, pra poder mandar tanto inteiros como strings
	if((recv(new_fd, msg, sizeof(Mensagem),0))==0) {
		printf("My client disconnected, waiting for a new one\n");
		break;
	}

		memcpy(&aux,msg,sizeof(Mensagem));
		if(aux.oper==0) //Copy
		{

			if(strcmp(data[aux.region],"")==0){
				//In case the region is empty
			strcpy(aux.dados,"erro");
			memcpy(msg,&aux,sizeof(Mensagem));

			if((send(new_fd,msg, sizeof(aux),0))==-1){
			perror("send"); //Do I need the number of bytes?
			printf("My client disconnected\n");
			break;
			}	
			printf("My friend at %s tried to copy from region %d, but it's empty\n",client,aux.region);
		}				
			
			
			else{
				
			strcpy(aux.dados,data[aux.region]);
			memcpy(msg,&aux,sizeof(Mensagem));

			if((send(new_fd,msg, sizeof(aux),0))==-1){
			perror("send"); //Do I need the number of bytes?
			printf("My client disconnected\n");
			break;
			}
			printf("My friend at %s copied %s from region %d\n",client,data[aux.region],aux.region);
		}
			}
		else if (aux.oper ==1 ) //Paste
		{	
			strcpy(data[aux.region],aux.dados);
			printf("My client pasted %s to region %d\n",data[aux.region],aux.region);
		}
		
	}

	close(new_fd);
}
	
	close(my_fd); //I don't want to listen anymore
	unlink("127.0.0.1");
	free(msg);
	exit(0);
	
}

int create_unix_sock(char* port){
		//Variables for the unix socket (Talvez fazer isso dentro de uma função?)
	struct sockaddr_in their_addr;
	socklen_t addr_size;
	struct addrinfo hints, *res,*p;
	int my_fd, new_fd,aux2;
	
	
	//Comunication variables


	
	//Prepare structs for the socket
	memset(&hints,0,sizeof(hints));
	hints.ai_family = AF_INET; //Only IPv4 for me AF_UNIX
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags= AI_PASSIVE;
	
	if((aux2=getaddrinfo(NULL,port,&hints,&res)) !=0){
		printf("getaddrinfo: \n");
		return -1;
	}

	//Make it, bind it, listen
	for(p= res; p!=NULL; p=p->ai_next){
		if((my_fd=socket(p->ai_family, p->ai_socktype, p->ai_protocol))==-1){
			perror("server: socket");
			continue;
		}

		if((bind(my_fd,p->ai_addr,p->ai_addrlen))== -1){
			close(my_fd);
			unlink("127.0.0.1");
			perror("server:bind");
			continue;
		}
		break;
	}

	if(p==NULL){
		printf("server: failed to bind\n");
		exit(1);
	}
	free(res);
	printf("Backup at %s:%s\n", SOCK_ADDR,port);
	return my_fd;
}
