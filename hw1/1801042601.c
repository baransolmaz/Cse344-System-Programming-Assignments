#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h> 

struct Task
{
    char *target;
    char *edit;
    int i;
};

void checkArgc(int argc);
struct Task *splitTasks(char *arg, int *size);
void startOperations(char* filepath,struct Task* tasks,int size);
int lockFile(char *filepath);
int readFile(int fd, char *buffer);
char* changeBuffer(struct Task *tasks,int taskSize,char *buffer,int bufferSize);
int writeFile(int fd, char *buffer);
void unlockFile(int fd);


char *do_E_Type(struct Task task, char *buffer, int bufferSize);
char *do_E_Type_v1(struct Task task, char *word);
char *do_E_Type_v2(struct Task task, char *word, int start, int end);

char *do_F_Type(struct Task task, char *buffer, int bufferSize);
char *do_F_Type_v1(struct Task task, char *word);
char *do_F_Type_v2(struct Task task, char *word, int start, int end);

char *do_Other_Types(struct Task task, char *buffer, int bufferSize);
char *do_Other_Types_v1(struct Task task, char *word);
char *do_Other_Types_v2(struct Task task, char *word, int start, int end);

int check_D_Type(char *target, int *start, int *end);
char charlwr(char c); 

int main(int argc, char *argv[]){
    checkArgc(argc);
    char *args = argv[1];
    char *filepath = argv[2];
    int size=0;
    struct Task *tasks = splitTasks(args,&size);
    startOperations(filepath,tasks,size);
    free(tasks);   
    return 0;
}
void checkArgc(int argc){
    if(argc < 3){
        perror("Insufficient Input Arguments\n");
    }else if (argc > 3){
        perror("Too Many Input Arguments\n");
    }else
        return;
    exit(0);
}
struct Task* splitTasks(char *arg,int *size){
    char *arr;
    struct Task *task = (struct Task *)malloc(sizeof(struct Task));
    *size=0;
    while ((int)strlen(arg) != 0)
    {
        arr = strtok_r(arg, ";", &arg);
        task = (struct Task*)realloc(task, sizeof(struct Task)*(*size+1));

        task[*size].target = strtok_r(arr, "/", &arr);
        task[*size].edit = strtok_r(arr, "/", &arr);
        if (*arr == 'i')
            task[*size].i = 1;
        else
            task[*size].i = 0;

        /*  printf("Target: %s\n",task->target);
        printf("edit: %s\n", task->edit);
        printf("i: %d\n", task->i); */
        *size+=1;
    }
    return task;
}
void startOperations(char *filepath, struct Task *tasks, int taskSize){
    int fd= lockFile(filepath);
    char *buffer = (char *)malloc(sizeof(char) * 1025);
    memset(buffer, 0, 1025);
    int bufferSize = readFile(fd,buffer);
    ftruncate(fd,0);
    lseek(fd,0,SEEK_SET);
    char* oneLine=NULL,*newBuffer=NULL,*rest=buffer;
    while ((oneLine = strtok_r(rest, "\n", &rest))!=NULL){
        newBuffer = changeBuffer(tasks, taskSize, oneLine,strlen(oneLine));
        strcat(newBuffer,"\n");
        int newSize = writeFile(fd, newBuffer);
    }

    unlockFile(fd);
    free(buffer);
    free(newBuffer);
}
int lockFile(char *filepath){
    struct flock lock;
    int fd = open(filepath, O_RDWR);
    if (fd==-1){
        perror("Failed to Open File");
        exit(0);
    }
    lock.l_type = F_WRLCK;
    lock.l_whence = 0;
    lock.l_start = 0;
    lock.l_len = 0; 
    int res=fcntl(fd, F_SETLKW, &lock);
    if (res==-1)
        perror("Failed to Lock File");

    return fd;
}
void unlockFile(int fd){
    struct flock lock;
    lock.l_type = F_UNLCK;
    lock.l_whence = 0;
    lock.l_start = 0;
    lock.l_len = 0;
    int err=fcntl(fd, F_SETLKW, &lock);
    if (err == -1)
        perror("Failed to Unlock File");
    int err2=close(fd);
    if (err2==-1)
        perror("Failed to Close File");
}
int readFile(int fd,char* buffer){
    return read(fd, buffer, 1024);
}
int writeFile(int fd, char *buffer){
    return write(fd,buffer, strlen(buffer));
}

