#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

#define ROW 10
#define COL 3
extern char **environ;
sig_atomic_t flag = 0;

void matrix_product(float first[][ROW], float second[][COL], float result[][COL], int r1, int c1, int r2, int c2);
void matrix_transpose(float first[ROW][COL], float result[COL][ROW], int r1, int c1);
void matrix_extraction(float first[ROW][COL], float second[ROW][COL], float result[ROW][COL], int r1, int c1);
void matrix_divide_n(float first[][COL], float result[][COL], int r1, int c1, int n);
void matrix_covariance(float first[ROW][COL], float result[COL][COL], int r1, int c1);
void display(float arr[][COL], int r, int c);
void matrix_initialize(char **mat, float arr[ROW][COL]);
void fileOP(float result[COL][COL],char *fileName,char *number);
void handler(int sig_number);
void sig_check();
int lockFile(char *filepath);
void unlockFile(int fd);

void main(int argc, char *argv[]){
    struct sigaction act;
    memset(&act, '\0', sizeof(act));
    act.sa_handler=&handler;
    sigaction(SIGINT,&act,NULL);

    sig_check();
    float main_arr[ROW][COL], result[COL][COL];
    matrix_initialize(environ,main_arr);
    sig_check();
    for (int i = 0; i < COL; i++)
        for (int j = 0; j < COL; j++)
            result[i][j]=0;
    sig_check();
    matrix_covariance(main_arr, result, ROW, COL);
    //sleep(10);
    sig_check();
    //display(result,COL,COL);
    fileOP(result,argv[2],argv[3]);
    sig_check();
    exit(0);
}
void matrix_product(float first[][ROW], float second[][COL], float result[][COL], int r1, int c1, int r2, int c2){
    for (int i = 0; i < r1; i++)
        for (int j = 0; j < c2; j++)
            for (int k = 0; k < c1; k++)
                result[i][j] += first[i][k] * second[k][j];
}
void matrix_transpose(float first[ROW][COL], float result[COL][ROW], int r1, int c1){
    for (int i = 0; i < r1; i++)
        for (int j = 0; j < c1; j++)
            result[j][i] = first[i][j];
}
void matrix_extraction(float first[ROW][COL], float second[ROW][COL], float result[ROW][COL], int r1, int c1){
    for (int i = 0; i < r1; i++)
        for (int j = 0; j < c1; j++)
            result[i][j] = first[i][j] - second[i][j];
}
void matrix_divide_n(float first[][COL], float result[][COL], int r1, int c1, int n){
    for (int i = 0; i < r1; i++)
        for (int j = 0; j < c1; j++)
            result[i][j] = first[i][j] / n;
}
void matrix_covariance(float first[ROW][COL], float result[COL][COL],int r1, int c1){
    float ones[r1][r1], result_pro[r1][c1],result_trans[c1][r1], result_div[r1][c1];
    for (int i = 0; i < r1; i++){
        for (int j = 0; j < r1; j++)
            ones[i][j]=1;
        for (int j = 0; j < c1; j++){
            result_pro[i][j]=0;
            result_trans[j][i] = 0;
            result_div[i][j]= 0;
        }
    }
    matrix_product(ones, first, result_pro, r1, r1, r1, c1);
    matrix_divide_n(result_pro,result_div,r1,c1,r1);
    matrix_extraction(first,result_div,result_pro, r1, c1);
    matrix_transpose(result_pro,result_trans,r1,c1);
    matrix_product(result_trans, result_pro, result, c1, r1, r1, c1);
    matrix_divide_n(result, result, c1, c1, r1);
}
void display(float arr[][COL],int r,int c){
    for (int i = 0; i < r; ++i)
        for (int j = 0; j < c; ++j){
            printf("%.2f  ", arr[i][j]);
            if (j == c - 1)
                printf("\n");
        }
}
void matrix_initialize(char **mat,float arr[ROW][COL]){
    for (int i = 0; i < ROW; i++)
        for (int j = 0; j < COL; j++)
            arr[i][j]=(float)mat[i][j];
}
void fileOP(float result[COL][COL], char *fileName,char *number){
    int fd=lockFile(fileName);
    char buf[500];
    int index=0;
    memset(buf,0,500);
    index += sprintf(buf+index, "%s",number);
    for (int i = 0; i < COL; i++)
        for (int j = 0; j < COL; j++)
            index+=sprintf(buf+index,"%.3f ",result[i][j]);
    buf[strlen(buf)-1]='\n';
    write(fd,buf,strlen(buf));
    unlockFile(fd);
    close(fd);
}
void handler(int sig_number){
    flag=1;
}
void sig_check(){
    if (flag==1) 
        exit(1);
}
int lockFile(char *filepath){
    struct flock lock;
    int fd = open(filepath, O_RDWR | O_APPEND | O_CREAT);
    if (fd == -1){
        perror("Failed to Open File");
        exit(0);
    }
    lock.l_type = F_WRLCK;
    lock.l_whence = 0;
    lock.l_start = 0;
    lock.l_len = 0;
    int res = fcntl(fd, F_SETLKW, &lock);
    if (res == -1)
        perror("Failed to Lock File");

    return fd;
}
void unlockFile(int fd){
    struct flock lock;
    lock.l_type = F_UNLCK;
    lock.l_whence = 0;
    lock.l_start = 0;
    lock.l_len = 0;
    int err = fcntl(fd, F_SETLKW, &lock);
    if (err == -1)
        perror("Failed to Unlock File");
    int err2 = close(fd);
    if (err2 == -1)
        perror("Failed to Close File");
}
