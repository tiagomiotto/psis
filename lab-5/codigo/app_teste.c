#include "clipboard.h"
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>

int main(){
		
		int fd = clipboard_connect("./");
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
		printf("\n");
		
		printf("Write the region in which you want it to be saved 0-9\n");
		fgets(region, 4,stdin);
		printf("\n");
		region_int= atoi(region);
		
		if((clipboard_paste(fd,region_int,dados,10))<0) {
			printf("paste failed \n");
		}
		else printf("Pasted: %s to region %d\n \n", dados,region_int);
		

		printf("Write the region which you want to read 0-9\n");
		fgets(region, 4,stdin);
		printf("\n");
		region_int= atoi(region);
		
		if((clipboard_copy(fd,region_int,dados2,10))<0){
			printf("copy failed \n");
		}
		else printf("Copied %s from region %d\n \n", dados2, region_int);
		
		printf("Continue? Type 0\n");
		fgets(quit, 4,stdin);
		printf("\n");
		
	}
		
		exit(0);
	}
