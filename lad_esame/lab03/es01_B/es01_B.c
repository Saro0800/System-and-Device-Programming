#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <error.h>
#include <sys/wait.h>

#define WAIT_TIME1 10
#define WAIT_TIME2 3
#define STR_NUM 5
#define STR_SIZE 128

extern int errno;

typedef struct{
    int size;
    char *str;
} rw_block;

void P_fcn(int *p1, int *p2);
void C_fcn(int *p, const int wait_time);
char *genString(const int size);
void convertAndPrint(char *str, int size);

int main(){
setbuf(stdout, 0);

    int p1[2], p2[2];
    int flag;

    if(pipe(p1) < 0){
        perror("Error creating P1: ");
        exit(1);
    }

    if(pipe(p2) < 0){
        perror("Error creating p2: ");
        exit(1);
    }

    //modify pipe1 to make read end non blocking
    if(fcntl(p1[0], F_GETFL, &flag) < 0){
        perror("Error getting p1[0] flag: ");
        exit(1);
    }
    flag |= O_NONBLOCK;
    if(fcntl(p1[0], F_SETFL, &flag) < 0){
        perror("Error setting p1[0] flag: ");
        exit(1);
    }

    //modify pipe2 to make read end non blocking
    if(fcntl(p2[0], F_GETFL, &flag) < 0){
        perror("Error getting flag of p2[0]: ");
        exit(1);
    }
    flag |= O_NONBLOCK;
    if(fcntl(p2[0], F_SETFL, &flag) < 0){
        perror("Error setting flag of p2[0]: ");
        exit(1);
    }

    if(fork()==0)
        C_fcn(p1, WAIT_TIME1);
    
    if(fork()==0)
        C_fcn(p2, WAIT_TIME2);

    P_fcn(p1, p2);

    wait(NULL);
    wait(NULL);

    return 0;
}

void P_fcn(int *p1, int *p2){
    rw_block block;

    for(int i=0; i<2*STR_NUM; ){
        if(read(p1[0], &block, sizeof(rw_block)) > 0){
            i++;
            //printf("C1: ");
            convertAndPrint(block.str, block.size);
        }

        if(read(p2[0], &block, sizeof(rw_block)) > 0){
            i++;
            //printf("\tC2: ");
            convertAndPrint(block.str, block.size);
        }
        sleep(1);
    }
}

void C_fcn(int *p, const int wait_time){
    unsigned int seed = getpid();
    rw_block block;

    for(int i=0; i<STR_NUM; i++){
        block.size = rand_r(&seed)%STR_SIZE + 1;
        block.str = genString(block.size);
        printf("\t\t%s\n", block.str);
        if(write(p[1], &block, sizeof(block))<0){
            perror("Error in C writing the R/W block: ");
            exit(1);
        }
        
        sleep(wait_time);
    }
    exit(0);
}

char *genString(const int size){
    char *tmp = malloc((size+1)*sizeof(char));
    unsigned int seed = getpid();

    for(int i=0; i<size; i++)
        tmp[i] = rand_r(&seed)%26+'a';
    tmp[size]='\0';
    //printf("\t\t%s\n", tmp);
    return tmp;
}

void convertAndPrint(char *str, int size){
    //printf("\t\t%s\n",str);
    for(int i=0; i<size; i++);
        //printf("%c", str[i]-'a'+'A');
    printf("\n");
}

