#include "clipboard.h"
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>

int main(){
		
		int fd = clipboard_connect(SOCK_PATH);
		if(fd<0) return 0;
		char *msg;
		if(fd== -1){
			exit(-1);
		}
		char dados[10];
		char dados2[10];
		char region[4];
		int region_int;
		char quit[4];

		
		while((atoi(quit))==0){

		printf("Write your phrase to be saved to the clipboard\n");
		fgets(dados, 10, stdin);
		//printf("\n");
		strtok(dados,"\n");
		/*Already implemented in library
		if((strcmp(dados,""))==0 || (strcmp(dados,"\n"))==0){
			do{
				printf("You can't paste an empty string, try again\n");
				fgets(dados, 10, stdin);
				printf("\n");
				strtok(dados,"\n");
				
			}while((strcmp(dados,""))==0 || (strcmp(dados,"\n"))==0);
		}*/
		
		printf("Write the region in which you want it to be saved 0-9\n");
		fgets(region, 4,stdin);
		
		region_int= atoi(region);
		
		if((clipboard_paste(fd,region_int,dados,10))<0) {
			printf("paste failed \n");
		}
		else printf("Pasted: %s to region %d\n", dados,region_int);
		

		printf("Write the region which you want to read 0-9\n");
		fgets(region, 4,stdin);
		
		region_int= atoi(region);
		
		if((clipboard_copy(fd,region_int,dados2,10))<0){
			printf("copy failed \n");
		}
		else printf("Copied %s from region %d\n", dados2, region_int);
		
		printf("Continue? Type 0\n");
		fgets(quit, 4,stdin);
		printf("\n");
		
	}
		close(fd);
		exit(0);
	}
