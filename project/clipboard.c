
#include "clipboard.h"

typedef struct _clipboard {
	size_t dataSize[10]; //dataSize[i] = sizeof clipboard[i]
	void *data[10];
} Clipboard_struct;

int sincronize(char* addr, char* port);
int backup_copy(int clipboard_id, int region, void *buf, size_t count);
int backup_paste(int clipboard_id, int region, void *buf, size_t count);
int create_unix_sock();
void *app_connect(void*  sock);
void *clipboard_handler(void* sock);
int create_inet_sock(char* port);
void *app_connection_handler(void*  sock);
void *clipboard_connection(void* sock);
void *propagation_handler();
void remove_me(int sockfd);
void add_me(int sockfd);

Clipboard_struct initLocalCp(void);

int* c_sock;
size_t c_sock_size;



int cl=0;
int backup_sock=0;
pthread_mutex_t lock;
pthread_mutex_t lock_c;

	Clipboard_struct clipboard; 
	Mensagem aux;
	void *msg;
	int len_data;
	char buff[10];


int main(int argc, char **argv){
	unlink("127.0.0.1");

	
	//Thread variables
	pthread_t client_thread,clip_thread,propaga_thread;
	msg = malloc(sizeof(Mensagem));
	c_sock_size=1;
	c_sock= malloc(sizeof(int));


	
	//Variables for communication
	int client;
	
	
	//Variables for the Backup sock
	int i,aux_backup;
	char port[10];
	srand(time(NULL));
	sprintf(port,"%d",(rand()%64738+1024));

	int my_inet,my_unix;
	my_inet=create_inet_sock(port);
	my_unix=create_unix_sock();
	
	if (pthread_mutex_init(&lock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return 1;
    }
    
	for(i=0;i<argc;i++) printf("%s\n",argv[i]);
	//Connect to the backup server
	if(argc==4){
		if((strcmp(argv[1],"-c"))==0){
			if((backup_sock=sincronize(argv[2], argv[3]))>0){

				printf("Connected to the backup at %s:%s\n",argv[2],argv[3]);
					c_sock[c_sock_size-1]=backup_sock;
				for(i=0;i<10;i++){

					backup_copy(backup_sock,i,clipboard.data[i],sizeof(char)*10);
					printf("data[%d]: %s\n", i, (char *)clipboard.data[i]);
				}
			}
			else{
				printf("Backup not found, exiting \n");
				return -1;
			} 
		}else {
			printf("argv[1] != -c,exiting\n");
			return -1;
		}
	}
	else {
		printf("Local mode\n");
		clipboard = initLocalCp();
	}
	
	if(my_unix>0)pthread_create(&client_thread,NULL,app_connect,&my_unix);
	if(my_inet>0)pthread_create(&clip_thread,NULL,clipboard_connection,&my_inet);
	pthread_create(&propaga_thread,NULL,propagation_handler,NULL);
	pthread_join(clip_thread, NULL);
	pthread_join(client_thread, NULL);
	pthread_join(propaga_thread, NULL);
	//Fazer signhandler pra matar todas as threads no CTRL+C
	//descobrir como matar todas as threads sem guardar os ids
	//pthread_cleanup_push(3)?
	//unlink(addr.sun_path);
	close(my_inet); //I don't want to listen anymore
	close(my_unix); //I don't want to listen anymore
	pthread_mutex_destroy(&lock);
	free(msg);
	exit(0);
	
}

Clipboard_struct initLocalCp(void){
	Clipboard_struct cp;
	int i;
	for(i=0; i<10; i++){
		cp.data[i] = NULL;
		cp.dataSize[i] = 0;
	}

	return cp;
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

	int okFlag;
	Mensagem aux;
	void *msg = malloc(sizeof(Mensagem));
	aux.region=region;
	aux.oper=0;
	
	memcpy(msg,&aux,sizeof(Mensagem));
	send(clipboard_id,msg,sizeof(Mensagem),0);

	recv(clipboard_id, msg, sizeof(Mensagem), 0); //Lê se o cliente tem espaço para receber a informação
	memcpy(&aux, msg, sizeof(Mensagem));
	printf("allocSize: %zu\n", aux.dataSize);

	if(aux.dataSize > 0){
		clipboard.data[region] = malloc(aux.dataSize);
		if(clipboard.data[region] == NULL){
			printf("Error allocating clipboard memory\n");
		}
		clipboard.dataSize[region] = aux.dataSize;
		if((recv(clipboard_id, clipboard.data[region], aux.dataSize, 0))<0){
			free(msg);
			return -1;
		} 
		
	}else{ //position is empty or error
		clipboard.data[region] = NULL;
		printf("position empty\n");
		return 0;
	}
	free(msg);
	return aux.dataSize;
}

int backup_paste(int clipboard_id, int region, void *buf, size_t count){
	Mensagem aux;
	void *msg = malloc(sizeof(Mensagem));
	void *data = malloc(count);
	int retorno, okFlag;
	if((strcmp(buf,""))==0 || (strcmp(buf,"\n"))==0){
		printf("You can't paste an empty line\n");
		free(data);
		free(msg);
		return -1;
	}
	printf("message: %s\n", (char *)buf);
	printf("count: %d\n", (int)count);
	aux.region=region;
	aux.oper=1;
	aux.dataSize = count;
	memcpy(msg,&aux,sizeof(Mensagem));
	retorno=send(clipboard_id,msg,sizeof(Mensagem),0); //informa o clipboard do tamanho da mensagem
	if(retorno<=0){
		printf("Problem sending info\n");
		remove_me(clipboard_id);
		free(msg);
		free(data);
		return -1;
	}
	recv(clipboard_id, &okFlag, sizeof(int), 0); //recebe o OK do clipboard depois de ter alocado memória para receber a mensagem
	if (!okFlag){
		printf("Problem with clipboard\n");
		remove_me(clipboard_id);
		free(msg);
		free(data);		
		return -1;
	}
	memcpy(data, buf, count);
	retorno=send(clipboard_id, data, aux.dataSize, 0); //envia a mensagem
	if(retorno<=0){
		printf("Problem sending data\n");
		remove_me(clipboard_id);
		free(msg);
		free(data);
		return -1;
	}
	free(data);
	free(msg);
	return retorno;
}


int create_unix_sock(){

	struct sockaddr_in their_addr;
	struct addrinfo hints, *res,*p;
	int my_fd;
	struct sockaddr_un addr;

		//Create unix sockets for client communication
	if((my_fd=socket(AF_UNIX,SOCK_STREAM,0))<0){
		printf("Socket unsuccessful\n");
		return -1;
	}
	int pid= getpid();
	char path[20];
	sprintf(path,"%s_%d",SOCK_PATH,pid);
	memset(&addr,0,sizeof(addr));
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, path);
	unlink(path);
		
	chmod(SOCK_PATH, 0777);
	chmod(path, 0777);
		
	if((bind (my_fd, (struct sockaddr*)&addr, sizeof(struct sockaddr)))<0){
		printf("Bind unsuccessful\n");
		perror("bind: unix");
		unlink(SOCK_PATH);
		close(my_fd);
		return -1;
	}
	printf("Unix socket creation successfull at %s\n", path);
	return my_fd;
}
int create_inet_sock(char* port){
	//Variables for the unix socket (Talvez fazer isso dentro de uma função?)
	struct sockaddr_in their_addr;
	socklen_t addr_size;
	struct addrinfo hints, *res,*p;
	int my_fd, new_fd,aux2;
	
	//Prepare structs for the socket
	memset(&hints,0,sizeof(hints));
	hints.ai_family = AF_INET; //Only IPv4 for me AF_UNIX
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags= AI_PASSIVE;
	unlink("127.0.0.1");
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
		return -1;
	}
	free(res);
	printf("Backup at %s:%s\n", SOCK_ADDR,port);
	return my_fd;
}
void *app_connection_handler(void*  sock){
	//Talk to me
	void *data;
	int new_fd = *(int*)sock;
	const int error = 0, success = 1;
	int client=cl;
	printf("App handler thread created\n");
	while(1){ 
		if((recv(new_fd, msg, sizeof(Mensagem), 0))==0) {
			printf("My client disconnected, waiting for a new one\n");
			close(new_fd);
			break;
		}//Implementar verificação se a região esta fora do alcance aqui
		//Guardar o tamanho do buffer aqui, trocar os dados de uma string pra uma struct, pra poder mandar tanto inteiros como strings
		memcpy(&aux,msg,sizeof(Mensagem));
		printf("mutex at client %d locking\n",client);
		pthread_mutex_lock(&lock);
		
		if(aux.oper==0) //Copy
		{

			if(clipboard.dataSize[aux.region]==0){
				//In case the region is empty
				
				if((send(new_fd, &error, sizeof(int), 0))==-1){
					perror("send"); //Do I need the number of bytes?
					printf("My client %d disconnected\n", client);
				break;
				}	
				printf("My client %d tried to copy from region %d, but it's empty\n",client,aux.region);
			} 
			else if(aux.dataSize < clipboard.dataSize[aux.region]){//cliente não tem espaço
				if((send(new_fd, &error, sizeof(int), 0))==-1){
					perror("send");
					printf("My client %d disconnected\n", client);
				break;
				}	
				printf("My client %d tried to copy from region %d, but he doesn't have enough space\n",client,aux.region);
			}			
			else{ //all good
				if((send(new_fd, &success, sizeof(int), 0))==-1){
					perror("send");
					printf("My client %d disconnected\n", client);
					break;
				}

				aux.dataSize = clipboard.dataSize[aux.region];
				
				memcpy(msg,&aux,sizeof(Mensagem));

				if((send(new_fd, msg, sizeof(aux), 0))==-1){
					perror("send"); 
					printf("My client disconnected\n");
					break;
				}
				if((send(new_fd, clipboard.data[aux.region], aux.dataSize, 0))==-1){
					perror("send"); 
					printf("My client disconnected\n");
					break;
				}
				printf("My friend %d copied %s from region %d\n",client,(char *)clipboard.data[aux.region], aux.region);
			}
		}
		else if (aux.oper ==1 ) //Paste
		{	
			int okFlag;
			data = malloc(aux.dataSize);
			if(data == NULL){
				printf("ERROR ALOCATING MEMORY\n");
				okFlag = 0;
				send(new_fd, &okFlag, sizeof(int), 0);
			}
			else{
				okFlag = 1;
				send(new_fd, &okFlag, sizeof(int), 0);
				recv(new_fd, data, aux.dataSize, 0);
				printf("data: %s\n", (char *)data);
				free(clipboard.data[aux.region]);
				clipboard.dataSize[aux.region] = aux.dataSize;
				clipboard.data[aux.region] = data;
			}
			
			printf("My friend %d pasted %s to region %d\n",client,(char *)clipboard.data[aux.region],aux.region);

		}
		pthread_mutex_unlock(&lock);
		printf("mutex at client %d unlocking\n",client);				
		
	}

	close(new_fd);
	pthread_exit(NULL);
}
void *clipboard_connection(void* sock){
	int my_fd = *(int*)sock;
	int new_fd;
	struct sockaddr_in their_addr;
	pthread_t tid;
	socklen_t addr_size;
	printf("Clipboard thread created\n");
	if(c_sock[0]>0)pthread_create(&tid,NULL,clipboard_handler,&c_sock[0]); //para o backup
	if((listen(my_fd, MAX_CALLS)) == -1){
		perror("listen");
		exit(1);
	}
	while(1){

	//printf("Server: Connect to me \n");
	
	//Connect it
	addr_size= sizeof(their_addr);
	new_fd=accept(my_fd, (struct sockaddr *)&their_addr,&addr_size);
	if((new_fd)==-1){
		perror("accept");
		continue;
	}
	else {
		add_me(new_fd);
		pthread_create(&tid,NULL,clipboard_handler,&new_fd);
	}
	}
	
	 pthread_join(tid, NULL);
	 pthread_exit(NULL);
}
void *app_connect(void*  sock){

	int my_fd = *(int*)sock;
	int new_fd;
	struct sockaddr_in their_addr;
	pthread_t tid;
	socklen_t addr_size;
	printf("App thread created\n");
	
	
	if((listen(my_fd, MAX_CALLS)) == -1){
		perror("listen");
		exit(1);
	}
	printf("Listenning\n");

	while(1){
	//Listen to it
	//Connect it
	addr_size= sizeof(their_addr);
	new_fd=accept(my_fd, NULL,NULL);
	if((new_fd)==-1){
		perror("accept");
		pthread_exit(NULL);}
	else{
		
		pthread_create(&tid,NULL,app_connection_handler,&new_fd);
		cl++;
		//continue;
	}
	}
	printf("a\n");
	pthread_join(tid, NULL);
	pthread_exit(NULL);
	//*sock=new_fd;

}
void *clipboard_handler(void* sock){ //Falta arrumar aqui
	void *data;
	int new_fd = *(int*)sock;
	char data_aux[10][10];
	int i,j, okFlag;
	int client;
	size_t dataSizeToSend;
	
	printf("Clipboard handler thread created\n");
	printf("Server: My at client %d is online \n", cl );
	


	//Talk to me
	while(1){ //Guardar o tamanho do buffer aqui, trocar os dados de uma string pra uma struct, pra poder mandar tanto inteiros como strings

		
		if((recv(new_fd, msg, sizeof(Mensagem),0))==0) {
			printf("My client disconnected, waiting for a new one\n");
			break;
		}
		

		memcpy(&aux,msg,sizeof(Mensagem));
		
		if(aux.oper==0) //Copy
		{

			if(clipboard.dataSize[aux.region]==0){
				//In case the region is empty
				aux.dataSize = clipboard.dataSize[aux.region];
				printf("empty okFlag: %d\n", (int)aux.dataSize);
				memcpy(msg,&aux,sizeof(Mensagem));
				if((send(new_fd, msg, sizeof(Mensagem), 0))==-1){
					perror("send"); //Do I need the number of bytes?
					printf("My client %d disconnected\n", client);
					break;
				}	
				printf("My client %d tried to copy from region %d, but it's empty\n",client,aux.region);
			} 
			/*else if(aux.dataSize < clipboard.dataSize[aux.region]){//cliente não tem espaço
				if((send(new_fd, &error, sizeof(int), 0))==-1){
					perror("send");
					printf("My client %d disconnected\n", client);
					break;
				}	
				printf("My client %d tried to copy from region %d, but he doesn't have enough space\n",client,aux.region);
			}*/			
			else{ //all good
				printf("Tou a mandar success\n");
				aux.dataSize = clipboard.dataSize[aux.region];
				printf("okFlag: %d\n", (int)aux.dataSize);
				memcpy(msg,&aux,sizeof(Mensagem));
				if((send(new_fd, msg, sizeof(Mensagem), 0))==-1){
					perror("send");
					printf("My client %d disconnected\n", client);
					break;
				}

				aux.dataSize = clipboard.dataSize[aux.region];
				
				memcpy(msg,&aux,sizeof(Mensagem));

				if((send(new_fd, clipboard.data[aux.region], aux.dataSize, 0))==-1){
					perror("send"); 
					printf("My client disconnected\n");
					break;
				}
				/*if((send(new_fd, clipboard.data[aux.region], aux.dataSize, 0))==-1){
					perror("send"); 
					printf("My client disconnected\n");
					break;
				}*/
				printf("My friend %d copied %s from region %d\n",client,(char *)clipboard.data[aux.region], aux.region);
			}
		}
		else if (aux.oper ==1 ) //Paste
		{	
			int okFlag;
			data = malloc(aux.dataSize);
			if(data == NULL){
				printf("ERROR ALOCATING MEMORY\n");
				okFlag = 0;
				send(new_fd, &okFlag, sizeof(int), 0);
			}
			else{
				pthread_mutex_lock(&lock);
				printf("mutex locking\n");
				okFlag = 1;
				send(new_fd, &okFlag, sizeof(int), 0);
				recv(new_fd, data, aux.dataSize, 0);
				printf("data: %s\n", (char *)data);
				free(clipboard.data[aux.region]);
				clipboard.dataSize[aux.region] = aux.dataSize;
				clipboard.data[aux.region] = data;
			}
			
			printf("My friend %d pasted %s to region %d\n",client,(char *)clipboard.data[aux.region],aux.region);

		}
		pthread_mutex_unlock(&lock);
		printf("mutex unlocking\n");				
		
	}
	remove_me(new_fd);
	close(new_fd);

	pthread_exit(NULL);
}
void *propagation_handler(){
	Clipboard_struct clipboard_cpy= clipboard;

	while(1){
		int i,j;
		for (i = 0; i < 10; ++i) //entra num ciclo infinito de sends
		{
			if(clipboard_cpy.data[i]!=clipboard.data[i]){ //Ponteiros alterados apontam pra outra memoria
				for(j=0;j<c_sock_size;j++) backup_paste(c_sock[j],i,clipboard.data[i],clipboard.dataSize[i]);
				//free(clipboard_cpy.data[i]);
				clipboard_cpy.data[i]=clipboard.data[i];
			}
		}
	} 
}
void remove_me(int sockfd){
	int i,j;
	pthread_mutex_lock(&lock_c);
	for (i = 0; i < c_sock_size; ++i) if(c_sock[i]==sockfd) break;
	else for(j=i;j<c_sock_size-1;j++) c_sock[i]=c_sock[i+1];
	c_sock= realloc(c_sock, (c_sock_size-1)*sizeof(int));
	c_sock_size--;
	pthread_mutex_unlock(&lock_c);

}

void add_me(int sockfd){
	
	pthread_mutex_lock(&lock_c);
	c_sock= realloc(c_sock, (c_sock_size+1)*sizeof(int));
	c_sock_size++;
	c_sock[c_sock_size-1]=sockfd;
	pthread_mutex_unlock(&lock_c);
}