char* changeBuffer(struct Task *tasks, int taskSize,char* buffer,int bufferSize){
    char* newBuffer = NULL;
    for (int i = 0; i < taskSize; i++){
        if (tasks[i].target[0]=='^'){
            newBuffer = do_E_Type(tasks[i], buffer, bufferSize);
        }else if (tasks[i].target[strlen(tasks[i].target)-1] == '$'){
            newBuffer = do_F_Type(tasks[i], buffer, bufferSize);
        }else{
            newBuffer = do_Other_Types(tasks[i], buffer, bufferSize);
        }

        buffer=newBuffer;   
    }

    return newBuffer;
}
int check_D_Type(char *target, int *start, int *end){
    int exist = -1;
    for (int i = 0; i < strlen(target); i++){
        if (target[i] == '['){
            *start = i;
            exist = 1;
        }else if (target[i] == ']'){
            *end = i;
        }
    }
    return exist;
}
char* do_E_Type(struct Task task, char *buffer, int bufferSize){
    char *newBuffer = (char *)malloc(sizeof(char) * 1024);
    memset(newBuffer, 0, 1024);
    int start=-1,end=-1;
    int is_D=check_D_Type(task.target, &start, &end);

    char *word , *newWord;
    word = strtok_r(buffer," ", &buffer);        
    
    if (is_D == -1){
        newWord = do_E_Type_v1(task, word);
    }else
        newWord = do_E_Type_v2(task, word, start, end);
    strcat(newBuffer, newWord);
    strcat(newBuffer, " ");
    strcat(newBuffer, buffer);

    return newBuffer;
}
char *do_E_Type_v1(struct Task task, char *word){
    int wordIndex = 0;
    for (int i = 1; i < strlen(task.target); i++)
        if (((task.i == 0) && (task.target[i] == word[wordIndex])) || ((task.i == 1) && (charlwr(task.target[i]) == charlwr(word[wordIndex])))){
            wordIndex++;
            if (task.target[i + 1] == '*'){
                while (((task.i == 0) && (task.target[i] == word[wordIndex])) || ((task.i == 1) && (charlwr(task.target[i]) == charlwr(word[wordIndex]))))
                    wordIndex++;
                i++;
            }
        }else if (task.target[i + 1] == '*'){
            i++;
        }else
            return word;

    char *newWord = (char *)malloc(sizeof(char) * (strlen(task.edit) + (strlen(word) - wordIndex)));
    memset(newWord, 0, strlen(newWord));
    for (int i = 0; i < strlen(task.edit); i++)
        newWord[i] = task.edit[i];
    for (int i = 0; i < strlen(word) - wordIndex; i++)
        newWord[strlen(task.edit) + i] = word[i + wordIndex];
    return newWord;
}
char *do_E_Type_v2(struct Task task, char *word, int start, int end){
    int wordIndex = 0;
    for (int i = 1; i < strlen(task.target); i++){
        if (((task.i == 0) && (task.target[i] == word[wordIndex])) || ((task.i == 1) && (charlwr(task.target[i]) == charlwr(word[wordIndex])))){
            wordIndex++;
            if (task.target[i + 1] == '*'){
                while (((task.i == 0) && (task.target[i] == word[wordIndex])) || ((task.i == 1) && (charlwr(task.target[i]) == charlwr(word[wordIndex]))))
                    wordIndex++;
                i++;
            }
        }else if (i == start){
            if (task.target[end + 1] == '*'){
                for (int k = start + 1; k < end; k++){
                    if ((task.i == 0 && task.target[k] == word[wordIndex]) || (task.i == 1 && charlwr(task.target[k]) == charlwr(word[wordIndex]))){
                        wordIndex++;
                        k = start;
                    }
                    i = end + 1;
                }
            }else
                for (int k = start + 1; k < end; k++){
                    if ((task.i == 0 && task.target[k] == word[wordIndex]) || (task.i == 1 && charlwr(task.target[k]) == charlwr(word[wordIndex]))){
                        wordIndex++;
                        k = end;
                    }
                    i = end;
                }
        }else
            return word;
    }

    char *newWord = (char *)malloc(sizeof(char) * (strlen(task.edit) + (strlen(word) - wordIndex)));
    memset(newWord, 0, strlen(newWord));
    for (int i = 0; i < strlen(task.edit); i++)
        newWord[i] = task.edit[i];
    for (int i = 0; i < strlen(word) - wordIndex; i++)
        newWord[strlen(task.edit) + i] = word[i + wordIndex];
    return newWord;
}

