#include "clipboard.h"


int clipboard_connect(char * clipboard_dir){
    int sockfd, numbytes;  
    char buf[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    int aux2;
    struct sockaddr_in ip;
    
    char s[INET6_ADDRSTRLEN];

	sockfd=socket(AF_UNIX,SOCK_STREAM,0);
	struct sockaddr_un addr;
	memset(&addr,0,sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, clipboard_dir, sizeof(addr.sun_path)-1);
	/*
    	//Prepare structs
	memset(&hints,0,sizeof(hints));
	hints.ai_family = AF_INET; //Only IPv4 for me
	hints.ai_socktype = SOCK_STREAM;
	

	
	if((aux2=getaddrinfo(clipboard_dir,MYPORT,&hints,&servinfo)) !=0){
		printf("getaddrinfo: \n");
		return -1;
	}

	//Make it, bind it, listen
	for(p= servinfo; p!=NULL; p=p->ai_next){
		if((sockfd=socket(p->ai_family, p->ai_socktype, p->ai_protocol))==-1){
			perror("server: socket");
			continue;
		}

		if((connect(sockfd,p->ai_addr,p->ai_addrlen))== -1){
			close(sockfd);
			perror("server:bind");
			continue;
		}
		break;
	}

	if(p==NULL){
		printf("server: failed to bind\n");
		exit(1);
	}
	free(servinfo);

	inet_ntop(p->ai_family,&(((struct sockaddr_in *)p->ai_addr)->sin_addr),s, sizeof s);
	*/
	if((connect(sockfd,(struct sockaddr*) &addr, sizeof(struct sockaddr_un)))==-1){
            close(sockfd);
            perror("client: connect");
            return -1;
	}
	printf("Connected to my bro %s\n", s);

	return sockfd;

}
//Guardar o tamanho do buffer aqui, trocar os dados de uma string pra uma struct, pra poder mandar tanto inteiros como strings
int clipboard_copy(int clipboard_id, int region, void *buf, size_t count){
	Mensagem aux;
	char *msg = malloc(sizeof(Mensagem));
	aux.region=region;
	aux.oper=0;
	
	memcpy(msg,&aux,sizeof(Mensagem));
	send(clipboard_id,msg,sizeof(Mensagem),0);
	
	if((recv(clipboard_id,msg,sizeof(Mensagem),0))<0) return -1;
	memcpy(&aux,msg,sizeof(Mensagem));
	if((strcmp(aux.dados,"erro"))!=0)
	strcpy(buf,aux.dados);
	else {
		printf("Region is empty\n");
		return -1;
}
	
	free(msg);
	return strlen(buf);
}
int clipboard_paste(int clipboard_id, int region, void *buf, size_t count){
	Mensagem aux;
	char *msg = malloc(sizeof(Mensagem));
	int retorno;
	if((strcmp(buf,""))==0 || (strcmp(buf,"\n"))==0){
		printf("You can't paste an empty line\n");
		return -1;
	}
	strcpy(aux.dados,buf);
	aux.region=region;
	aux.oper=1;
	memcpy(msg,&aux,sizeof(Mensagem));
	retorno=send(clipboard_id,msg,sizeof(Mensagem),0);
	if(retorno<=0){
		printf("Problem sending\n");
		return -1;
	}

	free(msg);
	return retorno;
}
