#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

#define SIZE 100
#define NUMBER 20
int main(){
	pid_t pid;
	char *const envParms[1] = {NULL};
	char *const name[1] = {"ex4_bash"}; 
    int i,status;

	if((pid = fork())<0){
		perror("fork");
		abort();
	}
	else if (pid ==0){

		FILE *fp;
		char **buffer=  malloc(NUMBER*sizeof(char*));
		for(i=0;i<NUMBER;i++){
			buffer[i]= (char*) malloc(SIZE*sizeof(char));
		}

		printf("Memory allocated\n");

		fp=fopen("commands.txt","r");
		if (fp == NULL) {
			printf("2\n");
			exit(EXIT_FAILURE);}
		i=0;
		printf("File openned\n");
		while(!feof(fp)){
			fgets(buffer[i],SIZE,fp);
			buffer[i]=strtok(buffer[i],"\n");
			i++;
		}
		printf("File read\n");

		execve("ex4_bash",buffer,envParms);
		printf("Program Called\n");
	}

	wait(&status);

		
		for(i=0;i<NUMBER;i++){
			free(buffer[i]);
		}
		free(buffer);
		exit(0);
	return(0);
}