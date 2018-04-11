
#include "clipboard.h"

 
int main(){
	struct sockaddr_in their_addr;
	socklen_t addr_size;
	struct addrinfo hints, *res,*p;
	int my_fd, new_fd,aux2;
	char client[INET6_ADDRSTRLEN];
	char data[10][10];
	Mensagem aux;
	char *msg = malloc(sizeof(Mensagem));
	int len_data;
	char buff[10];

	if((my_fd=socket(AF_UNIX,SOCK_STREAM,0))<0){
		printf("Socket unsuccessful\n");
		return -1;
	}
	printf("%d\n",my_fd );
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

	/*
	//Prepare structs
	memset(&hints,0,sizeof(hints));
	hints.ai_family = AF_INET; //Only IPv4 for me AF_UNIX
	//sockaddr_un
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags= AI_PASSIVE;
	
	if((aux2=getaddrinfo(NULL,MYPORT,&hints,&res)) !=0){
		printf("getaddrinfo: \n");
		return -1;
	}

	//Make it, bind it, listen
	for(p= res; p!=NULL; p=p->ai_next){
		if((my_fd=socket(p->ai_family, p->ai_socktype, p->ai_protocol))==-1){
			perror("server: socket");
			continue;
		}

		if((bind(my_fd,res->ai_addr,res->ai_addrlen))== -1){
			close(my_fd);
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
	*/

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
	printf("Server: My man is online \n" );

	
	//Talk to me

	
	while(1){ //Achar o signal, dar catch, e quebrar esse ciclo SIGPIPE
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
			printf("My man %s tried to copy from region %d, but it's empty\n",client,aux.region);
		}				
			
			
			else{
				
			strcpy(aux.dados,data[aux.region]);
			memcpy(msg,&aux,sizeof(Mensagem));

			if((send(new_fd,msg, sizeof(aux),0))==-1){
			perror("send"); //Do I need the number of bytes?
			printf("My client disconnected\n");
			break;
			}
			printf("My man %s copied %s from region %d\n",client,data[aux.region],aux.region);
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
	unlink(addr.sun_path);
	close(my_fd); //I don't want to listen anymore
	free(msg);
	exit(0);
	
}

