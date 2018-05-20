#include "clipboard.h"
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>

int main()
{


    char path[20];
    char dados[10];
    char dados2[10];
    char dados3[20];
    char region[4];
    int region_int;
    char quit[4];
    printf("Write the path of the\n");
    fgets(path, 20, stdin);
    strtok(path, "\n");

    int fd = clipboard_connect(path);
    char *msg;
    if(fd == -1)
    {
        exit(-1);
    }

    while((atoi(quit)) == 0)
    {

        printf("Write your phrase to be saved to the clipboard\n");
        fgets(dados, 10, stdin);
        strtok(dados, "\n");

        printf("Write the region in which you want it to be saved 0-9\n");
        fgets(region, 4, stdin);

        region_int = atoi(region);

        if((clipboard_copy(fd, region_int, dados, 10)) < 0)
        {
            printf("paste failed \n");
        }
        else printf("Pasted: %s to region %d\n", dados, region_int);


        printf("Write the region which you want to read 0-9\n");
        fgets(region, 4, stdin);

        region_int = atoi(region);

        if((clipboard_paste(fd, region_int, dados2, 10)) < 0)
        {
            printf("copy failed \n");
        }
        else printf("Copied %s from region %d\n", dados2, region_int);

        printf("Write the region which you want to wait for 0-9\n");
        fgets(region, 4, stdin);

        region_int = atoi(region);

        if((clipboard_wait(fd, region_int, dados3, 30)) < 0)
        {
            printf("copy failed \n");
        }
        else printf("Copied %s from region %d\n", dados3, region_int);

        printf("Continue? Type 0\n");
        fgets(quit, 4, stdin);
        printf("\n");


    }
    clipboard_close(fd);
    exit(0);
}
