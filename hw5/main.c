#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

float **result = NULL;
double **realOut = NULL;
double **imgOut = NULL;
int **first=NULL;
int **second = NULL;
int arrived=0;
int firstFd = -1;
int matrixSize= -1;
int outputFd = -1;
int secondFd = -1;
int threadNumber = 0;

pthread_cond_t cond;
pthread_mutex_t mutex;

sig_atomic_t flag = 0;
struct timeval start;

char *get_timestamp();
double getTime(struct timeval second, struct timeval first);
int fourierTransform(int start, int end);
int matrix_product(float **result, int start, int end);
int openFile(char *dataFile, int type);
int readFile(int fd, int **arr);
int sig_check_thread();
void *threadX(void *in);
void checkArgc(int argc, char *argv[]);
void closeFile(int fd);
void create_mutex();
void create_threads(int *arr, pthread_t *threads);
void free_arrays();
void handler(int sig_number);
void initialize_mutex();
void join_threads(pthread_t *threads);
void perror_call(char *er);
void remove_mutex();
void sig_check();
void startOperations(pthread_t *threads);
void writeFile();

int main(int argc, char *argv[]){
    struct sigaction act;
    memset(&act, '\0', sizeof(act));
    act.sa_handler = &handler;
    sigaction(SIGINT, &act, NULL);

    checkArgc(argc, argv);
        
    first = (int **)malloc(sizeof(int*) *matrixSize);
    second = (int **)malloc(sizeof(int*) * matrixSize);
    result = (float **)malloc(sizeof(float*) * matrixSize);
    realOut = (double **)malloc(sizeof(double *) * matrixSize);
    imgOut = (double **)malloc(sizeof(double *) * matrixSize);
    for (int i = 0; i <matrixSize; i++){
        first[i] = (int *)malloc(sizeof(int) * matrixSize);
        second[i] = (int *)malloc(sizeof(int) * matrixSize);
        result[i] = (float *)malloc(sizeof(float) * matrixSize);
        realOut[i] = (double *)malloc(sizeof(double ) * matrixSize);
        imgOut[i] = (double *)malloc(sizeof(double ) * matrixSize);
        memset(first[i],0,matrixSize);
        memset(second[i], 0, matrixSize);
        memset(result[i], 0, matrixSize);
        memset(realOut[i], 0, matrixSize);
        memset(imgOut[i], 0, matrixSize);
    }
    gettimeofday(&start, NULL);
    if (readFile(firstFd, first) == -1 || readFile(secondFd, second)==-1){
        closeFile(firstFd);
        closeFile(secondFd);
        closeFile(outputFd);
        perror_call("Insufficient Number of Characters in Input File\n");
    }

    printf("%s Two matrices of size %dx%d have been read. The number of threads is %d.\n",get_timestamp(), matrixSize,matrixSize,threadNumber);
    pthread_t threads[threadNumber];
    initialize_mutex();
    startOperations(threads);
    sig_check();

    struct timeval end;
    gettimeofday(&end, NULL);
    printf("%s The process has written the output file. The total time spent is %.4f seconds.\n",get_timestamp(), getTime(end,start));

    closeFile(firstFd);
    closeFile(secondFd);
    closeFile(outputFd);
    remove_mutex();

    free_arrays();
    return 0;
}
void checkArgc(int argc, char *argv[]){
    int m=0;
    if (argc != 11){
        perror_call("Usage : ./hw5 [-i] filePath1 [-j] filePath2 [-o] output [-n] 4 [-m] 2\n");
    }else{
        for (int i = 1; i < argc; i += 2)
            switch (argv[i][1]){
                case 'i':
                    firstFd = openFile(argv[i + 1], O_RDONLY);
                    break;
                case 'j':
                    secondFd = openFile(argv[i + 1], O_RDONLY);
                    break;
                case 'o':
                    outputFd = openFile(argv[i + 1], O_WRONLY|O_CREAT|O_TRUNC);
                    break;
                case 'n':
                    m = atoi(argv[i + 1]);
                    if (m <= 2)
                        perror_call("Insufficient Matrix Size\n");
                    matrixSize = (int)pow(2, m);
                    break;
                case 'm':
                    threadNumber = atoi(argv[i + 1]);
                    if (threadNumber < 2 || (threadNumber % 2 != 0))
                        perror_call("Insufficient Thread Number\n");
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
        printf("%s SIGINT RECEIVED...\n",get_timestamp());
        closeFile(firstFd);
        closeFile(secondFd);
        closeFile(outputFd);
        free_arrays();
        remove_mutex();
        exit(0);
    }
}
void free_arrays(){
    for (int i = 0; i < matrixSize; i++){
        free(first[i]);
        free(second[i]);
        free(result[i]);
        free(realOut[i]);
        free(imgOut[i]);
    }
    free(first);
    free(second);
    free(result);
    free(realOut);
    free(imgOut);
}
int sig_check_thread(){
    if (flag == 1){
        return 1;
    }else
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
    int fd = open(dataFile, type,0666);
    if (fd == -1){
        printf("Failed to Open File: %s\n", dataFile);
        exit(1);
    }
    return fd;
}
void startOperations(pthread_t *threads){
    int arr[threadNumber];
    create_threads(arr,threads);
    join_threads(threads);
    sig_check();
    writeFile();
}
void create_threads(int *arr,pthread_t *threads){
    for (int i = 0; i < threadNumber; i++){
        arr[i]=i;
        void *p=&arr[i];
        if (pthread_create(&threads[i], NULL, threadX, p) != 0)
            perror_call("pthread_create");
    }
}
void join_threads(pthread_t *threads){
    for (int i = 0; i < threadNumber; i++)
        if (pthread_join(threads[i], NULL) != 0)
            perror_call("pthread_join");
}
void perror_call(char *er){
    perror(er);
    closeFile(firstFd);
    closeFile(secondFd);
    closeFile(outputFd);
    remove_mutex();
    exit(EXIT_FAILURE);
}
void initialize_mutex(){
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond,NULL);
}
void remove_mutex(){
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
}
char* get_timestamp(){
    time_t t;
    time(&t);
    char *tm = ctime(&t);
    tm[strlen(tm)-1]=' ';
    return tm;
}
int matrix_product(float** result,int start ,int end){
    for (int i = 0; i < matrixSize; i++)
        for (int j = start; j < end; j++)
            for (int k = 0; k < matrixSize; k++){
                result[i][j] += first[i][k] * second[k][j];
                if (sig_check_thread() == 1)
                    return 1;
            }
    return 0;
}
int readFile(int fd,int **arr){
    char buf[matrixSize * matrixSize];
    if (read(fd, buf, matrixSize * matrixSize) != matrixSize * matrixSize)
        return -1;

    for (int i = 0; i < matrixSize * matrixSize; i++)
        arr[i/matrixSize][i%matrixSize]=(int)buf[i];
    
    return 0;
}
void* threadX(void *in){
    int id = *((int *)in);
    struct timeval mid,end;
    printf("thread %d, start %d ,end %d\n", id, id * matrixSize / threadNumber, (id + 1) * matrixSize / threadNumber);
    int e = matrix_product(result, id * matrixSize / threadNumber, (id + 1) * matrixSize / threadNumber);
    if (sig_check_thread() == 1 || e==1)
        return NULL;
    gettimeofday(&mid, NULL);
    printf("%s Thread %d has reached the rendezvous point in %.4f seconds.\n",get_timestamp(), id + 1, getTime(mid,start));
    
    pthread_mutex_lock(&mutex);

    arrived++;
    while (arrived<threadNumber){
        pthread_cond_wait(&cond,&mutex);
        if (sig_check_thread() == 1 )
            return NULL;
    }
    pthread_cond_broadcast(&cond);

    if (sig_check_thread() == 1)
        return NULL;
    printf("%s Thread %d is advancing to the second part\n",get_timestamp(), id+1);

    pthread_mutex_unlock(&mutex);
    
    if (sig_check_thread() == 1)
        return NULL;
    e=fourierTransform(id * matrixSize / threadNumber, (id + 1) * matrixSize / threadNumber);
    if (sig_check_thread() == 1 || e == 1)
        return NULL;
    gettimeofday(&end, NULL);
    printf("%s Thread %d has has finished the second part in %.4f seconds.\n",get_timestamp(), id + 1, getTime(end,mid));
    return NULL;
}
int fourierTransform(int start,int end){
    for (int yWave = 0; yWave < matrixSize; yWave++) // Two inner loops iterate on output data.
        for (int xWave = start; xWave < end; xWave++)
            
            for (int ySpace = 0; ySpace < matrixSize; ySpace++){ // Two inner loops iterate on input data.
                for (int xSpace = 0; xSpace < matrixSize; xSpace++){
                    realOut[yWave][xWave] += (result[ySpace][xSpace] * 
                        cos(-2 * M_PI * ((1.0 * xWave * xSpace / matrixSize)
                                     + (1.0 * yWave * ySpace / matrixSize))));
                    imgOut[yWave][xWave] += (result[ySpace][xSpace] *
                        sin(-2 * M_PI * ((1.0 * xWave * xSpace / matrixSize)
                                     + (1.0 * yWave * ySpace / matrixSize))));

                    if (sig_check_thread() == 1)
                        return 1;
                }
            }
    return 0;
}
void writeFile(){
    for (int i = 0; i < matrixSize; i++)
        for (int j = 0; j <matrixSize; j++){
            dprintf(outputFd, "%.3f+ %.3fi", realOut[i][j], imgOut[i][j]);
            if (j<matrixSize-1){
                dprintf(outputFd, ",");
            }else
                dprintf(outputFd, "\n");            
        }
}
double getTime(struct timeval second, struct timeval first){
    long seconds = (second.tv_sec - first.tv_sec);
    long micros = ((seconds * 1000000) + second.tv_usec) - (first.tv_usec);
    return (double)micros/CLOCKS_PER_SEC;
}