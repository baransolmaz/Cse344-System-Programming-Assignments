#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define SHARED_MEMORY_NAME "/shared"

char* ingrediants(char c);
int memory_read( int index, char needs[2]);
int openFile(char *dataFile, int type);
int wait_child(int childNumber);
void checkArgc(int argc, char *argv[]);
void chef(int index, char needs[2]);
void closeFile(int fd);
void handler(int sig_number);
void memory_create();
void memory_unlink();
void memory_write(char in[2]);
void pusher1();
void sig_check();
void startOperations();
void wholeSaler();
sig_atomic_t flag = 0;
sig_atomic_t unlink_flag = -1;

char *inputFile;
struct Chef{
    int id;
    char needs[2];
};
struct Shared{
    char ingred[2];
    sem_t saler;
    sem_t sent;
    sem_t needs_sems[6];
} shared;

struct Shared *shr_ptr;
struct Chef chefs[6];

void* addr;
int totalDesserts=0;
int main(int argc, char *argv[]){
    struct sigaction act;
    memset(&act, '\0', sizeof(act));
    act.sa_handler = &handler;
    sigaction(SIGINT, &act, NULL);

    checkArgc(argc,argv);
    memory_create();
    shr_ptr = (struct Shared *)addr;
    startOperations(); 
    memory_unlink();

    return 0;
}
void checkArgc(int argc, char *argv[]){
    if (argc != 3){
        perror("Usage : ./hw3unnamed -i inputFilePath\n");
    }else{
        for (int i = 1; i < argc; i += 2){
            switch (argv[i][1]){
                case 'i':
                    inputFile = argv[i + 1];
                    break;
                default:
                    break;
            }
        }
        return;
    }
    exit(1);
}
void handler(int sig_number){
    if (sig_number == SIGINT){
        flag = 1;
    }
}
void sig_check(){
    if (flag == 1){
        for (int i = 0; i < 6; i++){
            if (chefs[i].id != -1)
                kill(chefs[i].id, SIGINT);
        }
        totalDesserts = wait_child(6);
        printf("The Wholesaler (pid %d) is done. (Total Desserts : %d)\n", (int)getpid(), totalDesserts);
        memory_unlink();
        exit(1);
    }    
}
int sig_check2(){
    if (flag == 1){
       return 1;
    }
    return 0;
}
void wholeSaler(){
    sleep(1);
    int fd = openFile(inputFile,O_RDONLY);
    while (1){
        char buffer[3];
        memset(buffer,'\0',3);
        sig_check();
        if (read(fd, buffer, 3) != 3){
            for (int i = 0; i < 6; i++){
                if (chefs[i].id!= -1)
                    kill(chefs[i].id,SIGINT);
            }
            totalDesserts=wait_child(6);
            printf("The Wholesaler (pid %d) is done. (Total Desserts : %d)\n",(int)getpid(),totalDesserts);
            break;
        }
        sig_check();
        memory_write(buffer);
    }
    closeFile(fd);
}
void closeFile(int fd){
    int err2 = close(fd);
    if (err2 == -1)
        perror("Failed to Close File");
}
int openFile(char *dataFile, int type){
    int fd = open(dataFile, type);
    if (fd == -1){
        printf("Failed to Open File: %s", dataFile);
        exit(1);
    }
    return fd;
}
void chef(int chefIndex, char needs[2]){
    int counter=0;
    printf("Chef%d (pid %d) is waiting for %s and %s\n", chefIndex,
        (int)getpid(), ingrediants(needs[0]), ingrediants(needs[1]));
    sleep(1);
    while (1){
        if (sig_check2())
            break;
        int i=memory_read(chefIndex,needs);

        if (i != 0){
            counter++;
            printf("Chef%d (pid %d) has delivered the dessert\t Ingredients: %c%c\n",
                chefIndex, (int)getpid(), shr_ptr->ingred[0], shr_ptr->ingred[1]);
        }
        if (sig_check2())
            break;
        sem_post(&(shr_ptr->saler));
        if (sig_check2())
            break;
    }
    printf("Chef%d (pid %d) is exiting\t Prepared Desserts : %d\n", chefIndex, (int)getpid(),counter);
    //sleep(1);
    _exit(counter);
}
int wait_child(int childNumber){
    pid_t childPid;
    int num=0;
    for (int i = 0; i < childNumber; i++){
        int status=0;
        childPid = waitpid(chefs[i].id,&status,0);
        num+= WEXITSTATUS(status);
        if (childPid == -1)
            if (errno == ECHILD){
                break;
            }
    }
    return num;
}
void startOperations(){
    char needs[6][2] = {"WS","FW","SF","MF","MW","SM"}; 
    for (int i = 0; i < 6; i++){
        int child = fork();
        if (child == 0){ // Child
            chef(i, needs[i]);
            exit(EXIT_SUCCESS);
        }else if (child == -1){
            perror("Start Operations : Couldn't Create Chef");
        }else{//parent
            chefs[i].id=child;
            strncpy(chefs[i].needs,needs[i],2);
        }
    }
    wholeSaler();
}
void memory_write(char ing[3]){
    strncpy(shr_ptr->ingred, ing, 2);
    sem_wait(&(shr_ptr->saler));
    printf("The Wholesaler (pid %d) delivers %s and %s\n",(int)getpid(),ingrediants(ing[0]),ingrediants(ing[1]));
    pusher1();
    printf("The Wholesaler (pid %d) is waiting for the dessert\n", (int)getpid());
    sem_wait(&(shr_ptr->sent));
    printf("The Wholesaler(pid %d) has obtained the dessert and left\n", (int)getpid());
}
void memory_create(){
    int shr_mem_fd = shm_open(SHARED_MEMORY_NAME, O_CREAT | O_RDWR, 0777);
    if (shr_mem_fd == -1)
        perror("Memory Create : shm_open");
    unlink_flag = 0;
    int size = sizeof(struct Shared);
    if (ftruncate(shr_mem_fd, size) == -1)
        perror("Memory Create : ftruncate");

    struct Shared temp;
    for (int i = 0; i <6; i++)
        sem_init(&(temp.needs_sems[i]), 1,0);
    
    sem_init(&(temp.saler), 1, 1);
    sem_init(&(temp.sent), 1, 0);
    memset(temp.ingred,'\0',2);

    addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shr_mem_fd, 0);
    if (addr == MAP_FAILED)
        perror("Memory Create : mmap");
    memcpy(addr, &temp, size);
    if (close(shr_mem_fd) == -1)
        perror("Memory Create: close");
}
int memory_read(int index,char needs[2]){
    sem_wait(&(shr_ptr->needs_sems[index]));
    if (sig_check2())
        return 0;
    printf("Chef%d (pid %d) has taken the %s\t Ingredients: %c%c\n",
        index, (int)getpid(), ingrediants(shr_ptr->ingred[0]), shr_ptr->ingred[0], shr_ptr->ingred[1]);
    printf("Chef%d (pid %d) has taken the %s\t Ingredients: %c%c\n",
        index, (int)getpid(), ingrediants(shr_ptr->ingred[1]), shr_ptr->ingred[0], shr_ptr->ingred[1]);
    memset(shr_ptr->ingred, '\0', 2);
    printf("Chef%d (pid %d) is preparing the dessert\t Ingredients: %c%c\n",
        index, (int)getpid(), shr_ptr->ingred[0], shr_ptr->ingred[1]);
    if (sig_check2())
        return 0;
    sem_post(&(shr_ptr->sent));
    if (sig_check2())
        return 0;
    return 1;
}
void memory_unlink(){
    if (unlink_flag == 0){
        if (shm_unlink(SHARED_MEMORY_NAME) == -1){
            // writeLogErr("Memory Unlink: shm_unlink");
        }
        unlink_flag = 1;
    }
}
void pusher1(){
    if (('W' == shr_ptr->ingred[0] && 'S' == shr_ptr->ingred[1]) || ('W' == shr_ptr->ingred[1] && 'S' == shr_ptr->ingred[0])){
        sem_post(&(shr_ptr->needs_sems[0]));
    }else if (('F'== shr_ptr->ingred[0] && 'W' == shr_ptr->ingred[1]) || ('F'== shr_ptr->ingred[1] && 'W' == shr_ptr->ingred[0])){
        sem_post(&(shr_ptr->needs_sems[1]));
    }else if (('F' == shr_ptr->ingred[0] && 'S' == shr_ptr->ingred[1]) || ('F' == shr_ptr->ingred[1] && 'S' == shr_ptr->ingred[0])){
        sem_post(&(shr_ptr->needs_sems[2]));
    }else if (('F' == shr_ptr->ingred[0] && 'M' == shr_ptr->ingred[1]) || ('F' == shr_ptr->ingred[1] && 'M' == shr_ptr->ingred[0])){
        sem_post(&(shr_ptr->needs_sems[3]));
    }else if (('M' == shr_ptr->ingred[0] && 'W' == shr_ptr->ingred[1]) || ('M' == shr_ptr->ingred[1] && 'W' == shr_ptr->ingred[0])){
        sem_post(&(shr_ptr->needs_sems[4]));
    }else
        sem_post(&(shr_ptr->needs_sems[5])); 
}
char* ingrediants(char c){
    switch (c){
        case 'M':
            return "MILK";
        case 'F':
            return "FLOUR";
        case 'W':
            return "WALNUTS";
        case 'S':
            return "SUGAR";
        default:
            return NULL;
    }
}