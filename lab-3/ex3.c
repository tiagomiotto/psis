#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>


#define SYMBOL value
#define RANGE 10

int counter =0;

void alarm_handler(int signum){
	counter=0;
	printf("Alarm\n");
}

void delay(int number_of_seconds)
{
    // Converting time into milli_seconds
    int milli_seconds = 1000 * number_of_seconds;
 
    // Stroing start time
    clock_t start_time = clock();
 
    // looping till required time is not acheived
    while (clock() < start_time + milli_seconds)
        ;
}

int main(){
	int sec;
	struct sigaction new_action, old_action;

	new_action.sa_handler= alarm_handler;
	sigemptyset (&new_action.sa_mask);
	new_action.sa_flags = 0;

	sigaction(SIGALRM,NULL,&old_action);
	if(old_action.sa_handler != SIG_IGN){
		sigaction(SIGALRM,&new_action,NULL);
	}

	while(1){
		
		if(counter==0){
		srand(time(NULL));
		sec=rand()%RANGE;
		alarm(sec);
		}
		printf("%d\n", counter);
		counter++;
		delay(1000);
	}
	return(0);

}