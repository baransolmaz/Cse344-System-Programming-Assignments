#include "../libs/helper.h"
#include "../libs/queue.h"
#include <arpa/inet.h>
sig_atomic_t flag = 0;

struct Servant{
    char start[20];
    char end[20];
    int id;
    int port;
    int fd;
};
int servantCount=0;
int numberOfThreads;
int _SERVER_PORT_;
int totalRequests=0;
struct Servant *servants=NULL;
struct Queue *queue=NULL;
struct sockaddr_in *address=NULL;

pthread_cond_t cond;
pthread_mutex_t mutex;

char *getCity(char *req, int *index); //Returns city or null
int sendReqToServant(char *servant_ip, int servant_port, char *req);//sends request to servant
int sendReqToServants(char *req);   //To send req to all servants
int sendToServant(char *req); // sends to servant
int sig_check_thread(); //checks SIGINT in thread
int socketOperations();//socket op.
struct sockaddr_in *createSocket(struct sockaddr_in *address, int *server_fd);//To create socket
void *server(void *input);//server thread process
void addServant(char *buffer);//adds servant to array
void checkArgc(int argc, char *argv[]);//checks arg.
void clientRequest(int socket_fd, char *buffer);//To handle client req
void createThreads(int *arr, pthread_t *threads);
void freeQueue();//free Queue
void handler(int sig_number);
void initialize_mutex();
void joinThreads(pthread_t *threads);
void remove_mutex();
void sendResToClient(int sd, char *req);//send response to client
void sig_check();//main thread sigint check

