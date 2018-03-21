#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <time.h>
#include <sys/stat.h> 

int main(int argc, char const *argv[]){
	
	int fd;
	char buff[256];
	fd=fopen(argv[1],"r");
	
	read(fd,buf,256);
