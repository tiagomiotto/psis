#include "clipboard.h"
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(){
	int action, region, copyData, pasteData;
	char dados[2], aux[50], path[20];

	dados[1] = '\0';
	printf("Write the path of the\n");
    fgets(path, 20, stdin);
    strtok(path, "\n");

    int fd = clipboard_connect(path);
	// Connects to the cliboard
	if(fd == -1){
		exit(-1);
	}
	while(1) {
		//dados[0] = rand()%(122-65)+65;
		//sleep(1);
		// Sends the data to the cliboard server
		copyData = clipboard_paste(fd, 0, dados, 2);
		if(copyData < 1) {
			printf("Error on copy\n");
		}
		else {
			printf("Received %d - data: %s\n", copyData, dados);
		}
	}
	
	close(fd);
	exit(0);
}