int main(int argc, char *argv[]){
    struct sigaction act;
    memset(&act, '\0', sizeof(act));
    act.sa_handler = &handler;
    sigaction(SIGINT, &act, NULL);
    checkArgc(argc, argv);
    int arr[numberOfThreads];
    pthread_t server_threads[numberOfThreads];
    queue = createQueue();
    servants=(struct Servant*)malloc(sizeof(struct Servant));
    initialize_mutex();
    int server_fd, new_socket;
    int addrlen = sizeof(address);
    struct sockaddr_in *address = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
    struct sockaddr_in *head = address;
    address=createSocket(address,&server_fd);
    
    createThreads(arr, server_threads);

    while (1){
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address,(socklen_t *)&addrlen)) < 0)
           break;
        if (sig_check_thread() == 1)
            break;
        
        pthread_mutex_lock(&mutex);
        enqueue(queue,new_socket);
        pthread_mutex_unlock(&mutex);
        pthread_cond_signal(&cond);
        if (sig_check_thread() == 1)
            break;
    }
    pthread_cond_broadcast(&cond);
    joinThreads(server_threads);
    for (int i = 0; i < servantCount; i++)
        kill(servants[i].id,SIGINT);

    if (servants != NULL)
        free(servants);

    shutdown(server_fd, SHUT_RDWR);
    close(server_fd);
     if (head!=NULL)
        free(head);
    freeQueue();
    remove_mutex();
    sig_check();
    return 0;
}
void freeQueue(){
    free(queue->array);
    free(queue);
}
void checkArgc(int argc, char *argv[]){
    if (argc != 5){
        perror_call("Usage : ./server -p PORT -t numberOfThreads\n");
    }else
        for (int i = 1; i < argc; i += 2)
            switch (argv[i][1]){
                case 't':
                    numberOfThreads = atoi(argv[i + 1]);
                    if (numberOfThreads<5)
                        perror_call("At Least 5 threads\n");
                    break;
                case 'p':
                    _SERVER_PORT_ = atoi(argv[i + 1]);
                    break;
            }
}
void handler(int sig_number){
    flag = 1;
}
void sig_check(){
    if (flag == 1){
        printf("%sSIGINT has been received. I handled a total of %d requests. Goodbye.\n",getTimeStamp(),totalRequests);
        exit(1);
    }
}
void createThreads(int *arr, pthread_t *threads){
    for (int i = 0; i < numberOfThreads; i++){
        arr[i]=i;
        void *p = &arr[i];
        if (pthread_create(&threads[i], NULL, server, p) != 0)
            perror_call("pthread_create");
    }
}
void joinThreads(pthread_t *threads){
    for (int i = 0; i < numberOfThreads; i++)
        if (pthread_join(threads[i], NULL) != 0)
            perror_call("pthread_join");
}
void *server(void *input){
    while (1){
        char *buffer = (char *)malloc(sizeof(char) * 1024);
        char *head=buffer;
        pthread_mutex_lock(&mutex);
        while (1 > queue->size){
            if (sig_check_thread() == 1)
                break;
            pthread_cond_wait(&cond, &mutex);
            if (sig_check_thread() == 1)
                break;
        }
        int socket_fd=dequeue(queue);
        pthread_mutex_unlock(&mutex);
        if (sig_check_thread() == 1){
            free(head);
            break;
        }
        memset(buffer, '\0', 1024);

        read(socket_fd, buffer, 1024);
        if (sig_check_thread() == 1){
            free(head);
            break;
        }
        if (buffer[0] == 'c'){
            buffer += 2;
            clientRequest(socket_fd,buffer);
        }else if (buffer[0] == 'n'){
            buffer += 2;
            addServant(buffer);
        }
        close(socket_fd);
        free(head);
        if (sig_check_thread() == 1)
            break;
    }
    return NULL;
}
void clientRequest(int socket_fd,char * buffer){
    printf("%sRequest arrived “%s”\n", getTimeStamp(), buffer);
    totalRequests++;
    int res = sendToServant(buffer);
    char arr[20];
    sprintf(arr, "%d", res);
    printf("%sResponse received: %d, forwarded to client\n", getTimeStamp(), res);
    sendResToClient(socket_fd, arr);
}
int sendReqToServant (char* servant_ip,int servant_port,char* req){
    int sd = -1;
    struct sockaddr_in serv_addr;

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        perror_call("socket failed");

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(servant_port);

    if (inet_pton(AF_INET, servant_ip, &serv_addr.sin_addr) <= 0)
        perror_call("Invalid address/ Address not supported \n");

    if ((connect(sd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) < 0)
        perror_call("Server: Connection Failed\n");

    send(sd, req, strlen(req), 0); // req sent
    char buffer[100] = {0};
    memset(buffer, '\0', 100);

    read(sd, buffer, 100);
    close(sd);

    return atoi(&buffer[2]);
}
int sendReqToServants(char *req){   
    int res=0;
    for (int i = 0; i <servantCount; i++)
        res+=sendReqToServant("127.0.0.1",servants[i].port,req);
    return res;
}
int sig_check_thread(){
    if (flag == 1)
        return 1;
    return 0;
}
struct sockaddr_in *createSocket(struct sockaddr_in *address,int *server_fd){
    if (((*server_fd) = socket(AF_INET, SOCK_STREAM, 0)) == 0)
        perror_call("socket failed");

    address->sin_family = AF_INET;
    address->sin_addr.s_addr = INADDR_ANY;
    address->sin_port = htons(_SERVER_PORT_);

    if (bind((*server_fd), (struct sockaddr *)address,sizeof(*address)) < 0)
        perror_call("bind failed");

    if (listen(*server_fd, 256) < 0)
        perror_call("listen");

    return address;
}
int sendToServant(char* req){
    int index=0;
    char* city =getCity(req,&index);
    if (index==4){
        for (int i = 0; i < servantCount; i++)
            if (strcmp(city, servants[i].start) >= 0 && strcmp(city, servants[i].end) <=0){
                printf("%sContacting Servant %d\n",getTimeStamp(), servants[i].id);
                return sendReqToServant("127.0.0.1", servants[i].port,req);
            }
        return -1;
    }
    printf("%sContacting All Servants\n",getTimeStamp());
    return sendReqToServants(req);
}
void addServant(char *buffer){
    servants = realloc(servants, sizeof(struct Servant) * (servantCount + 1));
    char *fd = strtok_r(buffer, " ", &buffer);
    char *id = strtok_r(buffer, " ", &buffer);
    char *start = strtok_r(buffer, " ", &buffer);
    char *end = strtok_r(buffer, " ", &buffer);
    char *port = strtok_r(buffer, " ", &buffer);
    memset(servants[servantCount].start, '\0', 20);
    memset(servants[servantCount].end, '\0', 20);
    strncpy(servants[servantCount].start, start, strlen(start));
    strncpy(servants[servantCount].end, end, strlen(end));
    servants[servantCount].id = atoi(id);
    servants[servantCount].port = atoi(port);
    servants[servantCount].fd = atoi(fd);
    printf("%sServant %d, present at port %d, handling cities %s - %s\n", getTimeStamp(), servants[servantCount].id,
           servants[servantCount].port, servants[servantCount].start, servants[servantCount].end);
    servantCount++;
}
void sendResToClient(int sd,char *req){
    send(sd,req,strlen(req), 0);
}
char *getCity(char *req, int *index){
    int size = strlen(req);
    for (int i = 0; i < size; i++){
        if (req[0] == ' ')
            (*index)++;
        if ((*index) == 4)
            return ++req;
        (req)++;
    }
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