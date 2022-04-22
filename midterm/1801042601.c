#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/wait.h>
#include <time.h>

#define CLIENT_FIFO_TEMPLATE "/tmp/client.%d"
#define CLIENT_FIFO_NAME_LEN (sizeof(CLIENT_FIFO_TEMPLATE) + 20)

int checkArgc(int argc, char *argv[]);
int openFile(char *dataFile,int type);
void closeFile(int fd);
void read_firstLine(char *dataFile, int *coma);
int read_all(int fd,char **mat, int coma);
static void removeFifo(void);
int add_timestamp_client_pid(char dest[150]);
void printFirstLine(int coma);
void printSecondLine(int result, double time);
int send_request(char *pathToServerFifo, char **mat, int coma);
void handler(int sig_number);
void sig_check();

sig_atomic_t flag = 0;

char *pathToServerFifo;
char *pathToDataFile;
char **mat;
int size=0;
static char clientFifo[CLIENT_FIFO_NAME_LEN];

int main(int argc, char *argv[]){
    struct timeval start, end;

    gettimeofday(&start, NULL);
    struct sigaction act;
    memset(&act, '\0', sizeof(act));
    act.sa_handler = &handler;
    sigaction(SIGINT, &act, NULL);

    int index = checkArgc(argc,argv);
    pathToServerFifo=argv[index];
    pathToDataFile = argv[6-index];
    sig_check();
    read_firstLine(pathToDataFile,&size);
    size++;
    mat = (char **)malloc((size) * sizeof(char *));
    for (int i = 0; i < size; i++)
        mat[i]=(char*)malloc(1*sizeof(char));
    sig_check();
    printFirstLine(size);
    sig_check();
    int fd = openFile(pathToDataFile, O_RDONLY);
    if (read_all(fd,mat,size) != -1){
        sig_check();
        int result=send_request(pathToServerFifo,mat,size);
        sig_check();
        gettimeofday(&end, NULL);
        long seconds = (end.tv_sec - start.tv_sec);
        long micros = ((seconds * 1000000) + end.tv_usec) - (start.tv_usec);
        printSecondLine(result,(double)micros);
        sig_check();
    }else{
        perror("Not a square matrix");
    }
    sig_check();
    for (int i = 0; i < size; i++)
        free(mat[i]);
    free(mat);
    size=0;
    sig_check();
    return 0;
}
int checkArgc(int argc, char *argv[]){
    if (argc != 5){
        perror("Usage : ./client [-s] pathToServerFifo [-o] pathToDataFile\n");
    }else
        return (argv[1][1]=='s') ? 2 : 4;
        
    exit(0);
}
void handler(int sig_number){
    flag = 1;
}
void sig_check(){
    if (flag == 1){
        char dest[150];
        memset(dest, '\0', 150);
        int index = add_timestamp_client_pid(dest);
        index += sprintf(dest + index, "SIGINT RECEIVED, CLIENT TERMINATING...\n");
        write(STDOUT_FILENO, dest, index);
        int fd = open(pathToDataFile, O_RDONLY);
        close(fd);
        int fd2 = open(clientFifo,O_TRUNC);
        close(fd2);
        unlink(clientFifo);
        if(mat!=NULL){
            for (int i = 0; i < size; i++)
                free(mat[i]);
            free(mat);
        }
        exit(1);
    }
}
int read_all(int fd,char **mat ,int coma){
    char buff[1];
    int line_size=0,mat_index=0,num_coma=0;
    while (read(fd, buff,1) == 1){
        if (buff[0] == '\n'){
            mat[mat_index][line_size] ='\0';
            mat_index++;
            if (mat_index==coma)
                break;
            if (coma-1 != num_coma)
                return -1;
            line_size=0;
            num_coma=0;
        }else{
            mat[mat_index][line_size]=buff[0];
            if (buff[0]==',')
                num_coma++;
            
            line_size++;
            mat[mat_index] = (char *)realloc(mat[mat_index], (line_size+1 )* sizeof(char));
        }
    }
    closeFile(fd);
    return 0;
}
void read_firstLine(char *dataFile,int *coma){
    int fd = openFile(dataFile,O_RDONLY);
    char buff[1];
    while (read(fd, buff, 1) == 1){
        if (buff[0]==','){
            (*coma)++;
        }else if (buff[0] == '\n'){
            break;
        }
    }
    closeFile(fd);
    if ((*coma)<2){
        perror("Size is lower than 2\nTERMINATING...");
        exit(1);
    }
}
int send_request(char *pathToServerFifo,char** mat,int coma){
    umask(0);
    snprintf(clientFifo, CLIENT_FIFO_NAME_LEN, CLIENT_FIFO_TEMPLATE, (int)getpid());

    if (mkfifo(clientFifo, S_IRUSR | S_IWUSR | S_IWGRP) == -1 && errno != EEXIST)
        perror("mkfifo\n");

    if (atexit(removeFifo) != 0)
        perror("atexit\n");
   
    int serverFd = openFile(pathToServerFifo, O_WRONLY);
    char cpid[1000];
    memset(cpid, '\0', 1000);
    int index=0;
    index += sprintf(cpid, "%d;%d;",(int)getpid(),coma);
    for (int i = 0; i < coma; i++){
        index += sprintf(cpid+index, "%s;",mat[i]);
    }
    index = sprintf(cpid + index, "\n");
    if (write(serverFd, cpid, strlen(cpid)) != strlen(cpid))
        perror("Can't write to server\n");
    closeFile(serverFd);
    /* Open our FIFO, read and display response */
    
    int clientFd = openFile(clientFifo, O_RDONLY);
    int result;
    if (read(clientFd, &result, sizeof(int)) != sizeof(int))
        perror("Can't read response from server\n");
    closeFile(clientFd);
    return result;
}
static void removeFifo(void){ /* Invoked on exit to delete client FIFO */
    unlink(clientFifo);
}
void closeFile(int fd){
    int err2 = close(fd);
    if (err2 == -1)
        perror("Failed to Close File");
}
int openFile(char *dataFile,int type){
    int fd = open(dataFile, type);
    if (fd == -1){
        printf("Failed to Open File: %s",dataFile);
        exit(1);
    }
    return fd;
}
void printFirstLine(int coma){
    char dest[150];
    memset(dest, '\0', 150);
    int index= add_timestamp_client_pid(dest);
    index += sprintf(dest + index, "(%s) is submitting a %dx%d matrix\n", pathToDataFile,coma,coma);
    write(STDOUT_FILENO,dest,index);
}
int add_timestamp_client_pid(char dest[150]){
    int index=0;
    time_t t; 
    time(&t);
    index = sprintf(dest, "%s Client PID#%d :",ctime(&t),getpid());
    return index;
}
void printSecondLine(int result, double time){
    /*Client PID#667: the matrix is invertible, total time 2.25 seconds, goodbye. */
    char dest[150];
    memset(dest, '\0', 150);
    int index = add_timestamp_client_pid(dest);

    if (result==1){
        index += sprintf(dest + index, " the matrix is invertible, total time %.2f seconds, goodbye.\n",time/CLOCKS_PER_SEC );
    }else
        index += sprintf(dest + index, " the matrix is not invertible, total time %.2f seconds, goodbye.\n", time / CLOCKS_PER_SEC);
    write(STDOUT_FILENO, dest, index);
}
