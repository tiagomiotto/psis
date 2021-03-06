#include "clipboard.h"


int clipboard_connect(char *clipboard_dir)
{
    int sockfd, numbytes;
    char buf[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    int aux2;
    struct sockaddr_in ip;

    char s[INET6_ADDRSTRLEN];

    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, clipboard_dir, sizeof(addr.sun_path) - 1);

    if((connect(sockfd, (struct sockaddr *) &addr, sizeof(struct sockaddr_un))) == -1)
    {
        close(sockfd);
        perror("client: connect");
        return -1;
    }
    printf("Connected to a clipboard at %s\n", clipboard_dir);

    return sockfd;

}

int clipboard_paste(int clipboard_id, int region, void *buf, size_t count)
{
    int totalBytes = 0;
    int retorno;
    void *recvPtr;
    if(region > 10)
    {
        printf("Paste region out of bounds\n");
        return -1;
    }
    int okFlag;
    Mensagem aux;
    void *msg = malloc(sizeof(Mensagem));
    aux.region = region;
    aux.oper = 0;
    aux.dataSize = count;

    memcpy(msg, &aux, sizeof(Mensagem));
    send(clipboard_id, msg, sizeof(Mensagem), MSG_NOSIGNAL);

    recv(clipboard_id, &okFlag, sizeof(int), 0); //Lê se o cliente tem espaço para receber a informação

    if(okFlag == 1)
    {
        if((recv(clipboard_id, msg, sizeof(Mensagem), 0)) < 0) return -1;
        memcpy(&aux, msg, sizeof(Mensagem));
        do{ //read enquanto existe mensagem a ser enviada
            recvPtr = buf + totalBytes;
            retorno = recv(clipboard_id, recvPtr, aux.dataSize, 0);
            if(retorno <= 0)
            {
                printf("My client disconnected, waiting for a new one\n");
                return -1;
            }
            totalBytes += retorno;
        } while(totalBytes < aux.dataSize);  
    }
    else return -1;

    free(msg);
    return aux.dataSize;

}
int clipboard_copy(int clipboard_id, int region, void *buf, size_t count)
{
    Mensagem aux;
    void *msg = malloc(sizeof(Mensagem));
    void *data = malloc(count);
    int retorno, okFlag;
    if((strcmp(buf, "")) == 0 || (strcmp(buf, "\n")) == 0)
    {
        printf("You can't paste an empty line\n");
        free(data);
        free(msg);
        return -1;
    }
    //printf("message: %s\n", (char *)buf);
    //printf("count: %zx\n", count);
    aux.region = region;
    aux.oper = 1;
    aux.dataSize = count;
    memcpy(msg, &aux, sizeof(Mensagem));
    retorno = send(clipboard_id, msg, sizeof(Mensagem), MSG_NOSIGNAL); //informa o clipboard do tamanho da mensagem
    if(retorno <= 0)
    {
        printf("Problem sending info\n");
        free(msg);
        free(data);
        return -1;
    }
    recv(clipboard_id, &okFlag, sizeof(int), 0); //recebe o OK do clipboard depois de ter alocado memória para receber a mensagem
    if (!okFlag)
    {
        printf("Problem with clipboard\n");
        free(msg);
        free(data);
        return -1;
    }
    memcpy(data, buf, count);
    retorno = send(clipboard_id, data, aux.dataSize, MSG_NOSIGNAL); //envia a mensagem
    if(retorno <= 0)
    {
        printf("Problem sending data\n");
        free(msg);
        free(data);
        return -1;
    }
    free(data);
    free(msg);
    return retorno;
}

int clipboard_wait(int clipboard_id, int region, void *buf, size_t count)
{
    if(region > 10)
    {
        printf("Wait region out of bounds\n");
        return -1;
    }
    int okFlag;
    Mensagem aux;
    void *msg = malloc(sizeof(Mensagem));
    aux.region = region;
    aux.oper = 2;
    aux.dataSize = count;

    memcpy(msg, &aux, sizeof(Mensagem));
    send(clipboard_id, msg, sizeof(Mensagem), MSG_NOSIGNAL);

    recv(clipboard_id, &okFlag, sizeof(int), 0); //Lê se o cliente tem espaço para receber a informação

    if(okFlag)
    {
        if((recv(clipboard_id, msg, sizeof(Mensagem), 0)) < 0) return -1;
        memcpy(&aux, msg, sizeof(Mensagem));
        if((recv(clipboard_id, buf, aux.dataSize, 0)) < 0) return -1;
    }

    free(msg);
    return aux.dataSize;

}

void clipboard_close(int clipboard_id){
	close(clipboard_id);
}
