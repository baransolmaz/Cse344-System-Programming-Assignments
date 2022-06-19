#ifndef HELPER_H
#define HELPER_H

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

void closeFile(int fd){
    if (fd != -1)    {
        int err2 = close(fd);
        if (err2 == -1)
            perror("Failed to Close File");
    }
}
int openFile(char *fileName, int type){
    int fd = open(fileName, type);
    if (fd == -1){
        printf("Failed to Open File: %s", fileName);
        exit(1);
    }
    return fd;
}
void perror_call(char *er){
    perror(er);
    exit(EXIT_FAILURE);
}
char *getTimeStamp(){
    time_t t;
    time(&t);
    char *tm = ctime(&t);
    tm[strlen(tm) - 1] = ' ';
    return tm;
}
int getProcID(char* path){
    int fd=openFile(path,O_RDONLY);
    char arr[60];
    memset(arr, '\0', 60);
    char* line=arr;
    if(read(fd, line, 60)<=0)
        perror_call("No process ID");
    char *id = strtok_r(line, " ", &line);
    closeFile(fd);
    return atoi(id);
}
#endif // HELPER_H
