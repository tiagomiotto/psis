
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
	
	//Prepare structs
	memset(&hints,0,sizeof(hints));
	hints.ai_family = AF_INET; //Only IPv4 for me
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
	 
	if((listen(my_fd, MAX_CALLS)) == -1){
		perror("listen");
		exit(1);
	}

	printf("Server: Connect to me bitches\n");
	
	//Connect it
	while(1){

	addr_size= sizeof(their_addr);
	new_fd=accept(my_fd, (struct sockaddr *)&their_addr,&addr_size);
	if((new_fd)==-1){
		perror("accept");
		continue;
	}

	inet_ntop(AF_INET, &(their_addr.sin_addr),client,sizeof(client));
	printf("Server: My man %s is online \n", client );

	
	//Talk to me baby
	while(1){ //Achar o signal, dar catch, e quebrar esse ciclo SIGPIPE
	recv(new_fd, msg, sizeof(Mensagem),0);
	memcpy(&aux,msg,sizeof(Mensagem));
		if(aux.oper==0) //Copy
		{
			strcpy(aux.dados,data[aux.region]);
			memcpy(msg,&aux,sizeof(Mensagem));
			if((send(new_fd,msg, sizeof(aux),MSG_NOSIGNAL))==-1); perror("send"); //Do I need the number of bytes?
			printf("My man %s copied %s from region %d\n",client,data[aux.region],aux.region);
		}
		else if (aux.oper ==1 ) //Paste
		{
			strcpy(data[aux.region],aux.dados);
			printf("My man %s pasted %s to region %d\n",client,data[aux.region],aux.region);
		}
		
	}

	close(new_fd);
}
	
	close(my_fd); //I don't want to listen anymore
	free(msg);
	exit(0);
	
}

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}