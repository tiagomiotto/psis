//http://tldp.org/LDP/lpg/node11.html
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <time.h>

int is_prime(int num)
{
	int i;
     if (num <= 1) return 0;
     if (num % 2 == 0 && num > 2) return 0;
     for(i = 3; i < num / 2; i+= 2)
     {
         if (num % i == 0)
             return 0;
     }
     return 1;
}
int get_sequential_random(int last_num){
	int rng;
	srand(time(NULL));
	if((rng=rand()%99999)>=last_num) return rng;
	return get_sequential_random(last_num);
}

int main(int argc, char const *argv[])
{

	int j,i,np,interv,pid,status,aux,aux3=0;

	if (sscanf (argv[1], "%i", &np) != 1) {
    fprintf(stderr, "error - not an integer");
	}
	if (sscanf (argv[2], "%i", &interv) != 1) {
    fprintf(stderr, "error - not an integer");
	}
	
	int *fd = malloc(np*sizeof(int));
	pid_t *pids = malloc(np*sizeof(pid_t));
	srand(time(NULL));
	int rng=rand()%99999;

	for (i = 0; i < np; ++i)
	{
		pipe(&fd[2*i]);
	}
	

	for (i = 0; i < np; ++i)
	{
	if((pids[i] = fork())<0){
		perror("fork");
		abort();
	}
	else if (pids[i] ==0){ //Child code
		//int *aux2=malloc(interv*sizeof(int));
		int count=0,count2=0;
		int aux2;

		
		for(j=0;j<np;j++){ //Closes all other pipes
			if(j!=i){
			close(fd[2*j]);
			close(fd[2*j+1]);
			}
		}

		close(2*i+1); //Close write side of my pipe

		//count2=read(fd[2*i],aux2,sizeof(int)*interv);
		if((count2=read(fd[2*i],&aux2,sizeof(int))) <=0) {
			close(fd[2*i]);
			//free(aux2);
			exit(0);
		}
		else{
			if((is_prime(aux2))==1) count++;}
		/*
		for(j=0;j<count2/sizeof(int);j++){
			printf("%d \n", aux2[j]);
		if((is_prime(aux2[j]))==1) count++;
		}*/
	
		printf("%d primes\n",count);
		//free(aux2);
		exit(0);
	}else{ //Parent code
		
		for(j=0;j<np;j++){ //Closes all read sides
			close(fd[2*j]);
		}
		
		for(j=0;j<interv;j++){	
		write(fd[2*aux3+1],&rng,sizeof(int)); //writes to the pipe
		if(j>=np-1) aux3=0;
		aux3++;
		}
		
		for(j=0;j<np;j++){
			close(fd[2*j+1]);
		}
		
		aux=np;
		while(aux>0){
		pid = wait(&status);
		aux--;}
		free(pids);
		free(fd);
	}
	}
	
	return 0;
}
