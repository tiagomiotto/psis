#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>


int main(){
	char v1[100];
	char *v2;
	int i;
	
	printf("Write a word \n");
	fgets(v1, 100, stdin);

	v2 = malloc(sizeof(char)*(strlen(v1)+1)); //Strlen returns the size of the string
												//not including the \o 
	for (i=0; v1[i]!=0; i++){
		v2[i] = toupper(v1[i]);
	}
	v2[i]='\0'; //Copying the /0
	printf("Converted string: %s", v2);

 free(v2);
}
