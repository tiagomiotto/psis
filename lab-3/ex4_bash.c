#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>
#include<string.h>

#define BILLION 1E9


int main(int argc, char *argv[]){

	int n;
	struct timespec beginning,end;
	double running_time;
		
	for (n = 0; n < argc; ++n)
	{	
		if(strcmp(argv[n],"") != 0){
		clock_gettime(CLOCK_REALTIME,&beginning);
		system(argv[n]);
		clock_gettime(CLOCK_REALTIME,&end);
		running_time = (end.tv_sec - beginning.tv_sec )+ (end.tv_nsec - beginning.tv_nsec );
		printf( "The bash command ran in %f seconds\n", running_time/BILLION);}
	}

	return(0); 
	
}