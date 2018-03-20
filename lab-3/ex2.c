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
/*Create first 10 children*/
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
		
		exit(sec);
	}
}

int status;
pid_t pid,pid2;
/*Wait for them to exit and spawn a new child upon successfull exit*/
while(n>0){
	pid = wait(&status);
	if (WIFEXITED(status)){
    	int returned = WEXITSTATUS(status);
		printf("Process %d sleept for %d seconds and exited\n", pid, returned);
	}
	
	--n;

	if(n<10){

	if((pid2 = fork())<0){
		perror("fork");
		abort();
	}
	else if (pid2 ==0){
		srand(getpid());
		sec=rand()%11;
		sleep(sec);
		
		exit(sec);
	}
	n++;
	printf("Processes running %d\n",n );	
	}
}
return(0);
}
