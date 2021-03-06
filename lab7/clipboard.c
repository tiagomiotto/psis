
#include "clipboard.h"

 
int main(int argc, char **argv){


	//Variables for the UNIX SOCK
	struct sockaddr_in their_addr;
	socklen_t addr_size;
	struct addrinfo hints, *res,*p;
	int my_fd, new_fd,aux2;
	int client;
	
	//Variables for communication
	char data[10][10];
	Mensagem aux;
	char *msg = malloc(sizeof(Mensagem));
	int len_data;
	char buff[10];
	
	
	//Variables for the Backup sock
	int backup_sock=0,i,aux_backup;
	

	for(i=0;i<argc;i++) printf("%s\n",argv[i]);
	//Connect to the backup server
	if(argc==4){
		if((strcmp(argv[1],"-c"))==0){
			if((backup_sock=sincronize(argv[2], argv[3]))>0){
				printf("Connected to the backup at %s:%s\n",argv[2],argv[3]);
				for(i=0;i<10;i++){
					backup_copy(backup_sock,i,data[i],sizeof(char)*10);
				}
			}
			else{
				printf("Backup not found, exiting \n");
				return -1;
		} 
	} else {
		printf("argv[1] != -c,exiting\n");
		return -1;
		}
	}
	else printf("Local mode\n");
	
	aux_backup=backup_sock; //Pra caso o backup morra
	
	
	//Create unix sockets for client communication
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

	//Listen to it
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

	
	client++;
	printf("Server: My client %d is online \n", client );

	
	//Talk to me
	while(1){ 
	if((recv(new_fd, msg, sizeof(Mensagem),0))==0) {
		printf("My client disconnected, waiting for a new one\n");
		close(new_fd);
		break;
	}//Implementar verificação se a região esta fora do alcance aqui
	//Guardar o tamanho do buffer aqui, trocar os dados de uma string pra uma struct, pra poder mandar tanto inteiros como strings
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
			perror("send"); 
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
			int trash;	
				if(backup_sock>0){ //Se o backup morrer o sockfd vai a 0
				if((trash=backup_paste(backup_sock,aux.region,aux.dados,sizeof(aux.dados)))<0){
				printf("The backup died, i'm in local mode now\n");
				//return -1;
				}
		
			}
			if(aux_backup>0 && trash<0) printf("The backup died, i'm in local mode now\n"); // Não funciona ainda
			
		}
		
	
}

	close(new_fd);
}
	unlink(addr.sun_path);
	close(my_fd); //I don't want to listen anymore
	free(msg);
	exit(0);
	
}

int sincronize(char* addr, char* port){
    int sockfd, numbytes;  
    char buf[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    int aux2;
    struct sockaddr_in ip;
    char s[INET6_ADDRSTRLEN];

	printf("Connecting to backup at %s:%s\n",addr,port);
   	//Prepare structs
	memset(&hints,0,sizeof(hints));
	hints.ai_family = AF_INET; //Only IPv4 for me
	hints.ai_socktype = SOCK_STREAM;
	

	
	if((aux2=getaddrinfo(addr,port,&hints,&servinfo)) !=0){
		printf("getaddrinfo: \n");
		return -1;
	}

	//Make it, bind it, listen
	for(p= servinfo; p!=NULL; p=p->ai_next){
		if((sockfd=socket(p->ai_family, p->ai_socktype, p->ai_protocol))==-1){
			perror("backup: socket");
			continue;
		}

		if((connect(sockfd,p->ai_addr,p->ai_addrlen))== -1){
			close(sockfd);
			perror("backup:connect");
			continue;
		}
		break;
	}

	if(p==NULL){
		printf("server: failed to bind\n");
		exit(1);
	}
	free(servinfo);

	//inet_ntop(p->ai_family,&(((struct sockaddr_in *)p->ai_addr)->sin_addr),s, sizeof s);

	//printf("Connected to backup at %s\n", s);
	return sockfd;
}
int backup_copy(int clipboard_id, int region, void *buf, size_t count){
	Mensagem aux;
	char *msg = malloc(sizeof(Mensagem));
	aux.region=region;
	aux.oper=0;
	
	memcpy(msg,&aux,sizeof(Mensagem));
	send(clipboard_id,msg,sizeof(Mensagem),0);
	
	if((recv(clipboard_id,msg,sizeof(Mensagem),0))<0) return -1;
	memcpy(&aux,msg,sizeof(Mensagem));
	if((strcmp(aux.dados,"erro"))!=0){
	strcpy(buf,aux.dados);
	printf("Copied %s from the region %d of the backup\n",buf,region);}
	else {
		printf("Region %d is empty\n",region);
		return -1;
}
	
	free(msg);
	return strlen(buf);
}

int backup_paste(int clipboard_id, int region, void *buf, size_t count){
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
