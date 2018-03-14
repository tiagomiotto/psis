#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(){
pid_t pids[10];
int i;
int n=10;
int sec;
for (i = 0; i < n; ++i)
{
	if((pids[i] = fork())<0){
		perror("fork");
		abort();
	}
	else if (pids[i] ==0){
		srand(getpid());
		sec=rand()%11;
		sleep(sec);
		printf("I am proccess %d and i sleept for %d \n", getpid(), sec);
		exit(0);
	}
}

int status;
pid_t pid;
while((wait(&status))>0){

}
return(0);
}