char *do_F_Type(struct Task task, char *buffer, int bufferSize){
    char *newBuffer = (char *)malloc(sizeof(char) * 1024);
    memset(newBuffer, 0, 1024);
    int start = -1, end = -1;
    int is_D = check_D_Type(task.target, &start, &end);

    char *word="", *newWord;
    while (strlen(buffer) != 0){
        strcat(newBuffer, word);
        if (strlen(word) != 0)
            strcat(newBuffer, " ");

        word = strtok_r(buffer, " ", &buffer);
    }
    if (is_D == -1){
        newWord = do_F_Type_v1(task, word);
    }else
        newWord = do_F_Type_v2(task, word, start, end);
    
    strcat(newBuffer, newWord);
    strcat(newBuffer, " ");
    strcat(newBuffer, buffer);
    free(newWord);

    return newBuffer;  
}
char *do_F_Type_v1(struct Task task, char *word){
    int wordIndex = strlen(word)-1;
    for (int i = strlen(task.target) - 2; i >=0 ; i--){
        if (((task.i == 0) && (task.target[i] == word[wordIndex])) || ((task.i == 1) && (charlwr(task.target[i]) == charlwr(word[wordIndex])))){
            wordIndex--;
        }else if (task.target[i] == '*'){
            while (((task.i == 0) && (task.target[i-1] == word[wordIndex])) || ((task.i == 1) && (charlwr(task.target[i-1]) == charlwr(word[wordIndex]))))
                wordIndex--;
            i--;
        }else
            return word;
    }

    char *newWord = (char *)malloc(sizeof(char) * (strlen(task.edit) +wordIndex));
    memset(newWord, 0, strlen(newWord));
    for (int i = 0; i <wordIndex+1; i++)
        newWord[i] = word[i];
    for (int i = wordIndex+1; i < (strlen(task.edit) + wordIndex)+1; i++)
        newWord[i] = task.edit[i-wordIndex-1];
    return newWord;
}
char *do_F_Type_v2(struct Task task, char *word, int start, int end){
    int wordIndex = strlen(word) - 1;
    char *last;
    if (!((word[wordIndex] >= 65 && word[wordIndex] <= 90) || (word[wordIndex] >= 97 && word[wordIndex] <= 122))){
        last=&word[wordIndex];
        wordIndex--;
    }
        
    for (int i = strlen(task.target) - 2; i >= 0; i--){
        if (((task.i == 0) && (task.target[i] == word[wordIndex])) || ((task.i == 1) && (charlwr(task.target[i]) == charlwr(word[wordIndex])))){
            wordIndex--;
        }else if (task.target[i] == '*'){
            if (i==end+1){
                for (int k = end - 1; k > start; k--){
                    if (((task.i == 0) && (task.target[k] == word[wordIndex])) || ((task.i == 1) && (charlwr(task.target[k]) == charlwr(word[wordIndex])))){
                        wordIndex--;
                        k = end;
                    }
                    i = start;
                }
            }else{
                while (((task.i == 0) && (task.target[i-1] == word[wordIndex])) || ((task.i == 1) && (charlwr(task.target[i-1]) == charlwr(word[wordIndex]))))
                    wordIndex--;
                i--;
            }
        }else if (i == end){
            for (int k = end - 1; k > start; k--){
                if (((task.i == 0) && (task.target[k] == word[wordIndex])) || ((task.i == 1) && (charlwr(task.target[k]) == charlwr(word[wordIndex])))){
                    wordIndex--;
                    k = start;
                }
                i = start;
            }
        }
        else
            return word;
    }

    char *newWord = (char *)malloc(sizeof(char) * (strlen(task.edit) + wordIndex));
    memset(newWord, 0, strlen(newWord));
    for (int i = 0; i < wordIndex + 1; i++)
        newWord[i] = word[i];
    for (int i = wordIndex + 1; i < (strlen(task.edit) + wordIndex) + 1; i++)
        newWord[i] = task.edit[i - wordIndex - 1];
    strcat(newWord,last);
    return newWord;
}

char charlwr(char c){
    if (65 <=c && c<=90){
        return c+32;
    }else
        return c;
}

