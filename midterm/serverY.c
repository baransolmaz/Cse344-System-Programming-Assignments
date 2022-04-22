#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/signal.h>
#include <time.h>
#include <sys/wait.h>
#include "become_daemon.h"

#define CLIENT_FIFO_TEMPLATE "/tmp/client.%d"
#define CLIENT_FIFO_NAME_LEN (sizeof(CLIENT_FIFO_TEMPLATE) + 20)
sig_atomic_t flag = 0;

struct Worker{
    int Client_id;
    int size;
    int availability;
    int worker_id;
    int server_id;
    int pipe_write;
};
void handler(int sig_number,siginfo_t *info,void *v);
void sig_check();
int openFIFO();
void writeLogErr(char *buf);
void checkArgc(int argc, char *argv[]);
void readFIFO(int pool);
int determinantOfMatrix(int **mat, int n);
void getCofactor(int **mat, int **temp, int q, int n);
void createPool(int pool);
int worker(struct Worker *worker);
int invertible(int **mat, int size);
int openFile(char *dataFile, int type);
void closeFile(int fd);
void starterLog();
int getBusyWorkerNum();
void writeLogWorkerHandle(int wID, int id, int size);
void writeLogWorkerResponse(int id, int result);
void writeLogForward(int id, int size);
void writeLog(char *buf);
void lock_n_write(int fd, char *buf, int index);
void lockFile(int fd);
void unlockFile(int fd);
void wait_child(int childNumber);
void createServerZ();
void writeLogSIGINT();
void check_double_instant();
int add_timestamp_worker_pid(char dest[200], int id);
char *pathToServerFifo;
char *pathToLogFile;
int logFd=-1, dummyFd = -1,server_fifo = -1,double_fd=-1;
int poolSize=0,poolSize2=0,sleep_time=0;
int serverZ_id=-1,pipe_Z=-1;
int invert=0,notInvert=0,forwarded=0;
struct Worker *workers;
int main(int argc, char *argv[]){
    check_double_instant();
    becomeDaemon(0);
    struct sigaction act = {0};
    act.sa_flags = SA_SIGINFO;
    sigemptyset(&act.sa_mask);
    act.sa_sigaction = handler;
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGUSR1, &act, NULL);
    sigaction(SIGUSR2, &act, NULL);
    sigaction(SIGRTMIN, &act, NULL);
    
    checkArgc(argc, argv);
    sig_check();
    starterLog();
    sig_check();
    createServerZ();
    sig_check();
    createPool(poolSize);
    sig_check();
    free(workers);
    closeFile(double_fd);
    unlink(".double");
    exit(0);
}
void handler(int sig_number, siginfo_t *info, void *v){
    if (sig_number == SIGRTMIN){
        for (int i = 0; i < poolSize; i++)
            if (workers[i].worker_id == (int)(info->si_pid))
                workers[i].availability = 0;
    }else if (sig_number == SIGINT){
        flag = 1;
    }else if (sig_number == SIGUSR1){
        invert++;
    }else if (sig_number == SIGUSR2){
        notInvert++;
    }
}
void sig_check(){
    if (flag == 1){
        if (dummyFd != -1)
            closeFile(dummyFd);
        if (server_fifo != -1)
            closeFile(server_fifo);
        unlink(pathToServerFifo);

        if (workers != NULL){
            for (int i = 0; i < poolSize; i++)
                if (workers[i].worker_id != -1)
                    kill(workers[i].worker_id, SIGINT);
            wait_child(poolSize);
            free(workers);
            workers=NULL;
        }
        writeLogSIGINT();
        if (serverZ_id != -1){
            kill(serverZ_id, SIGINT);
            wait_child(1);
        }
        closeFile(double_fd);
        unlink(".double");
        exit(1);
    }
}
int openFIFO(){
    umask(0);
    if (mkfifo(pathToServerFifo, S_IRUSR | S_IWUSR | S_IWGRP) == -1 && errno != EEXIST)
        writeLogErr("Open FIFO : mkfifo");
    server_fifo = open(pathToServerFifo,O_RDONLY);
    if (server_fifo == -1)
        writeLogErr("Open FIFO : Open Server");
    sig_check();
    int dummyFd = open(pathToServerFifo, O_WRONLY);
    if (dummyFd == -1)
        writeLogErr("Open FIFO : Open Dummy");
    return dummyFd;
}
void checkArgc(int argc, char *argv[]){
    if (argc != 11){
        perror("Usage : ./serverY [-s] pathToServerFifo [-o] pathToLogFile [-p] poolSize [-r] poolSize2 [-t] 2\n");
    }else{
        for (int i = 1; i < argc; i += 2){
            switch (argv[i][1]){
                case 's':
                    pathToServerFifo = argv[i + 1];
                    break;
                case 'o':
                    pathToLogFile = argv[i + 1];
                    logFd=openFile(pathToLogFile,O_APPEND|O_WRONLY|O_CREAT|O_TRUNC);
                    break;
                case 'p':
                    poolSize = atoi(argv[i + 1]);
                    workers = (struct Worker *)malloc(poolSize * sizeof(struct Worker));
                    break;
                case 'r':
                    poolSize2 = atoi(argv[i + 1]);
                    break;
                case 't':
                    sleep_time = atoi(argv[i + 1]);
                    break;
                default:
                    break;
            }
        }
        return;
    }
    exit(1);
}
void readFIFO(int pool){
    while(1){
        char buff[2], line[1000], size[5], id[10];
        int indexLine = 0, semi_count = 0, size_index = 0,id_index=0;
        memset(id, '\0', 10);
        memset(size, '\0', 5);
        memset(buff, '\0', 2);
        memset(line, '\0', 1000);
        sig_check();
        while (read(server_fifo, buff, 1) == 1){
            sig_check();
            if (buff[0] == '\n' || buff[0] == '\0')
                break;
            if (buff[0] == ';'){
                semi_count++;
            }else if (semi_count==1){
                size[size_index++]=buff[0];
            }else if (semi_count == 0)
                id[id_index++] = buff[0];
            line[indexLine]=buff[0];
            indexLine++;
        };
        if ((int)strlen(line)==0)
            continue;
    
        line[indexLine] = '\n';
        indexLine++;

        int full=1;
        for (int i = 0; i < pool; i++){
            sig_check();
            if (workers[i].availability==0){ // Available
                full=0;
                workers[i].availability =1;
                writeLogWorkerHandle(workers[i].worker_id,atoi(id), atoi(size));
                write(workers[i].pipe_write, line,indexLine);
                break;
            }
        }
        if (full){
            writeLogForward(atoi(id), atoi(size));
            forwarded++;
            write(pipe_Z, line, indexLine);
        }
    }
    closeFile(server_fifo);
}
void getCofactor(int **mat, int **temp, int q, int n){
    int i = 0, j = 0;
    for (int row = 0; row < n; row++)
        for (int col = 0; col < n; col++)
            if (row != 0 && col != q){
                temp[i][j++] = mat[row][col];
                if (j == n - 1){
                    j = 0;
                    i++;
                }
            }  
}
int determinantOfMatrix(int **mat, int n){
    int d = 0;
    if (n == 1)
         return mat[0][0];
    int **temp;
    temp=(int**)malloc(n*sizeof(int*));
    for (int i = 0; i < n; i++){
        temp[i] = (int*)malloc(n * sizeof(int));
        memset(temp[i],0,n);
    }
    int sign = 1; 
    for (int f = 0; f < n; f++){
        getCofactor(mat,temp, f, n);
        d += sign * mat[0][f] * determinantOfMatrix(temp, n - 1);
        sign = -sign;
    }
    for (int i = 0; i < n; i++)
       free(temp[i]);
    free(temp);
    return d;
}
int worker(struct Worker *worker){
    while (1){
        char *buff,temp[1],*buff2;
        sig_check();
        buff= (char *)malloc(1000* sizeof(char));
        buff2=buff;
        memset(buff,'\0',1000);
        memset(temp,'\0',1);
        int buff_index=0;
        while (read(STDIN_FILENO, temp, 1)==1){
            if (flag==1){
                if (buff2!=NULL)
                    free(buff2);
                buff2=NULL;
                sig_check();
            }
            if (temp[0] == '\n' || temp[0] == '\0')
                break;
            buff[buff_index]=temp[0];
            buff_index++;
        }
        worker->availability=1;
        worker->Client_id= atoi(strtok_r(buff, ";", &buff));
        worker->size = atoi(strtok_r(buff, ";", &buff));
        int **mat;
        mat = (int **)malloc((worker->size) * sizeof(int *));
        for (int i = 0; i < worker->size; i++)
            mat[i] = (int *)malloc((worker->size) * sizeof(int));
        
        for (int i = 0; i < worker->size; i++){
            char *line;
            line = strtok_r(buff, ";", &buff);
            for (int j = 0; j < worker->size; j++){
                mat[i][j] = atoi(strtok_r(line, ",", &line));
            }
        }
        free(buff2);
        int result=invertible(mat,worker->size);
        if (result==0){
            kill(getppid(),SIGUSR2);
        }else
            kill(getppid(),SIGUSR1);

        for (int i = 0; i < worker->size; i++)
            free(mat[i]);
        free(mat);
        sig_check();
        /* Open client FIFO (previously created by client) */
        char clientFifo[CLIENT_FIFO_NAME_LEN];
        snprintf(clientFifo, CLIENT_FIFO_NAME_LEN, CLIENT_FIFO_TEMPLATE,(int)worker->Client_id);
        sig_check();
        int clientFd = open(clientFifo, O_WRONLY);
        if (clientFd == -1){ /* Open failed, give up on client */
            writeLogErr("Worker : Opening Client FIFO\n");
            kill(getppid(), SIGRTMIN);
            continue;
        }
        sleep(sleep_time);

        writeLogWorkerResponse(worker->Client_id, result);
        /* Send response and close FIFO */
        if (write(clientFd, &result, sizeof(int)) != sizeof(int))
            writeLogErr("Worker : Writing Client FIFO\n");
        if (close(clientFd) == -1)
            writeLogErr("Worker : Closing Client FIFO\n");

        sig_check();
        kill(getppid(), SIGRTMIN);
    }
}
void createPool(int pool){
    for (int i = 0; i < pool; i++){
        int pipes[2];
        pipe(pipes);//0 read - 1 write
        if (pipes[0] == -1 || pipes[1]==-1)
            writeLogErr("Create Pool : Failed to Create Pipe");
        int child=fork();
        if (child==0){//Child
            close(pipes[1]);
            dup2(pipes[0],STDIN_FILENO);
            worker(&workers[i]);
            exit(EXIT_SUCCESS);
        }else if (child == -1){
            writeLogErr("Create Pool : Couldn't Create Worker");
        }else{//parent
            workers[i].worker_id=child;
            workers[i].server_id = (int)getpid();
            workers[i].pipe_write=pipes[1];
            workers[i].availability = 0;
        }
    }
    sig_check();
    dummyFd = openFIFO();
    sig_check();
    readFIFO(pool);
}
int invertible(int **mat,int size){
    if (determinantOfMatrix(mat, size) != 0)
        return 1;
    else
        return 0;
}
int add_timestamp_worker_pid(char dest[200],int id){
    int index = 0;
    time_t t;
    time(&t);
    if (id==0)
        id = (int)getpid();
    index = sprintf(dest, "%s Worker PID#%d ", ctime(&t), id);
    return index;
}
void closeFile(int fd){
    int err = close(fd);
    if (err == -1)
        writeLogErr("Close File : Failed to Close File");
}
int openFile(char *dataFile, int type){
    int fd = open(dataFile, type,0666);
    if (fd == -1){
        char buf[50];
        sprintf(buf, "Open File : Failed to Open File: %s",dataFile);
        writeLogErr(buf);
        exit(1);
    }
    return fd;
}
void starterLog(){
    char buffer[100];
    time_t t;
    time(&t);
    int index = sprintf(buffer, "%s Server Y (%s, p = %d, t= %d) started. ID: %d\n", ctime(&t), pathToLogFile, poolSize, sleep_time,(int)getpid());
    lock_n_write(logFd,buffer,index);
}
int getBusyWorkerNum(){
    int counter=0;
    for (int i = 0; i < poolSize; i++)
        if (workers[i].availability==1)
            counter++;
    return counter;
}
void writeLogErr(char* buf){
    char temp[1024];
    time_t t;
    time(&t);
    int index = sprintf(temp, "%s Error: %s\n", ctime(&t), buf);
    lock_n_write(logFd, temp, index);
}
void writeLog(char *buf){
    char temp[1024];
    time_t t;
    time(&t);
    int index = sprintf(temp, "%s %s\n", ctime(&t), buf);
    lock_n_write(logFd, temp, index);
}
void writeLogWorkerHandle(int wID,int id,int size){
    char buf[200];
    int index=add_timestamp_worker_pid(buf,wID);
    index += sprintf(buf + index, "is handling Client PID#%d, matrix size %dx%d, pool busy %d / %d\n",id,size,size,getBusyWorkerNum(),poolSize);
    lock_n_write(logFd, buf, index);
}
void writeLogWorkerResponse(int id, int result){
    char buf[200];
    int index = add_timestamp_worker_pid(buf,0);
    index += sprintf(buf + index, "responding to Client PID#%d : ", id );
    if (result==0){
        index += sprintf(buf + index, "the matrix IS NOT invertible.\n");
    }else
        index += sprintf(buf + index, "the matrix IS invertible.\n");
    lock_n_write(logFd, buf, index);
}
void writeLogForward(int id,int size){
    char buf[200];
    time_t t;
    time(&t);
    int index = sprintf(buf, "%s Forwarding request of Client PID#%d to ServerZ, matrix size %dx%d, pool busy %d / %d\n", ctime(&t),id,size,size, getBusyWorkerNum(),poolSize);
    lock_n_write(logFd, buf, index);
}
void lockFile(int fd){
    struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_whence = 0;
    lock.l_start = 0;
    lock.l_len = 0;
    int res = fcntl(fd, F_SETLKW, &lock);
    if (res == -1)
        writeLogErr("Lock File : Failed to Lock File");
}
void unlockFile(int fd){
    struct flock lock;
    lock.l_type = F_UNLCK;
    lock.l_whence = 0;
    lock.l_start = 0;
    lock.l_len = 0;
    int err = fcntl(fd, F_SETLKW, &lock);
    if (err == -1)
        writeLogErr("Unlock File : Failed to Unlock File");
}
void lock_n_write(int fd ,char * buf,int index){
    lockFile(fd);
    write(fd, buf, index);
    unlockFile(fd);
}
void createServerZ(){
    char *argVec[10];
    argVec[9] = NULL;
    argVec[8] = (char *)malloc(sizeof(char) * 8);
    argVec[7] = "-i";
    argVec[6] = (char *)malloc(sizeof(char) * 5);
    sprintf(argVec[6], "%d",poolSize2);
    argVec[5] = "-r";
    argVec[4] = (char *)malloc(sizeof(char) * 5);
    sprintf(argVec[4], "%d",sleep_time);
    argVec[3] = "-t";
    argVec[2] = pathToLogFile;
    argVec[1] = "-l";
    argVec[0] = "./serverZ";

    int pipes[2];
    pipe(pipes); // 0 read - 1 write
    if (pipes[0] == -1 || pipes[1] == -1)
        writeLogErr("Create Server Z : Failed to Create Pipe");
    pipe_Z=pipes[1];
    sprintf(argVec[8], "%d", pipe_Z);
    if ((serverZ_id = fork()) == 0){
        close(pipes[1]);
        dup2(pipes[0], STDIN_FILENO);
        execv(argVec[0], argVec);
    }
    writeLog("Instantiated Server Z.");
    free(argVec[8]);
    free(argVec[6]);
    free(argVec[4]);
}
void wait_child(int childNumber){
    pid_t childPid;
    for (int i = 0; i < childNumber; i++){
        childPid = wait(NULL);
        if (childPid == -1)
            if (errno == ECHILD){
               // writeLog("No More Worker - Bye!\n");
                break;
            }//else
               // writeLog("Wait!\n");
    }
}
void writeLogSIGINT(){
    char temp[1024];
    time_t t;
    time(&t);
    int index = sprintf(temp, "%s SIGINT received, terminating Z and exiting server Y.Total requests handled : %d, %d invertible, %d not .%d requests were forwarded.\n", ctime(&t), (invert + notInvert), invert, notInvert, forwarded);
    lock_n_write(logFd, temp, index);
}
void check_double_instant(){
    double_fd = open(".double", O_CREAT | O_WRONLY | O_EXCL, S_IRUSR | S_IWUSR);
    if (double_fd < 0){ /* failure */
        if (errno == EEXIST){
            printf("Double Instantiation\n");
            exit(1);
        }
    }
}
