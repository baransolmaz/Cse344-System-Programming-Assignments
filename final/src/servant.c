#include "../libs/avl_city.h"
#include "../libs/helper.h"
#include <arpa/inet.h>
#include <dirent.h>

sig_atomic_t flag = 0;

char *_IP_;
char *directoryPath;
char endDir_name[30];
char startDir_name[30];
int _SERVER_PORT_;
int endDir_index = -1;
int procID=-1;
int requestCount = 0;
int servantPort=16000;
int startDir_index = -1;

int findEmptyPort(int *servant_fd, int port);//to find empty port
int sig_check_thread();//thread sigint check
struct Request *parseRequest(char *req);//To parse incoming line
struct TransactionNode *readFiles(char *dir, char *fileName);//To read file in directory
void *servant(void *in);//servant process
void checkArgc(int argc, char *argv[]);//arg check
void freeReq(struct Request *req);//to free requests
void getFolders();//to get folders in directory
void handler(int sig_number);
void notifyServer(int servant_fd,char *start, char *end, int port_num);//To notify server
void readFolders(char folderNames[][30]);//to read folders with files
void sig_check();
void socketOperations();//Socket op. for servant
void sortFolders(char folderNames[][30], int size);//To sort folder name

struct CityNode *root = NULL;
struct Request *requests=NULL;

int main(int argc, char *argv[]){
    struct sigaction act;
    memset(&act, '\0', sizeof(act));
    act.sa_handler = &handler;
    sigaction(SIGINT, &act, NULL);
    
    checkArgc(argc, argv);
    sig_check();
    procID = getProcID("/proc/self/stat");//getPid()
    getFolders();//Read Folders

    printf("Servant %d: loaded %s, cities %s-%s\n", procID, directoryPath,startDir_name,endDir_name);

    int servant_fd = -1,err=-1;
    sig_check();
    for (int i = 0; i <2000; i++){//Find empty port
        err= findEmptyPort(&servant_fd,servantPort);
        if(sig_check_thread()==1)
            break;
        if (err>=0)
            break;
        servantPort++;
    }
    printf("Servant %d: listening at port %d\n",procID,servantPort);
    sig_check();
    notifyServer(servant_fd,startDir_name, endDir_name, servantPort);//notify server
    sig_check();
    socketOperations(servant_fd);//do socket Operations
    freeNodeCity(root);
    close(servant_fd);
    printf("Servant %d: termination message received, handled %d requests in total.\n",procID,requestCount);
    return 0;
}
void checkArgc(int argc, char *argv[]){
    if (argc != 9){
        perror_call("Usage : ./servant -d directoryPath -c 10-19 -r IP -p PORT\n");
    }else{
        for (int i = 1; i < argc; i += 2)
            switch (argv[i][1])
            {
            case 'd':
                directoryPath = argv[i + 1];
                break;
            case 'c':
                startDir_index = atoi(strtok_r(argv[i + 1], "-", &argv[i+1]));
                endDir_index = atoi(argv[i + 1]);
                break;
            case 'r':
                _IP_ = argv[i + 1];
                break;
            case 'p':
                _SERVER_PORT_ = atoi(argv[i + 1]);
                break;
            }
    }
}
void handler(int sig_number){
    flag = 1;
}
void sig_check(){
    if (flag == 1){
        freeNodeCity(root);
        exit(1);
    }
}
int sig_check_thread(){
    if (flag == 1)
        return 1;
    return 0;
}
void socketOperations(int servant_fd){
    int new_socket=-1;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
      
    if (listen(servant_fd, 256) < 0)
        perror_call("listen");

    if (sig_check_thread() == 1)
        return;
    while (1){
        // accept socket
        if ((new_socket = accept(servant_fd, (struct sockaddr *)&address,(socklen_t *)&addrlen)) < 0)
            break;
        if (sig_check_thread() == 1){
            close(new_socket);
            break;
        }
        int *server_fd = (int *)malloc(sizeof(int));
        *server_fd=new_socket;
        pthread_t thread_id;
        if (sig_check_thread() == 1){
            free(server_fd);
            close(new_socket);
            break;
        }
        requestCount++;
        if (pthread_create(&thread_id, NULL, servant, server_fd) != 0)//create thread
            perror_call("pthread_create");
    }
    shutdown(servant_fd, SHUT_RDWR);
}
void getFolders(){
    struct dirent *de; // Pointer for directory entry
    DIR *dr = opendir(directoryPath);

    if (dr == NULL) // opendir returns NULL if couldn't open directory
        perror_call("Could not open current directory");
    
    char folderNames[512][30];
    int i=0;
    while ((de = readdir(dr)) != NULL){//read folders
        strcpy(folderNames[i], de->d_name);
        i++;
    }
    sortFolders(folderNames,i);//sort folder names
    for (int i = 0; i < 30; i++){
        startDir_name[i]=folderNames[startDir_index+1][i];
        endDir_name[i]=folderNames[endDir_index+1][i];
    }
    closedir(dr);
    readFolders(folderNames);//read Files
}
void sortFolders(char folderNames[][30],int size ){
    char temp[30];
    for (int i = 0; i < size; i++)
        for (int j = 0; j < size - 1 - i; j++)
            if (strcmp(folderNames[j], folderNames[j + 1]) > 0){
                strcpy(temp,folderNames[j]);
                strcpy(folderNames[j], folderNames[j + 1]);
                strcpy(folderNames[j+1],temp);
            }
}
void notifyServer(int servant_fd,char *start,char *end,int port_num){
    char hand[120];
    int sd=-1,server_fd=-1;
    struct sockaddr_in serv_addr;
    memset(hand,'\0',120);
    sprintf(hand, "n:%d %d %s %s %d", servant_fd,procID, start, end, port_num);
    sig_check();
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0)//open socket
        perror_call("socket failed");
    if (sig_check_thread() == 1)
        close(sd);
    sig_check();

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(_SERVER_PORT_);

    if (inet_pton(AF_INET, _IP_, &serv_addr.sin_addr) <= 0)
        perror_call("Invalid address/ Address not supported \n");

    if ((server_fd = connect(sd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) < 0)//conncet socket
        perror_call("Servant : Connection Failed\n");
    if (sig_check_thread() == 1){
        close(sd);
        close(server_fd);
    }
    sig_check();

    if(send(sd,hand, strlen(hand), 0)<0)//send to server
        perror_call("Send");

    close(server_fd);
    close(sd);
}
void readFolders(char folderNames[][30]){
    struct dirent *de; // Pointer for directory entry
    char path[60];
    for (int i = startDir_index+1; i < endDir_index+2; i++){
        memset(path,'\0',60);
        sprintf(path,"%s/%s",directoryPath,folderNames[i]);
       
        struct CityNode *city=newCityNode(folderNames[i]);

        DIR *dr = opendir(path);
        if (dr == NULL) // opendir returns NULL if couldn't open directory
            perror_call("Could not open current directory");

        while ((de = readdir(dr)) != NULL){//read files
            if (strlen(de->d_name)>2){
                struct DateNode *date = newDateNode(de->d_name);
                date->transactions= readFiles(path, de->d_name);
                city->dates = insertNodeDate(city->dates, date);
            }
        }
        closedir(dr);
        root = insertNodeCity(root,city); // Insert City
    }
}
struct TransactionNode *readFiles(char* dir,char* fileName){
    char path[60];
    memset(path, '\0', 60);
    sprintf(path, "%s/%s", dir, fileName);
    int fd=openFile(path,O_RDONLY);
    struct TransactionNode *node=NULL;
    char buf[2],line[60];
    memset(line, '\0', 60);
    memset(buf, '\0', 2);
    int index=0;
    while (read(fd, buf, 1) == 1){
        if (buf[0] == '\n'){
            if (index != 0){
                node=insertNodeTransaction(node,line);
                memset(line, '\0',60);
            }
            index = 0;
        }else{
            line[index] = buf[0];
            index++;
        }
    }
    closeFile(fd);
    return node;
}
struct Request *parseRequest(char* req){
    struct Request *node =NULL;
    node= (struct Request *)malloc(sizeof(struct Request));
    node->type = (char *)malloc(sizeof(char)*20);
    memset(node->type, '\0', 20);
    node->start = (char *)malloc(sizeof(char) * 11);
    memset(node->start, '\0',11);
    node->end = (char *)malloc(sizeof(char) * 11);
    memset(node->end, '\0', 11);
    node->city = (char *)malloc(sizeof(char) * 30);
    memset(node->city,'\0',30);
    strtok_r(req, " ", &req);//transaction
    char *type = strtok_r(req, " ", &req);
    for (int i = 0; i <strlen(type); i++)
        node->type[i] =type[i];

