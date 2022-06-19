#include "../libs/helper.h"
#include <arpa/inet.h>

sig_atomic_t flag = 0;

char *requestFile;
int _PORT_;
char *_IP_;
int requestCount=0;
char **requests;
int arrived = 0;

pthread_cond_t cond;
pthread_mutex_t mutex;

void *client(void *in);//client process
void checkArgc(int argc, char *argv[]);//Arg check
void createThreads(char arr[][80], pthread_t *threads);
void handler(int sig_number);
void initialize_mutex();
void joinThreads(pthread_t *threads);
void readFile();//To read given file
void remove_mutex();
void sig_check();
int sig_check_thread();
int socketOperations(char *req);//Socket op. for client

int main(int argc, char *argv[]){
    struct sigaction act;
    memset(&act, '\0', sizeof(act));
    act.sa_handler = &handler;
    sigaction(SIGINT, &act, NULL);

    checkArgc(argc, argv);
    sig_check();
    requests   = (char **)malloc(sizeof(char *) * (requestCount + 1));
    requests[0]= (char *)malloc(sizeof(char) * 75);
    memset(requests[0], '\0', 75);
    readFile();
    sig_check();
    char arr[requestCount][80];
    pthread_t threads[requestCount];
    initialize_mutex();
    createThreads(arr,threads);

    joinThreads(threads);
    sig_check();
    
    for (int i = 0; i <requestCount+1; i++)
        free(requests[i]);
    free(requests);
    remove_mutex();
    printf("%sClient : All threads have terminated,goodbye.\n",getTimeStamp());
    return 0;
}
void checkArgc(int argc, char *argv[]){
    if (argc != 7){
        perror_call("Usage : ./client -r requestFile -q PORT -s IP\n");
    }else
        for (int i = 1; i < argc; i += 2)
            switch (argv[i][1]){
                case 'r':
                    requestFile = argv[i + 1];
                    break;
                case 'q':
                    _PORT_ = atoi(argv[i + 1]);
                    break;
                case 's':
                    _IP_ = argv[i + 1];
                    break;
            }
}
void handler(int sig_number){
    flag = 1;
}
void sig_check(){
    if (flag == 1){
        if (requests!=NULL){   
            for (int i = 0; i < requestCount + 1; i++)
                free(requests[i]);
            free(requests);
        }
        remove_mutex();
        exit(1);
    }
}
void readFile(){
    int index=0,fd=openFile(requestFile,O_RDONLY);
    char buf[2];
    memset(buf,'\0',2);

    while (read(fd, buf, 1)==1){
        if (buf[0]=='\n'){
            if (index!=0){
                requestCount++;
                requests = realloc(requests, sizeof(char *) * (requestCount + 1));
                requests[requestCount] = (char *)malloc(sizeof(char) * 75);
                memset(requests[requestCount], '\0', 75);
            }
            index=0;
        }else{
            requests[requestCount][index]=buf[0];
            index++;
        }        
    }
    closeFile(fd);
}
void createThreads(char arr[][80], pthread_t *threads){
    printf("%s Client: I have loaded %d requests and I'm creating %d threads.\n", getTimeStamp(), requestCount, requestCount);
    for (int i = 0; i < requestCount; i++){
        sprintf(arr[i],"%d:%s",i,requests[i]);
        void *p = &arr[i];
        if (pthread_create(&threads[i], NULL,client, p) != 0)
            perror_call("pthread_create");
    }
}
void joinThreads(pthread_t *threads){
    for (int i = 0; i < requestCount; i++)
        if (pthread_join(threads[i], NULL) != 0)
            perror_call("pthread_join");
}
void *client(void *input){
    char *req;
    req = (char *)input;
    char *token = strtok_r(req, ":", &req);
    int id=atoi(token);
    printf("%sClient-Thread-%d: Thread-%d has been created\n",getTimeStamp(),id,id);
    pthread_mutex_lock(&mutex);
    arrived++;
    while (arrived < requestCount){ // Thread synchronization
        if (sig_check_thread() == 1)
            break;
        pthread_cond_wait(&cond, &mutex);
        if (sig_check_thread() == 1)
            break;
    }
    pthread_cond_broadcast(&cond);//Wake others
    pthread_mutex_unlock(&mutex);
    printf("%sClient-Thread-%d: I am requesting “/%s”.\n",getTimeStamp(),id,req);

    int result=socketOperations(req);//Do socket Op.
    printf("%sClient-Thread-%d: The server's response to “/%s” is %d.\n",getTimeStamp(),id,req,result);
    printf("%sClient-Thread-%d: Terminating.\n",getTimeStamp(), id);
    return NULL;
}
void initialize_mutex(){
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);
}
void remove_mutex(){
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
}
int sig_check_thread(){
    if (flag == 1)
        return 1;
    return 0;
}
int socketOperations(char *req){
    int sd = -1, client_fd=-1;
    struct sockaddr_in serv_addr;
    
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0)//Open socket
        perror_call("socket failed");

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(_PORT_);

    if (inet_pton(AF_INET, _IP_, &serv_addr.sin_addr) <= 0)
        perror_call("Invalid address/ Address not supported \n");
    // Connect to socket
    if ((client_fd = connect(sd, (struct sockaddr *)&serv_addr,sizeof(serv_addr))) < 0)
        perror_call("Client:Connection Failed\n");
    
    char arr[80];
    sprintf(arr, "c:%s",req);
    
    send(sd, arr, strlen(arr), 0);//Request send
    char buffer[100] = {0};
    memset(buffer,'\0',100);
    read(sd, buffer, 100);//Get Response

    close(client_fd);
    close(sd);
    return atoi(buffer);//Return result
}