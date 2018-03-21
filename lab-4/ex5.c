#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <time.h>
#include <sys/stat.h> 

int main(int argc, char const *argv[]){
	
	int fd;
	char input[256];
	mkfifo(argv[1],0666);
	
	fd= fopen(argv[1], O_WRONLY);
	printf("Input string\n");
	fgets(input,sizeof(input,stdin);
	write(fd,input,strlen(input));
	
	close(fd);
	unlink(argv[1]);
	
	return(0);
}
