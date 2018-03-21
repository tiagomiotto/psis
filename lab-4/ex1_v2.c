//http://tldp.org/LDP/lpg/node11.html
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <time.h>

int is_prime(int num)
{
     if (num <= 1) return 0;
     if (num % 2 == 0 && num > 2) return 0;
     for(int i = 3; i < num / 2; i+= 2)
     {
         if (num % i == 0)
             return 0;
     }
     return 1;
}
int get_sequential_random(int last_num){
	int rng;
	
	if((rng=rand()%99999)>=last_num) return rng;
	return get_sequential_random(last_num);
}

int main(int argc, char const *argv[])
{
	int j,i,fd[2];
	pid_t *pids = malloc(argv[1]*sizeof(pid_t));

	srand(time(NULL));
	int rng=rand()%99999;

	for (i = 0; i < argv[1]; ++i)
	{
	if((pids[i] = fork())<0){
		perror("fork");
		abort();
	}
	else if (pids[i] ==0){ //Child code
		int aux,count;
		close(fd[1]);
		while(read(fd[0],aux,sizeof(int))){
		if((is_prime(aux))==1) count++;
		}
		exit(0);
	}else{ //Parent code
		
		for(j=0;j<argv[2];j++){
		rng=get_sequential_random(rng);
		close(fd[0]); //Close output of the pipe
		write(fd[1],rng,sizeof(int)); //writes to the pipe
		}
		close(fd[0]);
		close(fd[1]);
	}
	}
	return 0;
}