    char *start = strtok_r(req, " ", &req);
    char *end = strtok_r(req, " ", &req);
    for (int i = 0; i <10; i++){
        node->start[i] = start[i];
        node->end[i] = end[i];
    }
    if (req==NULL){
        node->checkCity=0;
    }else
        for (int i = 0; i <strlen(req); i++)
            node->city[i]=req[i];
    return node;
}
int findEmptyPort(int *servant_fd,int port){
    struct sockaddr_in address;
    int temp=-1;
    if ((temp = socket(AF_INET, SOCK_STREAM, 0)) == 0)//open socket
        perror_call("socket failed");

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(servantPort);
    if (bind(temp, (struct sockaddr *)&address, sizeof(address)) < 0){//try to bind
        close(temp);
        return -1;
    }

    *servant_fd= temp;
    return temp;
}
void *servant(void *input){
    pthread_detach(pthread_self());//make detached
    int server_fd = *((int *)input);
    char buffer[1024];
    memset(buffer,'\0',1024);
    int size=read(server_fd, buffer, 1024);//read from socket
    if (sig_check_thread()==1)
        return NULL;
    
    if (size<=0){
        send(server_fd, "0", 1, 0);
        close(server_fd);
        free(input);
        return NULL;
    }
    struct Request *req=parseRequest(buffer);//parse line to request
    int result=searchCity(root,req);//search request
    if (sig_check_thread() == 1){
        freeReq(req);
        return NULL;
    }
    char arr[10];
    memset(arr,'\0',10);
    sprintf(arr,"s:%d",result);
    send(server_fd, arr, strlen(arr), 0);//send response to server
    freeReq(req);
    close(server_fd);
    free(input);
    return NULL;
}
void freeReq(struct Request *req){
    free(req->city);
    free(req->end);
    free(req->start);
    free(req->type);
    free(req);
}