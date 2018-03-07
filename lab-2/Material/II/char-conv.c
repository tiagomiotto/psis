#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#define SIZE 10

int main(){
	char v1[SIZE];
	char *v2;
	int i;
	v2= (char *) malloc(SIZE*sizeof(char));
	
	printf("Write a word \n");
	fgets(v1, SIZE, stdin);
	v2= (char *) malloc(strlen(v1)*sizeof(char));
	for (i=0; v1[i]!=0; i++){
		v2[i] = toupper(v1[i]);
	}

	printf("Converted string: %s", v2);
free(v2);

}
