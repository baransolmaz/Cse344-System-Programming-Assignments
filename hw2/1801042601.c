#include <signal.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/wait.h>

int checkArgc(int argc, char *argv[]);
int start_Operations(char *inputFile, char *outputFile,int** pids, int *pid_size);
void wait_child(int childNumber);
double frobenius(float matrix[9]);
void read_calculate(char *filename,double* frobs);
void find_MinDistance(double* frobs,int childNum);
void printProcess(int process_number, char *arr[11]);
void handler(int sig_number);
void sig_check(char *filename, int *pids, int childNumber,double *frobs);

sig_atomic_t flag = 0;

int main(int argc, char *argv[]){
    struct sigaction act;
    memset(&act, '\0', sizeof(act));
    act.sa_handler = &handler;
    sigaction(SIGINT, &act, NULL);
    printf("\n\nParent ID: ---%d---\n\n",getpid());
    int pid_size = 0,childNum=0;
    int i_index = checkArgc(argc, argv);
    int *pids = (int *)malloc(sizeof(int) * (pid_size + 1));
    double *frobs=(double *)malloc(sizeof(double)*childNum+1);

    sig_check(argv[6 - i_index], pids, pid_size,frobs);
    childNum=start_Operations(argv[i_index], argv[6 - i_index],&pids,&pid_size);

    sig_check(argv[6 - i_index], pids, pid_size,frobs);
    frobs = (double *)realloc(frobs,sizeof(double) * childNum);
    memset(frobs, 0, childNum);
    sig_check(argv[6 - i_index], pids, pid_size, frobs);
    read_calculate(argv[6 - i_index],frobs);
    sig_check(argv[6 - i_index], pids, pid_size, frobs);
    find_MinDistance(frobs,childNum);
    sig_check(argv[6 - i_index], pids, pid_size, frobs);

    free(pids);
    free(frobs);
    return 0;
}
int checkArgc(int argc, char *argv[]){
    if (argc != 5){
        perror("Usage : ./processP [-i] inputFilePath [-o] outputFilePath\n");
    }else
        return (argv[1][1]=='i') ? 2 : 4;
        
    exit(0);
}
int start_Operations(char* inputFile,char* outputFile,int** pids,int* pid_size){
    int fd = open(inputFile, O_RDONLY);
    if (fd == -1){
        perror("Failed to Open File");
        exit(0);
    }
    char *argVec[5];
    argVec[4]=NULL;
    argVec[3]=(char*)malloc(sizeof(char)*10);
    argVec[2]=outputFile;
    argVec[1]="-o";
    argVec[0]="processR_";
    unsigned char buff[3];
    memset(buff,0,3);
    int process_number=0, i = 0,id;
    char *arr[11];
    for (int i = 0; i < 10; i++){
        arr[i] = (char*)malloc(sizeof(char)*4);        
        memset(arr[i],0,4);
    }
    arr[10]=NULL;
    while (read(fd, buff, 3) == 3){
        arr[i][0] = buff[0];
        arr[i][1] = buff[1];
        arr[i][2] = buff[2];
        i++;
        if (i==10){
            printProcess(process_number,arr);
            if ((id =fork())==0){
                sprintf(argVec[3],"%d#", process_number);
                execve(argVec[0],argVec,arr);
            }
            (*pids)[*pid_size]=id;
            (*pid_size)++;
            *pids = (int *)realloc(*pids, sizeof(int) * (*pid_size + 1));////???????
            memset(argVec[3],'0',10);
            process_number++;
            i=0;
        }
    }
    printf("Reached EOF, collecting outputs from %s\n",outputFile);
    wait_child(process_number);
    for (int i = 0; i < 10; i++)
        free(arr[i]);
    if(argVec[3]!=NULL)
        free(argVec[3]);
    int err2 = close(fd);
    if (err2 == -1)
        perror("Failed to Close File");
    return process_number;
}
void wait_child(int childNumber){
    pid_t childPid;
    for (int i = 0; i < childNumber; i++){
        int childPid = wait(NULL);
        if (childPid == -1)
            if (errno == ECHILD){
                printf("No more children - bye!\n");
                break;
            }else
                printf("wait");
    }
}
double frobenius(float matrix[9]){
    double sum=0;
    for (int i = 0; i < 9; i++)
        sum+= matrix[i]*matrix[i];
    return sqrt(sum);
}
void read_calculate(char* filename,double* frobs){
    int temp_index=0,frob_index, matrix_index=0;
    char buff[1], temp[10];
    int fd=open(filename,O_RDONLY);
    if (fd == -1){
        perror("Failed to Open File");
        exit(0);
    }
    float matrix[9];
    memset(temp,'0',10);
    while (read(fd,buff,1)){
        if (buff[0] == '#'){
            char ind[temp_index+1];
            memset(ind,0,temp_index+1);
            strncpy(ind,temp,temp_index);
            frob_index=atoi(ind);
            temp_index = 0;
            memset(temp, '0', 10);
        }else if (buff[0]==' '){
            temp_index=0;
            matrix[matrix_index] = atof(temp);
            matrix_index++;
            memset(temp, '0', 10);
        }else if (buff[0] == '\n'){
            matrix[matrix_index]=atof(temp);
            frobs[frob_index]=frobenius(matrix);
            matrix_index=0,
            memset(temp, '0', 10);
            memset(matrix, 0, 9);
        }else{
            temp[temp_index]=buff[0];
            temp_index++;
        }
    }
    int err2 = close(fd);
    if (err2 == -1)
        perror("Failed to Close File");
}
void find_MinDistance(double* frobs,int num){
    int process[2];
    process[0]=0;
    process[1] = 1;
    float diff;
    if(num <=1){
        printf("Not Enough Process to Calculate Distance\n"); 
    }else{
        diff=fabsf(frobs[0]-frobs[1]);
        for (int i = 0; i <num-1; i++)
            for (int j = i+1; j < num; j++)
                if (fabsf(frobs[i] - frobs[j])<diff){
                    diff = fabsf(frobs[i] - frobs[j]);
                    process[0]=i;
                    process[1] = j;
                }

        printf("The closest 2 matrices are %d and %d, and their distance is %.3f.\n", process[0]+1, process[1]+1,diff);
    }
}
void printProcess(int process_number, char *arr[11]){
    printf("Created R_%d with ",process_number+1);
    for (int i = 0; i < 10; i++){
        printf("(");
        for (int j = 0; j < 3; j++){
            printf("%d",arr[i][j]);
            if (j!=2)
                printf(",");
        }
        printf(")");
        if (i != 9)
            printf(",");
    }
    printf("\n");
}
void handler(int sig_number){
    flag = 1;
}
void sig_check(char *filename,int* pids,int childNumber,double* frobs){
    if (flag == 1){
        for (int i = 0; i < childNumber; i++)
            kill(pids[i],SIGINT);
        wait_child(childNumber);
        int fd = open(filename, O_TRUNC);
        close(fd);
        unlink(filename);
        free(frobs);
        free(pids);
        exit(1);
    }
}
