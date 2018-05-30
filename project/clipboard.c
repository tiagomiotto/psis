#include <semaphore.h>
#include "clipboard.h"

typedef struct _clipboard
{
    size_t dataSize[10]; //dataSize[i] = sizeof clipboard[i]
    void *data[10];
} Clipboard_struct;

int sincronize(char *addr, char *port);
int backup_copy(int clipboard_id, int region, void *buf, size_t count);
int backup_paste(int clipboard_id, int region, void *buf, size_t count);
int create_unix_sock();
void *app_connect(void  *sock);
void *clipboard_handler(void *sock);
int create_inet_sock(char *port);
void *app_connection_handler(void  *sock);
void *clipboard_connection(void *sock);
int ppgtToParent(int clipboard_id, int region, void *buf, size_t count);
void remove_me(int sockfd);
void add_me(int sockfd);
void destroyCond (pthread_cond_t *cond);
int first_connection(int sock);
void kill_signal(int signo);
void add_my_thread(int sockfd, int *tds, int *tds_size);
int init_mutex();

Clipboard_struct initLocalCp(void);

int *c_sock;
size_t c_sock_size;
sig_atomic_t kill_flag = 0;
int clip_rcv = 0;
int region_rcv;
int *tdsa;
int tdsa_size = 1;
int *tdsc;

int tdsc_size = 1;

int cl = 0;
int backup_sock = 0;
pthread_mutex_t lock;
pthread_mutex_t lock_c;
pthread_mutex_t lockMsg;
pthread_mutex_t lockRecvs;
pthread_mutex_t lock_sig;
pthread_cond_t conditions[10];



Clipboard_struct clipboard;