char *do_Other_Types(struct Task task, char *buffer, int bufferSize){
    char *newBuffer = (char *)malloc(sizeof(char) * bufferSize);
    memset(newBuffer, 0, strlen(newBuffer));
    int start = -1, end = -1;
    int is_D = check_D_Type(task.target, &start, &end);

    char *word, *newWord;
    while ((word = strtok_r(buffer, " ", &buffer)) != NULL){
        if (is_D == -1){
            newWord = do_Other_Types_v1(task, word);
        }else
            newWord = do_Other_Types_v2(task, word, start, end);

        strcat(newBuffer, newWord);
        strcat(newBuffer, " ");
        free(newWord);
    }
    free(word);
    return newBuffer;
}
char *do_Other_Types_v1(struct Task task, char *word){
    int start=0;
    int targetIndex=0,newIndex=0,flag=0;
    char *newWord = (char *)malloc(sizeof(char) * (strlen(task.edit) * strlen(word)));
    memset(newWord, 0, (strlen(task.edit) * strlen(word)));
    for (int i = 0; i < strlen(word); i++){
        if ((targetIndex+1<strlen(task.target))&& (task.target[targetIndex + 1] == '*')){
            if (targetIndex==0)
                start=i;
            while (((task.i == 0) && (word[i] == task.target[targetIndex])) || ((task.i == 1) && (charlwr(word[i]) == charlwr(task.target[targetIndex]))))
                i++;
            i--;
            targetIndex+=2;
        }else if (targetIndex < strlen(task.target) && ((task.i == 0) && (word[i] == task.target[targetIndex])) || ((task.i == 1) && (charlwr(word[i]) == charlwr(task.target[targetIndex])))){
            if (targetIndex == 0)
                start = i;
            targetIndex++;
        }else if (targetIndex == strlen(task.target)){
            strcat(newWord, task.edit);
            targetIndex=0;
            newIndex += strlen(task.edit);
            start=i;
            i--;
        }else{
            strncpy(&newWord[newIndex], &word[start],(i+1)-start);
            newIndex += (i+1) - start;
            start = i+1;
            targetIndex=0;
        }
        if (i == strlen(word) - 1){
            if (targetIndex == strlen(task.target)){
                strcat(newWord, task.edit);
                newIndex += strlen(task.edit);
            }else{
                strncpy(&newWord[newIndex], &word[start], (i + 1) - start);
                newIndex += (i + 1) - start;
            }
        }
    }

    return newWord;
}
char *do_Other_Types_v2(struct Task task, char *word, int start, int end){
    char *newWord = (char *)malloc(sizeof(char) * (strlen(task.edit) * strlen(word)));
    memset(newWord, 0, (strlen(task.edit) * strlen(word)));
    int targetIndex=0,newIndex=0,start_cpy=0;
    int wordIndex=0;

    for (int i = 0; i < strlen(word); i++){
        if(targetIndex==start){
            if (targetIndex == 0 && targetIndex <= start)
                start_cpy=i;

            int flag=0,temp=i;
            if(task.target[end+1]=='*'){
                for (int k = start+1; k < end; k++)
                    if (((task.i == 0) && (word[i] == task.target[k])) || ((task.i == 1) && (charlwr(word[i]) == charlwr(task.target[k])))){
                        k = start;
                        i++;
                    }
                flag = 1;
                targetIndex = end + 2;
                i--;
            }else
                for (int k = start + 1; k < end; k++)
                    if (((task.i == 0) && (word[i] == task.target[k])) || ((task.i == 1) && (charlwr(word[i]) == charlwr(task.target[k])))){
                        targetIndex=end+1;
                        k=end;
                        flag = 1;
                    }
            if (flag==0){
                strncpy(&newWord[newIndex], &word[start_cpy], (i + 1) - start_cpy);
                newIndex += (i + 1) - start_cpy;
                start_cpy = i + 1;
                targetIndex = 0;
            }
        }else if ((targetIndex + 1 < strlen(task.target)) && (task.target[targetIndex + 1] == '*')){
            if (targetIndex == 0 )
                start_cpy = i;
            while (((task.i == 0) && (word[i] == task.target[targetIndex])) || ((task.i == 1) && (charlwr(word[i]) == charlwr(task.target[targetIndex]))))
                i++;
            i--;
            targetIndex += 2;
        }else if (targetIndex < strlen(task.target) && ((task.i == 0) && (word[i] == task.target[targetIndex])) || ((task.i == 1) && (charlwr(word[i]) == charlwr(task.target[targetIndex])))){
            if (targetIndex == 0)
                start_cpy =i;
            targetIndex++;
        }else if (targetIndex == strlen(task.target)){
            strcat(newWord, task.edit);
            targetIndex = 0;
            newIndex += strlen(task.edit);
            start_cpy = i;
            i--;
        }else{
            strncpy(&newWord[newIndex], &word[start_cpy], (i + 1) - start_cpy);
            newIndex += (i + 1) - start_cpy;
            start_cpy = i + 1;
            targetIndex = 0;
        }
        if (i == strlen(word) - 1){
            if (targetIndex == strlen(task.target)){
                strcat(newWord, task.edit);
                newIndex += strlen(task.edit);
            }else{
                strncpy(&newWord[newIndex], &word[start_cpy], (i + 1) - start_cpy);
                newIndex += (i + 1) - start_cpy;
            }
        }
    }

    return newWord;
}

