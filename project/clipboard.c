#include <semaphore.h>
#include "clipboard.h"

typedef struct _clipboard
{
    size_t dataSize[10];
    void *data[10];
} Clipboard_struct;

int sincronize(char *addr, char *port);
int backup_copy(int clipboard_id, int region, void *buf, size_t count);
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
int recvAll(int fd, void *data, size_t count);

Clipboard_struct initLocalCp(void);

int *c_sock;
size_t c_sock_size;
sig_atomic_t kill_flag = 0; //Flag for SIGINT handling
int clip_rcv = 0;
int region_rcv;
int *tdsa;
int tdsa_size = 1;
int *tdsc;

int tdsc_size = 1;

int cl = 0;
int backup_sock = 0;
pthread_rwlock_t cbLock;
pthread_mutex_t lock_c;
pthread_mutex_t lockMsg;
pthread_mutex_t lock_sig;
pthread_cond_t conditions[10];



Clipboard_struct clipboard;



int main(int argc, char **argv)
{

    //Handle CTRL C
    struct sigaction psa = {0};
    psa.sa_handler = kill_signal;
    sigaction(SIGINT, &psa, NULL);

    //Thread variables
    pthread_t client_thread, clip_thread, backup_thread;

    ///Start sockets array
    c_sock_size = 1;
    c_sock = malloc(sizeof(int));

    //Variable to avoid memory leak on pthread join
    void **test;
    test = NULL;



    //Variables for the Backup sock
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
    if(my_inet>0  && my_unix> 0)pthread_create(&clip_thread, NULL, clipboard_connection, &my_inet);

    if(my_inet > 0 && my_unix>0)pthread_join(clip_thread, test);
    if(my_unix > 0)pthread_join(client_thread, test);
    else printf("Unix Sock failed, exiting\n");
    close(backup_sock);
    close(my_inet); //I don't want to listen anymore
    close(my_unix); //I don't want to listen anymore
    pthread_mutex_destroy(&lock_c);
    pthread_mutex_destroy(&lockMsg);
    pthread_rwlock_destroy(&cbLock);
    destroyCond(conditions);
    unlink(SOCK_PATH);
    free(test);
    printf("exiting\n");
    exit(0);

}
/*****************************************************************************/
/* init_mutex. Initialize the needed mutexes                                 */
/*****************************************************************************/
int init_mutex()
{
    if(pthread_rwlock_init(&cbLock, NULL) != 0)
    {
        printf("\n RWlock init failed\n");
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
    if (pthread_mutex_init(&lock_sig, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return 1;
    }
    return 0;
}
/*****************************************************************************/
/* initLocalCp. Initialize an empty clipboard .                              */
/*****************************************************************************/
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
/*****************************************************************************/
/* destroyCond. Destroy all the phtread_conditions .                         */
/*****************************************************************************/
void destroyCond (pthread_cond_t *cond)
{
    int i;
    for(i = 0; i < 10; i++)
    {
        pthread_cond_destroy(&conditions[i]);
    }
}
/*****************************************************************************/
/* sincronize. Creates a socket to comunicate with the top clipboard         */
/*****************************************************************************/
int sincronize(char *addr, char *port)
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int aux2;
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

    //printf("Connected to backup at %s\n", s);
    return sockfd;
}
/*****************************************************************************/
/* backup_copy. Sends a copy request to another clipboard .                  */
/*****************************************************************************/
int backup_copy(int clipboard_id, int region, void *buf, size_t count)
{
    Mensagem aux;
    void *msg = malloc(sizeof(Mensagem));


    int retorno;

    aux.region = region;
    aux.oper = 1;
    aux.dataSize = count;
    memcpy(msg, &aux, sizeof(Mensagem));


    pthread_mutex_lock(&lockMsg);
    retorno = send(clipboard_id, msg, sizeof(Mensagem), MSG_NOSIGNAL); //informa o clipboard do tamanho da mensagem
    if(retorno < 0)
    {
        pthread_mutex_unlock(&lockMsg);
        printf("Problem sending info\n");
        remove_me(clipboard_id);
        free(msg);
        return -1;
    }
    if(aux.dataSize > 0)
    {
        retorno = send(clipboard_id, buf, aux.dataSize, MSG_NOSIGNAL); //envia a mensagem
        if(retorno < 0)
        {
            pthread_mutex_unlock(&lockMsg);
            printf("Problem sending data\n");
            remove_me(clipboard_id);
            free(msg);
            return -1;
        }
    }
    pthread_mutex_unlock(&lockMsg);
    free(msg);
    return retorno;
}
/*****************************************************************************/
/* create_unix_sock. Creates unix socket for local communication with apps   */
/*****************************************************************************/
int create_unix_sock()
{


    int my_fd;
    struct sockaddr_un addr;

    //Create unix sockets for client communication
    if((my_fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
    {
        printf("Socket unsuccessful\n");
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SOCK_PATH);




    if((bind (my_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un))) < 0)
    {
        printf("Bind unsuccessful\n");
        perror("bind: unix");
        unlink(SOCK_PATH);
        close(my_fd);
        return -1;
    }
    printf("Unix socket creation successfull at %s\n", addr.sun_path);
    return my_fd;
}
/*****************************************************************************/
/* create_inet_sock. Creates inet socket for communication with clipboards   */
/*****************************************************************************/
int create_inet_sock(char *port)
{
    //Variables for inet sock
    struct addrinfo hints, *res, *p;
    int my_fd, aux2;
    char s[INET6_ADDRSTRLEN];


    //Prepare structs for the socket
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
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
    printf("Socket for other clipboards at %s:%s\n", s, port);
    return my_fd;
}
/*****************************************************************************/
/* app_connection_handle. Handler for the app communication thread           */
/*****************************************************************************/
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
    int kill_me;
    printf("App handler thread created for app %d\n", client);
    while(1)
    {
        pthread_mutex_lock(&lock_sig);
        kill_me = kill_flag;
        pthread_mutex_unlock(&lock_sig);
        if(kill_me == 1)
        {
            break;
        }
        if((recv(new_fd, msg, sizeof(Mensagem), 0)) <= 0)
        {
            printf("App %d disconnected, waiting for a new one\n",client);
            break;
        }
        memcpy(&aux, msg, sizeof(Mensagem));

        if(aux.region > 9 || aux.region < 0)
        {
            printf("App %d tried to access invalid region\n",client);
            break;
        }
        if(aux.oper == 0) //Paste
        {
            pthread_rwlock_rdlock(&cbLock);
            if(clipboard.dataSize[aux.region] == 0)
            {
                //In case the region is empty

                if((send(new_fd, &error, sizeof(int), MSG_NOSIGNAL)) == -1)
                {
                    perror("send");
                    printf("App %d disconnected\n", client);
                    break;
                }
                printf("App %d tried to copy from region %d, but it's empty\n", client, aux.region);
            }
            else if(aux.dataSize < clipboard.dataSize[aux.region]) //cliente não tem espaço
            {
                if((send(new_fd, &error, sizeof(int), MSG_NOSIGNAL)) == -1)
                {
                    perror("send");
                    printf("App %d disconnected\n", client);
                    break;
                }
                printf("App %d tried to copy from region %d, but he doesn't have enough space\n", client, aux.region);
            }
            else  //all good
            {
                if((send(new_fd, &success, sizeof(int), MSG_NOSIGNAL)) == -1)
                {
                    perror("send");
                    printf("App %d disconnected\n", client);
                    break;
                }

                aux.dataSize = clipboard.dataSize[aux.region];

                memcpy(msg, &aux, sizeof(Mensagem));

                if((send(new_fd, msg, sizeof(aux), MSG_NOSIGNAL)) == -1)
                {
                    perror("send");
                    printf("App %d disconnected\n",client);
                    break;
                }
                if((send(new_fd, clipboard.data[aux.region], aux.dataSize, MSG_NOSIGNAL)) == -1)
                {
                    perror("send");
                    printf("App %d disconnected\n",client);
                    break;
                }
                printf("App %d read %s from region %d\n", client, (char *)clipboard.data[aux.region], aux.region);
            }
            pthread_rwlock_unlock(&cbLock);
        }

        else if (aux.oper == 1 ) //Copy
        {
            int okFlag;
            data = malloc(aux.dataSize);
            if(data == NULL)
            {
                printf("ERROR ALOCATING MEMORY\n");
                okFlag = 0;
                if(send(new_fd, &okFlag, sizeof(int), MSG_NOSIGNAL) == -1)
                {
                    perror("send");
                    printf("App %d disconnected\n",client);
                    break;
                }
            }
            else
            {
                okFlag = 1;
                if(send(new_fd, &okFlag, sizeof(int), MSG_NOSIGNAL) == -1)
                {
                    perror("send");
                    printf("App %d disconnected \n",client);
                    break;
                }

                if(recvAll(new_fd, data, aux.dataSize) == -1)
                {
                    printf("App %d disconnected, waiting for a new one\n", client);
                    break;
                }
                //printf("data: %s\n", (char *)data);


                if(c_sock[0] == 0)
                {
                    pthread_rwlock_wrlock(&cbLock);

                    if(clipboard.data[aux.region] != NULL)
                        free(clipboard.data[aux.region]);


                    clipboard.dataSize[aux.region] = aux.dataSize;
                    clipboard.data[aux.region] = data;

                    pthread_cond_broadcast(&conditions[aux.region]);

                    pthread_rwlock_unlock(&cbLock);

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
                else  //Propagate to top node
                {
                    pthread_mutex_lock(&lock_c);
                    aux_sockc = c_sock[0];
                    pthread_mutex_unlock(&lock_c);
                    ppgtToParent(aux_sockc, aux.region, data, aux.dataSize);
                }
            }

            printf("App %d pasted %s to region %d\n", new_fd, (char *)clipboard.data[aux.region], aux.region);
        }
        else if(aux.oper == 2)   //Wait
        {

            printf("App %d is waiting for the region %d\n", new_fd, aux.region);
            pthread_mutex_lock(&lock_c);
            pthread_cond_wait( &conditions[aux.region], &lock_c);
            pthread_mutex_unlock(&lock_c);
            if((recv(new_fd, msg, sizeof(Mensagem), 0)) <= 0){
                printf("App %d died while waiting\n",client );
                break;
            }
            if(clipboard.dataSize[aux.region] == 0)
            {
                //In case the region is empty

                if((send(new_fd, &error, sizeof(int), 0)) == -1)
                {
                    perror("send");
                    printf("App %d disconnected\n", client);
                    break;
                }
                printf("App %d tried to copy from region %d, but it's empty\n", client, aux.region);
            }
            else if(aux.dataSize < clipboard.dataSize[aux.region]) //cliente não tem espaço
            {
                if((send(new_fd, &error, sizeof(int), 0)) == -1)
                {
                    perror("send");
                    printf("App %d disconnected\n", client);
                    break;
                }
                printf("App %d tried to copy from region %d, but he doesn't have enough space\n", client, aux.region);
            }
            else  //all good
            {
                if((send(new_fd, &success, sizeof(int), 0)) == -1)
                {
                    perror("send");
                    printf("App %d disconnected\n", client);
                    break;
                }

                aux.dataSize = clipboard.dataSize[aux.region];

                memcpy(msg, &aux, sizeof(Mensagem));

                if((send(new_fd, msg, sizeof(aux), 0)) == -1)
                {
                    perror("send");
                    printf("App disconnected\n");
                    break;
                }
                if((send(new_fd, clipboard.data[aux.region], aux.dataSize, 0)) == -1)
                {
                    perror("send");
                    printf("App disconnected\n");
                    break;
                }
                printf("App %d waited and read %s from region %d\n", client, (char *)clipboard.data[aux.region], aux.region);
            }

        }
        else
        {
            printf("Invalid operation asked by App %d\n",client);
            break;
        }
    }
    free(msg);
    close(new_fd);
    pthread_exit(NULL);
}
/*****************************************************************************/
/* clipboard_connection. Handler for the clipboard connection listener thread*/
/*****************************************************************************/
void *clipboard_connection(void *sock)
{
    //pthread_exit(NULL);
    int my_fd = *(int *)sock;
    int new_fd, i;
    void *data;
    size_t count;
    socklen_t addr_size;
    struct sockaddr_in their_addr;
    pthread_t tid;
    pthread_attr_t attr;
    fd_set readfds;
    int max_sd;
    //int activity;
    int kill_me;
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 15000;

    //Set thread attribute as detached
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    /*********************************/


    if((listen(my_fd, MAX_CALLS)) == -1)
    {
        perror("listen");
        exit(1);
    }
    while(1)
    {
        //Handle CTRL C
        pthread_mutex_lock(&lock_sig);
        kill_me = kill_flag;
        pthread_mutex_unlock(&lock_sig);
        if(kill_me == 1) break;

        //Timeout on accept//
        FD_ZERO(&readfds);
        FD_SET(my_fd, &readfds);
        max_sd = my_fd;
        select(max_sd + 1, &readfds, NULL, NULL, &tv);
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
                for(i = 0; i < 10; i++)
                {
                    pthread_rwlock_rdlock(&cbLock);
                    count = clipboard.dataSize[i];
                    data = malloc(count);
                    memcpy(data, clipboard.data[i], count);
                    pthread_rwlock_unlock(&cbLock);
                    backup_copy(new_fd, i, data, count);
                    free(data);
                }
                add_me(new_fd);
                pthread_create(&tid, &attr, clipboard_handler, &new_fd);

            }
        }
    }


    printf("all apps are off\n");
    pthread_exit(NULL);
}
/*****************************************************************************/
/* app_connection. Handler for the app connection listener thread            */
/*****************************************************************************/
void *app_connect(void  *sock)
{
    int my_fd = *(int *)sock;
    int new_fd;



    //Thread variables
    pthread_t tid;
    pthread_attr_t attr;

    //Variables for select
    fd_set readfds;
    int max_sd;
    //int activity;
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 15000;
    //Variable for finalization
    int kill_me;

    //Set thread attribute as detached
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    /*********************************/


    if((listen(my_fd, MAX_CALLS)) == -1)
    {
        perror("listen");
        exit(1);
    }
    printf("Listenning\n");

    while(1)
    {
        //Handle CTRL C
        pthread_mutex_lock(&lock_sig);
        kill_me = kill_flag;
        pthread_mutex_unlock(&lock_sig);
        if(kill_me == 1) break;

        //Adds timeout on accept
        FD_ZERO(&readfds);
        FD_SET(my_fd, &readfds);
        max_sd = my_fd;
        select(max_sd + 1, &readfds, NULL, NULL, &tv);
        tv.tv_sec = 0;
        tv.tv_usec = 15000;

        if(FD_ISSET(my_fd, &readfds))
        {
            //addr_size = sizeof(their_addr);
            new_fd = accept(my_fd, NULL, NULL);
            if((new_fd) == -1)
            {
                perror("accept");
                pthread_exit(NULL);
            }
            else
            {

                pthread_create(&tid, &attr, app_connection_handler, &new_fd);

            }
        }

    }


    printf("all clipboards are off\n");
    pthread_exit(NULL);
}
/*****************************************************************************/
/* clipboard_handler. Handler for the clipboard communication thread         */
/*****************************************************************************/
void *clipboard_handler(void *sock)
{
    void *data, *data2;
    Mensagem aux;
    void *msg = malloc(sizeof(Mensagem));
    int new_fd = *(int *)sock;

    int i;
    int aux_sockc;
    int client = new_fd;

    int kill_me = 0;

    printf("Top: Clipboard at sockfd %d is online \n", new_fd );

    //Talk to me
    while(1)
    {
        pthread_mutex_lock(&lock_sig);
        kill_me = kill_flag;
        pthread_mutex_unlock(&lock_sig);
        if(kill_me == 1)
        {
            remove_me(new_fd);
            close(new_fd);
            if(data2 != NULL)free(data2);
            if(msg != NULL) free(msg);
            pthread_exit(NULL);
        }
        if((recv(new_fd, msg, sizeof(Mensagem), 0)) <= 0)
        {
            printf("Clipboard: My client disconnected, waiting for a new one\n");
            break;
        }


        memcpy(&aux, msg, sizeof(Mensagem));
        if(aux.region > 9)
        {
            printf("Clipboard: OUTOFBOUNDS! (exiting)\n");
            break;
        }

        if (aux.oper == 1 ) //Copy
        {
            data = malloc(aux.dataSize);   //ponteiro que vai ser atribuído à posição do clipboard
            data2 = malloc(aux.dataSize); //ponteiro para fazer memcpy
            if(data == NULL || data2 == NULL)
            {
                printf("Clipboard: ERROR ALOCATING MEMORY\n");
                exit(1);
            }
            else
            {
                if(aux.dataSize > 0)
                {
                    if(recvAll(new_fd, data2, aux.dataSize) == -1)
                    {
                        printf("Clipboard: My client disconnected, waiting for a new one\n");
                        break;
                    }

                    memcpy(data, data2, aux.dataSize);

                    pthread_rwlock_wrlock(&cbLock);

                    if(clipboard.data[aux.region] != NULL)
                        free(clipboard.data[aux.region]);


                    clipboard.dataSize[aux.region] = aux.dataSize;
                    clipboard.data[aux.region] = data;
                    pthread_cond_broadcast(&conditions[aux.region]);
                    pthread_rwlock_unlock(&cbLock);

                    for (i = 1; i < c_sock_size; i++)
                    {
                        //CB recebem sempre paste de cima, por isso podem propagar sempre
                        pthread_mutex_lock(&lock_c);
                        aux_sockc = c_sock[i];
                        pthread_mutex_unlock(&lock_c);
                        backup_copy(aux_sockc, aux.region, data2, aux.dataSize);

                    }
                }
            }


            printf("Another clipboard %d pasted %s to region %d\n", client, (char *)clipboard.data[aux.region], aux.region);

            free(data2);

        }
        else if(aux.oper == 2) //propagation to parent
        {
            data2 = malloc(aux.dataSize);
            data = malloc(aux.dataSize);

            if(data == NULL)
            {
                printf("Clipboard: ERROR ALOCATING MEMORY\n");
                exit(1);
            }
            else
            {
                if(recvAll(new_fd, data2, aux.dataSize) == -1)
                {
                    printf("Clipboard: My clipboard client disconnected, waiting for a new one\n");
                    break;
                }
                memcpy(data, data2, aux.dataSize);

                if(c_sock[0] != 0)
                {
                    pthread_mutex_lock(&lock_c);
                    aux_sockc = c_sock[0];
                    pthread_mutex_unlock(&lock_c);
                    ppgtToParent(aux_sockc, aux.region, data, aux.dataSize);
                }
                else
                {
                    pthread_rwlock_wrlock(&cbLock);

                    if(clipboard.data[aux.region] != NULL)
                        free(clipboard.data[aux.region]);

                    clipboard.dataSize[aux.region] = aux.dataSize;
                    clipboard.data[aux.region] = data;
                    pthread_cond_broadcast(&conditions[aux.region]);
                    pthread_rwlock_unlock(&cbLock);
                    for (i = 1; i < c_sock_size; i++)
                    {
                        //CB recebem sempre paste de cima, por isso podem propagar sempre
                        pthread_mutex_lock(&lock_c);
                        aux_sockc = c_sock[i];
                        pthread_mutex_unlock(&lock_c);
                        backup_copy(aux_sockc, aux.region, data2, aux.dataSize);
                    }
                }
            }
            free(data2);
        }
    }

    remove_me(new_fd);
    close(new_fd);
    free(msg);
    pthread_exit(NULL);
}
/*****************************************************************************/
/* ppgtToPArent. Sends a propagate request to another clipboard              */
/*****************************************************************************/
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
    pthread_mutex_lock(&lockMsg);
    retorno = send(clipboard_id, msg, sizeof(Mensagem), 0); //informa o clipboard do tamanho da mensagem
    if(retorno < 0)
    {
        pthread_mutex_unlock(&lockMsg);
        printf("Problem sending info (ppgtToParent)\n");
        remove_me(clipboard_id);
        free(msg);
        free(data);
        return -1;
    }
    memcpy(data, buf, count);
    retorno = send(clipboard_id, data, aux.dataSize, 0); //envia a mensagem
    if(retorno < 0)
    {
        pthread_mutex_unlock(&lockMsg);
        printf("Problem sending data\n");
        remove_me(clipboard_id);
        close(clipboard_id);
        free(msg);
        free(data);
        return -1;
    }
    pthread_mutex_unlock(&lockMsg);
    free(data);
    free(msg);
    return retorno;
}
/*****************************************************************************/
/* remove_me. Remove a clipboard sockfd from memory when it disconnects      */
/*****************************************************************************/
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
/*****************************************************************************/
/* add_me. Save a clipboard sockfd in memory when it connects                */
/*****************************************************************************/
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
/*****************************************************************************/
/* kill_signal. Handler for ctrl C                                           */
/*****************************************************************************/
void kill_signal(int signo)
{

    pthread_mutex_lock(&lock_sig);
    kill_flag = 1;
    pthread_mutex_unlock(&lock_sig);

    return;

}
/*****************************************************************************/
/* recvAll. Forces the reception of all bytes                                */
/*****************************************************************************/
int recvAll(int fd, void *data, size_t count)
{
    int totalBytes = 0, retorno;
    void *recvPtr;
    do
    {
        recvPtr = data + totalBytes;
        retorno = recv(fd, recvPtr, count, MSG_NOSIGNAL);
        if(retorno <= 0)
        {
            return -1;
        }

        totalBytes += retorno;
    }
    while(totalBytes < count);
    return 0;
}
