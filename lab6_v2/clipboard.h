#define MAXDATASIZE 1000
#define MYPORT "1337"
#define MAX_CALLS 10
#define SOCK_PATH "./lab6_v2"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <sys/un.h>
#include <signal.h>
#include <string.h>
typedef struct Mensagem{
	int region;
	char dados[10];
	int oper;
}Mensagem;



int clipboard_connect(char * clipboard_dir);
int clipboard_copy(int clipboard_id, int region, void *buf, size_t count);
int clipboard_paste(int clipboard_id, int region, void *buf, size_t count);

