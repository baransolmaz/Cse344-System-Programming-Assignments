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
#include <sys/mman.h>
#include <semaphore.h>
#include "become_daemon.h"
#define MAX_QUEUE_SIZE 1024
sig_atomic_t flag = 0;
sig_atomic_t unlink_flag = -1;
struct Worker{
    int Client_id;
    int size;
    int availability;
    int worker_id;
};
struct Shared{
    char queue[MAX_QUEUE_SIZE][MAX_QUEUE_SIZE];
    int current_index;
    int end_index;
    int worker_size;
    int working;
    sem_t full;
    sem_t empty;
    sem_t lock;
}shared;

#define SHARED_MEMORY_NAME "/shared"
#define CLIENT_FIFO_TEMPLATE "/tmp/client.%d"
#define CLIENT_FIFO_NAME_LEN (sizeof(CLIENT_FIFO_TEMPLATE) + 20)
void read_pipe();
int determinantOfMatrix(int **mat, int n);
void getCofactor(int **mat, int **temp, int q, int n);
int invertible(int **mat, int n);
void handler(int sig_number);
void checkArgc(int argc, char *argv[]);
int add_timestamp_worker_pid(char dest[200], int id);
void closeFile(int fd);
int openFile(char *dataFile, int type);
void starterLog();
void memory_create();
void memory_read();
void memory_write(char *buf, int bufSize);
void memory_unlink();
void writeLogErr(char *buf);
void writeLog(char *buf);
void writeLogWorkerHandle(int wid,int id, int size);
void writeLogWorkerResponse(int id, int result);
void wait_child(int childNumber);
void lockFile(int fd);
void unlockFile(int fd);
void lock_n_write(int fd, char *buf, int index);
void createWorkers();
void sig_check2();
char *pathToLogFile;
int logFd, pipe_in;
int poolSize,sleep_time;
int invert=0,notInvert=0;
int shr_mem_fd;
void* addr;
struct Worker *workers;
struct Shared *shr_ptr;
int main(int argc, char *argv[]){
    struct sigaction act;
    memset(&act, '\0', sizeof(act));
    act.sa_handler = &handler;
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGUSR1, &act, NULL);
    sigaction(SIGUSR2, &act, NULL);
    checkArgc(argc,argv);
    starterLog();
    memory_create();
    shr_ptr=(struct Shared*) addr;
    createWorkers();
    read_pipe();
    


    return 0;
}
void checkArgc(int argc, char *argv[]){
    for (int i = 1; i < argc; i += 2){
        switch (argv[i][1]){
            case 'i':
                pipe_in = atoi(argv[i + 1]);
                break;
            case 'l':
                pathToLogFile = argv[i + 1];
                logFd = openFile(pathToLogFile, O_APPEND | O_WRONLY | O_CREAT);
                break;
            case 'r':
                poolSize = atoi(argv[i + 1]);
                break;
            case 't':
                sleep_time = atoi(argv[i + 1]);
                break;
            default:
                break;
        }
    }
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
    temp = (int **)malloc(n * sizeof(int *));
    for (int i = 0; i < n; i++){
        temp[i] = (int *)malloc(n * sizeof(int));
        memset(temp[i], 0, n);
    }
    int sign = 1;
    for (int f = 0; f < n; f++){
        getCofactor(mat, temp, f, n);
        d += sign * mat[0][f] * determinantOfMatrix(temp, n - 1);
        sign = -sign;
    }
    for (int i = 0; i < n; i++)
        free(temp[i]);
    free(temp);
    return d;
}
int invertible(int **mat, int size){
    if (determinantOfMatrix(mat, size) != 0)
        return 1;
    else
        return 0;
}
void handler(int sig_number){
    if (sig_number == SIGINT){
        flag = 1;
    }else if (sig_number == SIGUSR1){
        kill((int)getppid(), SIGUSR1);
        invert++;
    }else if (sig_number == SIGUSR2){
        kill((int)getppid(), SIGUSR2);
        notInvert++;
    }
}
int add_timestamp_worker_pid(char dest[200],int id){
    int index = 0;
    time_t t;
    time(&t);
    if (id==0)
        id = (int)getpid();
    index = sprintf(dest, "%s Z: Worker PID#%d ", ctime(&t), id);
    return index;
}
void closeFile(int fd){
    int err = close(fd);
    if (err == -1)
        writeLogErr("Close File : Failed to Close File");
}
int openFile(char *dataFile, int type){
    int fd = open(dataFile, type, 0666);
    if (fd == -1){
        char buf[60];
        sprintf(buf, "Open File : Failed to Open File: %s", dataFile);
        writeLogErr(buf);
        exit(1);
    }
    return fd;
}
void starterLog(){
    char buffer[100];
    time_t t;
    time(&t);
    int index = sprintf(buffer, "%s Z: Server Z (%s, p = %d, t= %d) started. ID: %d\n", ctime(&t), pathToLogFile, poolSize, sleep_time,(int)getpid());
    lock_n_write(logFd, buffer, index);
}
void writeLogErr(char *buf){
    char temp[1024];
    time_t t;
    time(&t);
    int index = sprintf(temp, "%s Z: Error: %s\n", ctime(&t), buf);
    lock_n_write(logFd, temp, index);
}
void writeLog(char *buf){
    char temp[1024];
    time_t t;
    time(&t);
    int index = sprintf(temp, "%s Z: %s\n", ctime(&t), buf);
    lock_n_write(logFd, temp, index);
}
void writeLogWorkerHandle(int wid,int id, int size){
    char buf[200];
    int index = add_timestamp_worker_pid(buf,wid);
    index += sprintf(buf + index, "is handling Client PID#%d, matrix size %dx%d, pool busy %d / %d\n", id, size, size, shr_ptr->working, poolSize);
    lock_n_write(logFd, buf, index);
}
void writeLogWorkerResponse(int id, int result){
    char buf[200];
    int index = add_timestamp_worker_pid(buf,0);
    index += sprintf(buf + index, "responding to Client PID#%d : ", id);
    if (result == 0){
        index += sprintf(buf + index, "the matrix IS NOT invertible.\n");
    }else
        index += sprintf(buf + index, "the matrix IS invertible.\n");
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
void lock_n_write(int fd, char *buf, int index){
    lockFile(fd);
    write(fd, buf, index);
    unlockFile(fd);
}
void writeLogSIGINT(){
    char temp[1024];
    time_t t;
    time(&t);
    int index = sprintf(temp, "%s Z: SIGINT received, exiting Z .Total requests handled : %d, %d invertible, %d not .\n", ctime(&t), (invert + notInvert), invert, notInvert);
    lock_n_write(logFd, temp, index);
}
void sig_check(){
    if (flag == 1){
        if (workers!=NULL){
            for (int i = 0; i < poolSize; i++)
                kill(workers[i].worker_id, SIGINT);
            free(workers);
            wait_child(poolSize);
            workers=NULL;
        }
        
        sem_destroy(&(shr_ptr->empty));
        sem_destroy(&(shr_ptr->full));
        sem_destroy(&(shr_ptr->lock));
        memory_unlink();
        writeLogSIGINT();
        exit(1);
    }
}
void sig_check2(){
    if (flag == 1)
        exit(1);
}
void wait_child(int childNumber){
    pid_t childPid;
    for (int i = 0; i < childNumber; i++){
        childPid = wait(NULL);
        if (childPid == -1)
            if (errno == ECHILD){
                //writeLog("No More Worker - Bye!");
                break;
            }//else
               // writeLog("Wait! Worker Exist!");
    }
}
void read_pipe(){
    while (1){
        char *buff, temp[1];
        sig_check();
        buff = (char *)malloc(1000 * sizeof(char));
        memset(buff, '\0', 1000);
        memset(temp, '\0', 1);
        int buff_index = 0;
        while (read(STDIN_FILENO, temp, 1) == 1){
            sig_check();
            if (temp[0] == '\n' || temp[0] == '\0')
                break;
            buff[buff_index] = temp[0];
            buff_index++;
        }
        if ((int)strlen(buff)<=1)
            continue;
        memory_write(buff,buff_index);
        free(buff);
        sig_check();
    }
}
void worker(struct Worker *worker){
    while (1){
        char *line=NULL;
        sig_check2();
        memory_read(&line);
        sig_check2();
        if (((int)strlen(line) <= 1 )|| line==NULL)
            continue;
        sig_check2();
        (shr_ptr->working)++;
        worker->availability = 1;
        worker->Client_id = atoi(strtok_r(line, ";", &line));
        worker->size = atoi(strtok_r(line, ";", &line));
        writeLogWorkerHandle((int)getpid(),worker->Client_id,worker->size);
        sig_check2();
        int **mat;
        mat = (int **)malloc((worker->size) * sizeof(int *));
        for (int i = 0; i < worker->size; i++)
            mat[i] = (int *)malloc((worker->size) * sizeof(int));
        for (int i = 0; i < worker->size; i++){
            char *row;
            row = strtok_r(line, ";", &line);
            for (int j = 0; j < worker->size; j++)
                mat[i][j] = atoi(strtok_r(row, ",", &row));
        }
        int result = invertible(mat, worker->size);
        for (int i = 0; i < worker->size; i++)
            free(mat[i]);
        free(mat);
        sig_check2();
        if (result == 0){
            kill((int)getppid(), SIGUSR2);
        }else
            kill((int)getppid(), SIGUSR1);
        char clientFifo[CLIENT_FIFO_NAME_LEN];
        snprintf(clientFifo, CLIENT_FIFO_NAME_LEN, CLIENT_FIFO_TEMPLATE, (int)worker->Client_id);
        sig_check2();
        int clientFd = open(clientFifo, O_WRONLY);
        if (clientFd == -1){ /* Open failed, give up on client */
            writeLogErr("Worker : Opening Client FIFO\n");
            (shr_ptr->working)--;
            continue;
        }
        sleep(sleep_time); 
        writeLogWorkerResponse(worker->Client_id, result);
        /* Send response and close FIFO */
        if (write(clientFd, &result, sizeof(int)) != sizeof(int))
            writeLogErr("Worker : Writing Client FIFO\n");
        if (close(clientFd) == -1)
            writeLogErr("Worker : Closing Client FIFO\n");
        (shr_ptr->working)--;
        sig_check2();
    }
    
}
void memory_write(char* buf,int bufSize){
    sem_wait(&(shr_ptr->empty));
    sem_post(&(shr_ptr->lock));
    strncpy(shr_ptr->queue[shr_ptr->end_index], buf, bufSize);
    shr_ptr->end_index = ((shr_ptr->end_index )+ 1) % MAX_QUEUE_SIZE;
    sem_wait(&(shr_ptr->lock));
    sem_post(&(shr_ptr->full));
}
void memory_create(){
    shr_mem_fd = shm_open(SHARED_MEMORY_NAME, O_CREAT | O_RDWR, 0777);
    if (shr_mem_fd == -1)
        writeLogErr("Memory Create : shm_open");
    unlink_flag=0;
    int size = sizeof(struct Shared);
    if (ftruncate(shr_mem_fd, size) == -1)
        writeLogErr("Memory Create : ftruncate");
    struct Shared temp;
    temp.current_index = 0;
    temp.end_index = 0;
    temp.worker_size = poolSize;
    temp.working = 0;
    sem_init(&(temp.empty), 1, MAX_QUEUE_SIZE);
    sem_init(&(temp.full), 1, 0);
    sem_init(&(temp.lock), 1, 1);
    memset(temp.queue, 0, MAX_QUEUE_SIZE * MAX_QUEUE_SIZE);

    addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shr_mem_fd, 0);
    if (addr == MAP_FAILED)
        writeLogErr("Memory Create : mmap");
    memcpy(addr, &temp, size);
    if (close(shr_mem_fd) == -1)
        writeLogErr("Memory Create: close");
}
void memory_read(char **line){
    sem_wait(&(shr_ptr->full));
    sem_post(&(shr_ptr->lock));
    if (shr_ptr->current_index <= shr_ptr->end_index){
        *line = shr_ptr->queue[shr_ptr->current_index];
        shr_ptr->current_index = ((shr_ptr->current_index) + 1) % MAX_QUEUE_SIZE;
    }
    sem_wait(&(shr_ptr->lock));
    sem_post(&(shr_ptr->empty));
}
void memory_unlink(){
    if(unlink_flag==0){
        if (shm_unlink( SHARED_MEMORY_NAME) == -1){
            //writeLogErr("Memory Unlink: shm_unlink");
        }
        unlink_flag=1;
    }
}
void createWorkers(){
    workers = (struct Worker *)malloc(poolSize * sizeof(struct Worker));
    for (int i = 0; i < poolSize; i++){
        int child = fork();
        if (child == 0){ // Child
            worker(&workers[i]);
            exit(EXIT_SUCCESS);
        }else if (child == -1){
            writeLogErr("Create Workers : Couldn't Create Worker");
        }else{ // parent
            workers[i].worker_id = child;
            workers[i].availability = 0;
        }
    }
    sig_check();
}