int main(int argc, char **argv)
{
    unlink("127.0.0.1");
    struct sigaction psa;
    psa.sa_handler = kill_signal;
    sigaction(SIGINT, &psa, NULL);
    tdsc = malloc(sizeof(int));
    tdsa = malloc(sizeof(int));
    //Thread variables
    pthread_t client_thread, clip_thread, backup_thread;

    c_sock_size = 1;
    c_sock = malloc(sizeof(int));
    void **test;
    test = NULL;
    //*test=NULL;



    //Variables for communication
    int client;


    //Variables for the Backup sock
    int i, aux_backup;
    char port[10];
    srand(time(NULL));
    sprintf(port, "%d", (rand() % 64738 + 1024));

    int my_inet, my_unix;
    my_inet = create_inet_sock(port);
    my_unix = create_unix_sock();

    if((init_mutex()) != 0) return 1;



    //Connect to the backup server
    if(argc == 4)
    {
        if((strcmp(argv[1], "-c")) == 0)
        {
            if((backup_sock = sincronize(argv[2], argv[3])) > 0)
            {
                printf("Connected to the backup at %s:%s\n", argv[2], argv[3]);
                pthread_mutex_lock(&lock_c);
                c_sock[c_sock_size - 1] = backup_sock;
                pthread_mutex_unlock(&lock_c);
                pthread_create(&backup_thread, NULL, clipboard_handler, &backup_sock); //para o backup
                first_connection(backup_sock);

                /*for(i = 0; i < 10; i++)
                {
                    backup_paste(backup_sock, i, clipboard.data[i], 10);
                }*/
            }
            else
            {
                printf("Backup not found, exiting \n");
                return -1;
            }
        }
        else
        {
            printf("argv[1] != -c,exiting\n");
            return -1;
        }
    }
    else
    {
        printf("Local mode\n");
        clipboard = initLocalCp();
        c_sock[0] = 0;
    }

    if(my_unix > 0)pthread_create(&client_thread, NULL, app_connect, &my_unix);
    if(my_inet > 0)pthread_create(&clip_thread, NULL, clipboard_connection, &my_inet);

    pthread_join(clip_thread, test);
    pthread_join(client_thread, test);

    close(backup_sock);
    close(my_inet); //I don't want to listen anymore
    close(my_unix); //I don't want to listen anymore
    pthread_mutex_destroy(&lock);
    pthread_mutex_destroy(&lock_c);
    pthread_mutex_destroy(&lockMsg);
    pthread_mutex_destroy(&lockRecvs);
    destroyCond(conditions);
    free(test);
    free(tdsa);
    free(tdsc);
    printf("exiting\n");
    exit(0);

}
int init_mutex()
{
    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return 1;
    }
    if (pthread_mutex_init(&lock_c, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return 1;
    }
    if (pthread_mutex_init(&lockMsg, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return 1;
    }
    if (pthread_mutex_init(&lockRecvs, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return 1;
    }
    if (pthread_mutex_init(&lock_sig, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return 1;
    }
    return 0;
}

Clipboard_struct initLocalCp(void)
{
    Clipboard_struct cp;
    int i;
    for(i = 0; i < 10; i++)
    {
        cp.data[i] = NULL;
        cp.dataSize[i] = 0;
    }

    return cp;
}

void destroyCond (pthread_cond_t *cond)
{
    int i;
    for(i = 0; i < 10; i++)
    {
        pthread_cond_destroy(&conditions[i]);
    }
}

int sincronize(char *addr, char *port)
{
    int sockfd, numbytes;
    char buf[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    int aux2;
    struct sockaddr_in ip;
    char s[INET6_ADDRSTRLEN];

    printf("Connecting to backup at %s:%s\n", addr, port);
    //Prepare structs
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; //Only IPv4 for me
    hints.ai_socktype = SOCK_STREAM;



    if((aux2 = getaddrinfo(addr, port, &hints, &servinfo)) != 0)
    {
        printf("getaddrinfo: \n");
        return -1;
    }

    //Make it, bind it, listen
    for(p = servinfo; p != NULL; p = p->ai_next)
    {
        if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            perror("backup: socket");
            continue;
        }

        if((connect(sockfd, p->ai_addr, p->ai_addrlen)) == -1)
        {
            close(sockfd);
            perror("backup:connect");
            continue;
        }
        break;
    }

    if(p == NULL)
    {
        printf("server: failed to bind\n");
        exit(1);
    }
    free(servinfo);

    inet_ntop(p->ai_family, &(((struct sockaddr_in *)p->ai_addr)->sin_addr), s, sizeof s);

    printf("Connected to backup at %s\n", s);
    return sockfd;
}
int backup_paste(int clipboard_id, int region, void *buf, size_t count)
{
    int okFlag;
    Mensagem aux;
    void *msg = malloc(sizeof(Mensagem));
    void *data;


    aux.region = region;
    aux.oper = 0;
    printf("Chamei paste, região %d, tamanho %zu\n", region, count);



    memcpy(msg, &aux, sizeof(Mensagem));
    send(clipboard_id, msg, sizeof(Mensagem), 0);
    pthread_mutex_lock(&lockRecvs);
    recv(clipboard_id, msg, sizeof(Mensagem), 0); //Lê se o cliente tem espaço para receber a informação
    memcpy(&aux, msg, sizeof(Mensagem));

    if(aux.dataSize > 0)
    {
        data = malloc(aux.dataSize);
        if(data == NULL)
        {
            printf("Error allocating clipboard memory\n");
        }
        pthread_mutex_lock(&lock);
        clipboard.dataSize[region] = aux.dataSize;
        pthread_mutex_unlock(&lock);
        if((recv(clipboard_id, data, aux.dataSize, 0)) < 0)
        {
            free(msg);
            pthread_mutex_unlock(&lockRecvs);
            return -1;
        }
        pthread_mutex_unlock(&lockRecvs);
        pthread_mutex_lock(&lock);
        clipboard.data[aux.region] = data;
        pthread_mutex_unlock(&lock);
    }
    else   //position is empty or error
    {
        pthread_mutex_unlock(&lockRecvs);
        pthread_mutex_lock(&lock);
        clipboard.data[region] = NULL;
        pthread_mutex_unlock(&lock);
        return 0;
    }
    free(msg);
    return aux.dataSize;
}

int backup_copy(int clipboard_id, int region, void *buf, size_t count)
{
    Mensagem aux;
    void *msg = malloc(sizeof(Mensagem));
    void *data = malloc(count);
    int retorno, okFlag;

    printf("Vou mandar para o clipboard %d region %d message: %s\n", clipboard_id, region, (char *)buf);
    printf("count: %d\n", (int)count);
    aux.region = region;
    aux.oper = 1;
    aux.dataSize = count;
    memcpy(msg, &aux, sizeof(Mensagem));
    memcpy(data, buf, count);
    printf("teste\n");

    pthread_mutex_lock(&lockMsg);
    retorno = send(clipboard_id, msg, sizeof(Mensagem), MSG_NOSIGNAL); //informa o clipboard do tamanho da mensagem
    if(retorno <= 0)
    {
        printf("Problem sending info\n");
        remove_me(clipboard_id);
        free(msg);
        free(data);
        return -1;
    }
    if(aux.dataSize > 0)
    {
        printf("teste2\n");
        retorno = send(clipboard_id, data, aux.dataSize, MSG_NOSIGNAL); //envia a mensagem
        if(retorno <= 0)
        {
            printf("Problem sending data\n");
            remove_me(clipboard_id);
            free(msg);
            free(data);
            return -1;
        }
    }
    pthread_mutex_unlock(&lockMsg);
    printf("Paste complete %s :%d\n", (char *)data, (int)aux.dataSize);
    free(data);
    free(msg);
    return retorno;
}
int create_unix_sock()
{

    struct sockaddr_in their_addr;
    struct addrinfo hints, *res, *p;
    int my_fd;
    struct sockaddr_un addr;

    //Create unix sockets for client communication
    if((my_fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
    {
        printf("Socket unsuccessful\n");
        return -1;
    }
    int pid = getpid();
    char path[20];
    sprintf(path, "%s%d", SOCK_PATH, pid);
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, path);
    unlink(path);

    //chmod(SOCK_PATH, 0777);
    //chmod(path, 0777);

    if((bind (my_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr))) < 0)
    {
        printf("Bind unsuccessful\n");
        perror("bind: unix");
        unlink(SOCK_PATH);
        close(my_fd);
        return -1;
    }
    printf("Unix socket creation successfull at %s\n", path);
    return my_fd;
}
int create_inet_sock(char *port)
{
    //Variables for the unix socket (Talvez fazer isso dentro de uma função?)
    struct sockaddr_in their_addr;
    socklen_t addr_size;
    struct addrinfo hints, *res, *p;
    int my_fd, new_fd, aux2;
    char s[INET6_ADDRSTRLEN];


    //Prepare structs for the socket
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; //Only IPv4 for me AF_UNIX
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    unlink("127.0.0.1");
    if((aux2 = getaddrinfo(NULL, port, &hints, &res)) != 0)
    {
        printf("getaddrinfo: \n");
        return -1;
    }

    //Make it, bind it, listen
    for(p = res; p != NULL; p = p->ai_next)
    {
        if((my_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            perror("server: socket");
            continue;
        }

        if((bind(my_fd, p->ai_addr, p->ai_addrlen)) == -1)
        {
            close(my_fd);
            unlink("127.0.0.1");
            perror("server:bind");
            continue;
        }
        break;
    }

    if(p == NULL)
    {
        printf("server: failed to bind\n");
        return -1;
    }
    inet_ntop(p->ai_family, &(((struct sockaddr_in *)p->ai_addr)->sin_addr), s, sizeof s);
    free(res);
    printf("Backup at %s:%s\n", s, port);
    return my_fd;
}
void *app_connection_handler(void  *sock)
{
    //Talk to me
    void *data;
    int i;
    int aux_sockc;
    int new_fd = *(int *)sock;
    Mensagem aux;
    void *msg = malloc(sizeof(Mensagem));
    const int error = 0, success = 1;
    int client = new_fd;
    pthread_t propaga_thread;
    int kill_me;
    printf("App handler thread created for client %d\n", client);
    while(1)
    {
        pthread_mutex_lock(&lock_sig);
        kill_me = kill_flag;
        pthread_mutex_unlock(&lock_sig);
        if(kill_me == 1)
        {
            close(new_fd);
            //remove_my_thread(pthread_self(),tdsa,&tdsa_size);
            //pthread_mutex_unlock(&lock_c);
            pthread_exit(NULL);
        }
        if((recv(new_fd, msg, sizeof(Mensagem), 0)) == 0)
        {
            printf("My client disconnected, waiting for a new one\n");
            close(new_fd);
            break;
        }//Implementar verificação se a região esta fora do alcance aqui
        //Guardar o tamanho do buffer aqui, trocar os dados de uma string pra uma struct, pra poder mandar tanto inteiros como strings
        memcpy(&aux, msg, sizeof(Mensagem));


        if(aux.oper == 0) //Paste
        {

            if(clipboard.dataSize[aux.region] == 0)
            {
                //In case the region is empty

                if((send(new_fd, &error, sizeof(int), MSG_NOSIGNAL)) == -1)
                {
                    perror("send"); //Do I need the number of bytes?
                    printf("My client %d disconnected\n", client);
                    break;
                }
                printf("My client %d tried to copy from region %d, but it's empty\n", client, aux.region);
            }
            else if(aux.dataSize < clipboard.dataSize[aux.region]) //cliente não tem espaço
            {
                if((send(new_fd, &error, sizeof(int), MSG_NOSIGNAL)) == -1)
                {
                    perror("send");
                    printf("My client %d disconnected\n", client);
                    break;
                }
                printf("My client %d tried to copy from region %d, but he doesn't have enough space\n", client, aux.region);
            }
            else  //all good
            {
                if((send(new_fd, &success, sizeof(int), MSG_NOSIGNAL)) == -1)
                {
                    perror("send");
                    printf("My client %d disconnected\n", client);
                    break;
                }

                aux.dataSize = clipboard.dataSize[aux.region];

                memcpy(msg, &aux, sizeof(Mensagem));

                if((send(new_fd, msg, sizeof(aux), MSG_NOSIGNAL)) == -1)
                {
                    perror("send");
                    printf("My client disconnected\n");
                    break;
                }
                if((send(new_fd, clipboard.data[aux.region], aux.dataSize, MSG_NOSIGNAL)) == -1)
                {
                    perror("send");
                    printf("My client disconnected\n");
                    break;
                }
                printf("My friend %d read %s from region %d\n", client, (char *)clipboard.data[aux.region], aux.region);
            }
        }

        else if (aux.oper == 1 ) //Copy
        {
            int okFlag;
            data = malloc(aux.dataSize);
            if(data == NULL)
            {
                printf("ERROR ALOCATING MEMORY\n");
                okFlag = 0;
                send(new_fd, &okFlag, sizeof(int), 0);
            }
            else
            {
                okFlag = 1;
                send(new_fd, &okFlag, sizeof(int), MSG_NOSIGNAL);
                recv(new_fd, data, aux.dataSize, 0);
                printf("data: %s\n", (char *)data);


                if(c_sock[0] == 0)
                {
                    printf("sou pai\n");
                    pthread_mutex_lock(&lock);
                    printf("mutex at client %d locked\n", client);
                    if((memcmp(data, "", 1)) != 0)
                    {
                        free(clipboard.data[aux.region]);
                        clipboard.dataSize[aux.region] = aux.dataSize;
                        clipboard.data[aux.region] = data;
                    }
                    pthread_cond_broadcast(&conditions[aux.region]);
                    //Mutex pode desbloquear aqui
                    pthread_mutex_unlock(&lock);
                    printf("mutex at client %d unlocking\n", client);

                    //Propagate, copy to children
                    for (i = 1; i < c_sock_size; i++)
                    {
                        pthread_mutex_lock(&lock_c);
                        aux_sockc = c_sock[i];
                        pthread_mutex_unlock(&lock_c);
                        if (aux.region > 9)
                        {
                            printf("Deu erro aqui! (exiting)\n");
                            exit(0);
                        }
                        backup_copy(aux_sockc, aux.region, data, aux.dataSize);

                    }
                }
                else  //Propagate to top node, and it will propagate
                {
                    pthread_mutex_lock(&lock_c);
                    aux_sockc = c_sock[0];
                    pthread_mutex_unlock(&lock_c);
                    printf("não sou pai, c_sock[0] = %d\n", c_sock[0]);
                    ppgtToParent(aux_sockc, aux.region, data, aux.dataSize);
                }
            }

            printf("App %d pasted %s to region %d\n", new_fd, (char *)clipboard.data[aux.region], aux.region);
            //free(data);


        }
        else if(aux.oper == 2)   //Wait
        {

            printf("My client %d is waiting for the region %d\n", new_fd, aux.region);
            pthread_mutex_lock(&lock_c);
            pthread_cond_wait( &conditions[aux.region], &lock_c);
            pthread_mutex_unlock(&lock_c);
            printf("My client %d is no longer waiting for the region %d\n", new_fd, aux.region);
            if(clipboard.dataSize[aux.region] == 0)
            {
                //In case the region is empty

                if((send(new_fd, &error, sizeof(int), 0)) == -1)
                {
                    perror("send"); //Do I need the number of bytes?
                    printf("My client %d disconnected\n", client);
                    break;
                }
                printf("My client %d tried to copy from region %d, but it's empty\n", client, aux.region);
            }
            else if(aux.dataSize < clipboard.dataSize[aux.region]) //cliente não tem espaço
            {
                if((send(new_fd, &error, sizeof(int), 0)) == -1)
                {
                    perror("send");
                    printf("My client %d disconnected\n", client);
                    break;
                }
                printf("My client %d tried to copy from region %d, but he doesn't have enough space\n", client, aux.region);
            }
            else  //all good
            {
                if((send(new_fd, &success, sizeof(int), 0)) == -1)
                {
                    perror("send");
                    printf("My client %d disconnected\n", client);
                    break;
                }

                aux.dataSize = clipboard.dataSize[aux.region];

                memcpy(msg, &aux, sizeof(Mensagem));

                if((send(new_fd, msg, sizeof(aux), 0)) == -1)
                {
                    perror("send");
                    printf("My client disconnected\n");
                    break;
                }
                if((send(new_fd, clipboard.data[aux.region], aux.dataSize, 0)) == -1)
                {
                    perror("send");
                    printf("My client disconnected\n");
                    break;
                }
                printf("My friend %d waited and read %s from region %d\n", client, (char *)clipboard.data[aux.region], aux.region);
            }

        }
    }
    //remove_my_thread(pthread_self(),tdsa,&tdsa_size);
    close(new_fd);
    pthread_exit(NULL);
}
void *clipboard_connection(void *sock)
{
    //pthread_exit(NULL);
    int my_fd = *(int *)sock;
    int new_fd;
    struct sockaddr_in their_addr;
    pthread_t tid;
    socklen_t addr_size;
    fd_set readfds;
    int max_sd;
    int activity;
    int kill_me;
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 15000;


    printf("Clipboard thread created\n");
    //if(c_sock[0] > 0)pthread_create(&tid, NULL, clipboard_handler, &c_sock[0]); //para o backup
    if((listen(my_fd, MAX_CALLS)) == -1)
    {
        perror("listen");
        exit(1);
    }
    while(1)
    {
        pthread_mutex_lock(&lock_sig);
        kill_me = kill_flag;
        pthread_mutex_unlock(&lock_sig);
        if(kill_me == 1) break;
        //Para dar timeout no accept//
        FD_ZERO(&readfds);
        FD_SET(my_fd, &readfds);
        max_sd = my_fd;
        activity = select(max_sd + 1, &readfds, NULL, NULL, &tv);
        //printf("Server: Connect to me \n");
        //printf("select clip timed out\n");
        tv.tv_sec = 0;
        tv.tv_usec = 15000;
        //Connect it
        if(FD_ISSET(my_fd, &readfds))
        {
            addr_size = sizeof(their_addr);
            new_fd = accept(my_fd, (struct sockaddr *)&their_addr, &addr_size);
            if((new_fd) == -1)
            {
                perror("accept");
                continue;
            }
            else
            {
                add_me(new_fd);
                pthread_create(&tid, NULL, clipboard_handler, &new_fd);
                add_my_thread(tid, tdsc, &tdsc_size);
            }
        }
    }
    int i;
    for(i = 0; i < tdsc_size; i++)pthread_join(tdsc[i], NULL);
    printf("all apps are off\n");
    pthread_exit(NULL);
}
void *app_connect(void  *sock)
{
    //pthread_exit(NULL);
    int my_fd = *(int *)sock;
    int new_fd;
    struct sockaddr_in their_addr;
    pthread_t tid;
    socklen_t addr_size;
    //Variables for select
    fd_set readfds;
    int max_sd;
    int activity;
    struct timeval tv;
    //Variable for finalization
    int kill_me;
    tv.tv_sec = 0;
    tv.tv_usec = 15000;
    printf("App thread created\n");



    if((listen(my_fd, MAX_CALLS)) == -1)
    {
        perror("listen");
        exit(1);
    }
    printf("Listenning\n");

    while(1)
    {
        pthread_mutex_lock(&lock_sig);
        kill_me = kill_flag;
        pthread_mutex_unlock(&lock_sig);
        if(kill_me == 1) break;
        //Para dar timeout no accept
        FD_ZERO(&readfds);
        FD_SET(my_fd, &readfds);
        max_sd = my_fd;
        activity = select(max_sd + 1, &readfds, NULL, NULL, &tv);
        //printf("select timed out\n");
        tv.tv_sec = 0;
        tv.tv_usec = 15000;

        if(FD_ISSET(my_fd, &readfds))
        {
            addr_size = sizeof(their_addr);
            new_fd = accept(my_fd, NULL, NULL);
            if((new_fd) == -1)
            {
                perror("accept");
                pthread_exit(NULL);
            }
            else
            {

                pthread_create(&tid, NULL, app_connection_handler, &new_fd);
                cl++;
                add_my_thread(tid, tdsa, &tdsa_size);
                //continue;
            }
        }

    }

    int i;
    for(i = 0; i < tdsa_size; i++)pthread_join(tdsa[i], NULL);
    printf("all clipboards are off\n");
    pthread_exit(NULL);


}
void *clipboard_handler(void *sock)
{
    void *data, *data2;
    Mensagem aux;
    void *msg = malloc(sizeof(Mensagem));
    int new_fd = *(int *)sock;
    char data_aux[10][10];
    int i, j;
    int aux_sockc;
    int client = new_fd;
    size_t dataSizeToSend;
    pthread_t propaga_thread;
    int kill_me = 0;

    printf("Clipboard handler thread created\n");
    printf("Server: My at client %d is online \n", new_fd );

    //Talk to me
    while(1)  //Guardar o tamanho do buffer aqui, trocar os dados de uma string pra uma struct, pra poder mandar tanto inteiros como strings
    {
        pthread_mutex_lock(&lock_sig);
        kill_me = kill_flag;
        pthread_mutex_unlock(&lock_sig);
        if(kill_me == 1)
        {
            remove_me(new_fd);
            close(new_fd);
            //free(data2);
            //remove_my_thread(pthread_self(),tdsc,&tdsc_size);
            pthread_exit(NULL);
        }
        pthread_mutex_lock(&lockRecvs);
        if((recv(new_fd, msg, sizeof(Mensagem), 0)) == 0)
        {
            printf("My client disconnected, waiting for a new one\n");
            break;
        }
        pthread_mutex_unlock(&lockRecvs);


        memcpy(&aux, msg, sizeof(Mensagem));
        printf("pedido: %d, region: %d\n", aux.oper, aux.region);
        if(aux.region > 9)
        {
            printf("OUTOFBOUNDS! (exiting)\n");
            break;
        }

        if(aux.oper == 0) //Paste
        {
            if(clipboard.dataSize[aux.region] == 0)
            {
                //In case the region is empty
                aux.dataSize = clipboard.dataSize[aux.region];
                memcpy(msg, &aux, sizeof(Mensagem));
                pthread_mutex_lock(&lockMsg);
                if((send(new_fd, msg, sizeof(Mensagem), 0)) == -1)
                {
                    pthread_mutex_unlock(&lockMsg);
                    perror("send"); //Do I need the number of bytes?
                    printf("My client %d disconnected\n", client);
                    break;
                }
                pthread_mutex_unlock(&lockMsg);
                //printf("My client %d tried to copy from region %d, but it's empty\n",new_fd,aux.region);
            }

            else  //all good
            {
                pthread_mutex_lock(&lock);
                aux.dataSize = clipboard.dataSize[aux.region];
                data = malloc(aux.dataSize);
                memcpy(data, clipboard.data[aux.region], aux.dataSize);
                pthread_mutex_unlock(&lock);


                memcpy(msg, &aux, sizeof(Mensagem));
                pthread_mutex_lock(&lockMsg);
                if((send(new_fd, msg, sizeof(Mensagem), 0)) == -1)
                {
                    perror("send");
                    printf("My client %d disconnected\n", client);
                    pthread_mutex_unlock(&lockMsg);
                    break;
                }


                if((send(new_fd, data, aux.dataSize, 0)) == -1)
                {
                    perror("send");
                    printf("My client disconnected\n");
                    pthread_mutex_unlock(&lockMsg);
                    break;
                }
                pthread_mutex_unlock(&lockMsg);
                printf("My friend %d copied %s from region %d\n", client, (char *)clipboard.data[aux.region], aux.region);
                free(data);
            }
        }
        else if (aux.oper == 1 ) //Copy
        {
            data = malloc(aux.dataSize);   //ponteiro que vai ser atribuído à posição do clipboard
            data2 = malloc(aux.dataSize); //ponteiro para fazer memcpy
            printf("Recebi pedido de copy com tamanho %d, região %d, operação %d\n", (int)aux.dataSize, aux.region, aux.oper);
            /* if(data == NULL)
             {
                 printf("ERROR ALOCATING MEMORY\n");
                 exit(1);
             }
             else */if (1)
            {
                if(aux.dataSize > 0)recv(new_fd, data2, aux.dataSize, 0);
                pthread_mutex_unlock(&lockRecvs);
                printf("Recebi de outro cb, data: %s\n", (char *)data2);

                memcpy(data, data2, aux.dataSize);

                pthread_mutex_lock(&lock);
                printf("mutex locking\n");
                if((memcmp(data, "", 1)) != 0)
                {
                    free(clipboard.data[aux.region]);

                    clipboard.dataSize[aux.region] = aux.dataSize;
                    clipboard.data[aux.region] = data;
                }
                pthread_cond_broadcast(&conditions[aux.region]);
                pthread_mutex_unlock(&lock);
                printf("mutex unlocked\n");

                for (i = 1; i < c_sock_size; i++)
                {
                    //CB recebem sempre paste de cima, por isso podem propagar sempre
                    pthread_mutex_lock(&lock_c);
                    aux_sockc = c_sock[i];
                    pthread_mutex_unlock(&lock_c);
                    backup_copy(aux_sockc, aux.region, data2, aux.dataSize);

                }

            }


            printf("Another clipboard %d pasted %s to region %d\n", client, (char *)clipboard.data[aux.region], aux.region);
            free(data);
            free(data2);

        }
        else if(aux.oper == 2) //propagation to parent
        {

            data = malloc(aux.dataSize);
            printf("vou propagar informação para o meu pai com tamanho %d, região %d, operação %d\n", (int)aux.dataSize, aux.region, aux.oper);
            if(data == NULL)
            {
                printf("ERROR ALOCATING MEMORY\n");
                exit(1);
            }
            else
            {
                recv(new_fd, data, aux.dataSize, 0);
                printf("Recebi de outro cb, data: %s\n", (char *)data);
                if(c_sock[0] != 0)
                {
                    pthread_mutex_lock(&lock_c);
                    aux_sockc = c_sock[0];
                    pthread_mutex_unlock(&lock_c);
                    ppgtToParent(aux_sockc, aux.region, data, aux.dataSize);
                }
                else
                {
                    pthread_mutex_lock(&lock);
                    free(clipboard.data[aux.region]);
                    //Mutex lock aqui
                    clipboard.dataSize[aux.region] = aux.dataSize;
                    clipboard.data[aux.region] = data;
                    pthread_cond_broadcast(&conditions[aux.region]);
                    pthread_mutex_unlock(&lock);
                    for (i = 1; i < c_sock_size; i++)
                    {
                        //CB recebem sempre paste de cima, por isso podem propagar sempre
                        pthread_mutex_lock(&lock_c);
                        aux_sockc = c_sock[i];
                        pthread_mutex_unlock(&lock_c);
                        backup_copy(aux_sockc, aux.region, data, aux.dataSize);
                    }
                }

            }
            free(data);
        }
        else if (aux.oper == 3)
        {
            //Locks grandes
            printf("First connect from %d\n", new_fd);
            pthread_mutex_lock(&lock);
            for(i = 0; i < 10; i++)
            {
                backup_copy(new_fd, i, clipboard.data[i], clipboard.dataSize[i]);
                if(i == 9) break;
            }
            pthread_mutex_unlock(&lock);
            printf("First connect complete\n");
        }

        //printf("mutex unlocking\n");

    }
    //remove_my_thread(pthread_self(),tdsc,&tdsc_size);
    remove_me(new_fd);
    close(new_fd);
    free(data2);

    pthread_exit(NULL);
}
int ppgtToParent(int clipboard_id, int region, void *buf, size_t count)
{
    int retorno;
    Mensagem aux;
    void *msg = malloc(sizeof(Mensagem));
    void *data = malloc(count);

    aux.region = region;
    aux.oper = 2;
    aux.dataSize = count;
    memcpy(msg, &aux, sizeof(Mensagem));
    retorno = send(clipboard_id, msg, sizeof(Mensagem), 0); //informa o clipboard do tamanho da mensagem
    if(retorno <= 0)
    {
        printf("Problem sending info (ppgtToParent)\n");
        remove_me(clipboard_id);
        free(msg);
        free(data);
        return -1;
    }
    memcpy(data, buf, count);
    retorno = send(clipboard_id, data, aux.dataSize, 0); //envia a mensagem
    if(retorno <= 0)
    {
        printf("Problem sending data\n");
        remove_me(clipboard_id);
        close(clipboard_id);
        free(msg);
        free(data);
        return -1;
    }
    printf("propagation complete %s :%d\n", (char *)data, (int)aux.dataSize);
    free(data);
    free(msg);
    return retorno;//propaga informação para cima sem escrever
}

void remove_me(int sockfd)
{
    int i, j;
    printf("Removing clip at %d\n", sockfd );
    pthread_mutex_lock(&lock_c);

    if(sockfd == c_sock[0])
        c_sock[0] = 0;
    else
    {
        for (i = 0; i < c_sock_size; ++i)
            if(c_sock[i] == sockfd)
            {
                for(j = i; j < c_sock_size - 1; j++)
                    c_sock[j] = c_sock[j + 1];
                break;
            }
        c_sock = realloc(c_sock, (c_sock_size - 1) * sizeof(int));
        c_sock_size--;
    }


    pthread_mutex_unlock(&lock_c);
    return;
}

void add_me(int sockfd)
{
    printf("Adding new clip at %d\n", sockfd );
    pthread_mutex_lock(&lock_c);
    c_sock = realloc(c_sock, (c_sock_size + 1) * sizeof(int));
    c_sock_size++;
    c_sock[c_sock_size - 1] = sockfd;
    pthread_mutex_unlock(&lock_c);
    return;
}
int first_connection(int clipboard_id)
{

    Mensagem aux;
    void *msg = malloc(sizeof(Mensagem));

    aux.region = 0;
    aux.oper = 3;
    printf("Chamei first_connection\n");


    memcpy(msg, &aux, sizeof(Mensagem));
    memcpy(msg, &aux, sizeof(Mensagem));
    send(clipboard_id, msg, sizeof(Mensagem), 0);
    free(msg);
    return 1;
}
void kill_signal(int signo)
{

    pthread_mutex_lock(&lock_sig);
    kill_flag = 1;
    pthread_mutex_unlock(&lock_sig);
    pthread_mutex_lock(&lock_sig);

    pthread_mutex_unlock(&lock_sig);
    return;

}
void add_my_thread(int sockfd, int *tds, int *tds_size)
{
    printf("Adding new thread at %d\n", sockfd );

    tds = realloc(tds, (*tds_size + 1) * sizeof(int));
    *tds_size++;
    tds[*tds_size - 1] = sockfd;

    return;
}

