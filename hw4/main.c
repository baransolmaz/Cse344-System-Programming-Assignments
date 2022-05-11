#include "semun.h"
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define FTOK_NAME "hw4"

int consumersNumber = 0;
int inputFd=-1;
int loopTime= 0;
int semid=0;
key_t key = 0;
sig_atomic_t flag = 0;

char* get_timestamp();
int openFile(char *dataFile, int type);
void checkArgc(int argc, char *argv[]);
void closeFile(int fd);
void create_sems();
void create_threads(int* arr,pthread_t *sup, pthread_t *threads);
void handler(int sig_number);
void initialize_sems();
void join_threads(pthread_t *threads);
void remove_sems();
void semPost(int sem_index);
void semWait_All();
void sig_check();
int sig_check_consumer();
int sig_check_supplier();
void startOperations(pthread_t *sup,pthread_t *threads);
void* consumer(void *in);
void* supplier();
void perror_call(char *er);

int main(int argc, char *argv[]){
    struct sigaction act;
    memset(&act, '\0', sizeof(act));
    act.sa_handler = &handler;
    sigaction(SIGINT, &act, NULL);

    checkArgc(argc, argv);
    pthread_t consumer_threads[consumersNumber];
    pthread_t supplier_thread;
    create_sems();
    initialize_sems();
    startOperations(&supplier_thread,consumer_threads);
    sig_check();
    remove_sems();
    
    return 0;
}
void checkArgc(int argc, char *argv[]){
    if (argc != 7){
        perror_call("Usage : ./hw4 [-C] ConsumerNumber [-N] LoopTime [-F] inputfilePath\n");
    }else{
        for (int i = 1; i < argc; i += 2)
            switch (argv[i][1]){
                case 'F':
                    inputFd = openFile(argv[i + 1],O_RDONLY);
                    break;
                case 'C':
                    consumersNumber = atoi(argv[i + 1]);
                    if (consumersNumber <= 4)
                        perror_call("Insufficient Consumers Number, Must be greater than 4.\n");
                    break;
                case 'N':
                    loopTime = atoi(argv[i + 1]);
                    if (loopTime <= 1)
                        perror_call("Insufficient Loop Time, Must be greater than 1.\n");
                    break;
            }
    }
}
void handler(int sig_number){
    if (sig_number == SIGINT){
        flag = 1;
    }
}
void sig_check(){
    if (flag == 1){
        closeFile(inputFd);
        printf("MAIN\n");
        remove_sems();
        exit(0);
    }
}
int sig_check_supplier(){
    if (flag == 1){
        printf("Supplier\n");
        return 1;
    }
    return 0; 
}
int sig_check_consumer(){
    if (flag == 1){
        printf("Consumer\n");
        for (int i = 0; i < consumersNumber; i++){
            semPost(0);
            semPost(1);
        }
        return 1;
    }
    return 0;
}
void closeFile(int fd){
    if (fd!=-1){
        int err2 = close(fd);
        if (err2 == -1)
            perror_call("Failed to Close File");
    }    
}
int openFile(char *dataFile, int type){
    int fd = open(dataFile, type);
    if (fd == -1){
        printf("Failed to Open File: %s", dataFile);
        exit(1);
    }
    return fd;
}
void startOperations(pthread_t *sup, pthread_t *threads){
    int arr[consumersNumber];
    create_threads(arr,sup,threads);
    join_threads(threads);
    sig_check();
}
void *consumer(void* in){
    int id = *((int*)in);
    for (int i = 0; i <loopTime; i++){
        if (sig_check_consumer() == 1)
            break;
        printf("%s Consumer-%d at iteration %d (waiting).Current amounts: %d x '1', %d x '2'.\n",
              get_timestamp(), id, i, semctl(semid, 0, GETVAL, NULL),semctl(semid, 1, GETVAL, NULL));
        if (sig_check_consumer() == 1)
            break;
        semWait_All();

        if (sig_check_consumer() == 1)
            break;
        printf("%s Consumer-%d at iteration %d (consumed).Post-consumption amounts: %d x '1', %d x '2'.\n",
              get_timestamp(), id, i, semctl(semid, 0, GETVAL, NULL),semctl(semid, 1, GETVAL, NULL));
    }
    printf("%s Consumer-%d has left.\n", get_timestamp(), id);
    return NULL;
}
void* supplier(){
    while (1){
        char buffer[3];
        memset(buffer, '\0', 3);
        if(sig_check_supplier()==1)
            break;
        if (read(inputFd, buffer, 1) != 1)//EOF
            break;

        if (sig_check_supplier() == 1)
            break;
        printf("%s Supplier: read from input a '%c'. Current amounts: %d x '1', %d x '2'.\n", get_timestamp(), buffer[0], semctl(semid, 0, GETVAL, NULL), semctl(semid, 1, GETVAL, NULL));
        switch (buffer[0]){
            case '1':
                semPost(0);
                break;
            case '2':
                semPost(1);
                break;
            default:
                break;
        }
        printf("%s Supplier: delivered a '%c'. Post-delivery amounts: %d x '1', %d x '2'.\n", get_timestamp(), buffer[0], semctl(semid, 0, GETVAL, NULL), semctl(semid, 1, GETVAL, NULL));
    }
    printf("%s The Supplier has left.\n", get_timestamp());
    return NULL;
}
void create_threads(int *arr,pthread_t *sup,pthread_t *threads){
    for (int i = 0; i < consumersNumber; i++){
        arr[i]=i;
        void *p=&arr[i];
        if (pthread_create(&threads[i], NULL, consumer, p) != 0)
            perror_call("pthread_create");
    }
    if (pthread_create(sup, NULL, supplier, NULL) != 0)
        perror_call("pthread_create");

    if (pthread_detach(*sup) != 0)
        perror_call("pthread_detach");
}
void join_threads(pthread_t *threads){
    for (int i = 0; i < consumersNumber; i++)
        if (pthread_join(threads[i], NULL) != 0)
            perror_call("pthread_join");
}
void perror_call(char *er){
    perror(er);
    exit(EXIT_FAILURE);
}
void create_sems(){
    key = ftok(FTOK_NAME, 1);
    if (key == -1)
        perror_call("ftok");
    
    semid = semget(key, 2, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    if (semid == -1)
        perror_call("semget");
}
void initialize_sems(){
    union semun arg;
    unsigned short arr[2] = {0, 0};
    arg.array = arr;
    if (semctl(semid, 0, SETALL, arg) == -1)
        perror_call("semctl-initialize");
}
void remove_sems(){
    if (semctl(semid, 0, IPC_RMID) == -1)
        perror_call("semctl-remove");
}
void semPost(int sem_index){
    struct sembuf buf={0};
    buf.sem_num=sem_index;
    buf.sem_op=1;
    buf.sem_flg=0;
    semop(semid,&buf,1);
}
void semWait_All(){
    struct sembuf buf[2] = {{0},{0}};
    for (int i = 0; i < 2; i++){
        buf[i].sem_num = i;
        buf[i].sem_op =-1;
        buf[i].sem_flg = 0;
    }
    semop(semid,buf,2);
}
char* get_timestamp(){
    time_t t;
    time(&t);
    char *tm = ctime(&t);
    tm[strlen(tm)-1]=' ';
    return tm;
}