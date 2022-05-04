#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/stat.h>

#define deb_print_lv1 0
#define deb_print_lv2 0

#define SHM_SIZE 1024
#define TIMES 3

extern int errno;

void P1_fcn(int shm_id);
void P2_fcn(int shm_id);
void generate_random_shm(char* shm, unsigned int len);
void read_shm(char* shm, unsigned int len);

int main(int argc, char *argv[]){

    setbuf(stdout, 0);

    //check parameters
    if(argc < 3){
        printf("Missing parameters (%d/2)\n", argc-1);
        exit(1);
    }

    key_t key;                          //key for the IPC structure
    int id = atoi(argv[1]);             //Project id for the key
    char *pathname = strdup(argv[2]);   //File for the key
    char *fifo_1to2 = "./tmp/fifo_1to2.txt";
    char *fifo_2to1 = "./tmp/fifo_2to1.txt";
    
    void *status;

    int shm_id;                         //shared memory id

    //print parameters for debug purpouse
    if(deb_print_lv1){
        printf("id: %d\n",id);
        printf("pathname: %s\n", pathname);
    }

    //create the key
    key = ftok(pathname, id);
    if(key == -1){
        printf("Main failed creating the key\n");
        exit(1);
    }

    //get the shared memory identifier
    shm_id = shmget(key, SHM_SIZE, 0644 | IPC_CREAT);
    if(shm_id == -1){
        printf("Main failed getting shm_id\n");
        exit(1);
    }

    //system("mkdir ./tmp");
    //create the fifo_1to2
    if(mkfifo(fifo_1to2, 0666) < 0){
        perror("Main failed creating fifo_1to2\n");
        exit(1);
    }

    //create the fifo_2to1
    if(mkfifo(fifo_2to1, 0666) < 0){
        perror("Main failed creating fifo_1to2\n");
        exit(1);
    }

    //create the processes
    if(!fork())
        P2_fcn(shm_id);    //child process

    P1_fcn(shm_id);        //parent process

    wait(status);

    system("rm ./tmp/fifo_1to2.txt");
    system("rm ./tmp/fifo_2to1.txt");

    return 0;
}

void P1_fcn(int shm_id){

    int seed = getpid();
    unsigned int size;
    char *shared_mem;       //chunk of shared memory

    char *fifo_1to2 = "./tmp/fifo_1to2.txt";
    char *fifo_2to1 = "./tmp/fifo_2to1.txt";

    int fifo1to2, fifo2to1;

    //attach the shared memory
    shared_mem = (char *) shmat(shm_id, 0, 0);
    if(shared_mem == (char *)-1){
        perror("P1 failed attacching the chunck of shared memory\n");
        exit(1);
    }

    //opening fifo fifo_1to2 for writing on it
    fifo1to2 = open(fifo_1to2, O_WRONLY);
    if(fifo1to2 < 0){
        perror("P1 failed opening fifo1to2\n");
        exit(1);
    }

    //opening fifo fifo_2to1 for reading from it
    fifo2to1 = open(fifo_2to1, O_RDONLY);
    if(fifo2to1 < 0){
        perror("P1 failed opening fifo2to1\n");
        exit(1);
    }

    for(int j=0; j<TIMES; j++){
        //randomize the length of the message
        size = rand_r(&seed)%(SHM_SIZE-1);   
        //randomize the message
        generate_random_shm(shared_mem, size);
        write(fifo1to2, &size, sizeof(int));

        //sleep(1);

        read(fifo2to1, &size, sizeof(int));
        printf("\n\nP1: size: %d\n", size);
        read_shm(shared_mem, size);
    }
    
    close(fifo1to2);
    close(fifo2to1);

    //detach the memory
    shmdt(shared_mem);

}

void P2_fcn(int shm_id){
    int seed = getpid();
    unsigned int size;
    char *shared_mem;       //chunk of shared memory

    char *fifo_1to2 = "./tmp/fifo_1to2.txt";
    char *fifo_2to1 = "./tmp/fifo_2to1.txt";

    int fifo1to2, fifo2to1;

    //attach the shared memory
    shared_mem = (char *) shmat(shm_id, 0, 0);
    if(shared_mem == (char *)-1){
        perror("P2 failed attacching the chunck of shared memory\n");
        exit(1);
    }

    //opening fifo fifo_1to2 for reading from it
    fifo1to2 = open(fifo_1to2, O_RDONLY);
    if(fifo1to2 < 0){
        perror("P1 failed opening fifo1to2\n");
        exit(1);
    }

    //opening fifo fifo_2to1 for writin on it
    fifo2to1 = open(fifo_2to1, O_WRONLY);
    if(fifo2to1 < 0){
        perror("P1 failed opening fifo2to1\n");
        exit(1);
    }


    for(int j=0; j<TIMES; j++){
        read(fifo1to2, &size, sizeof(int));
        printf("\n\nP2: size: %d\n", size);
        read_shm(shared_mem, size);

        //sleep(1);

        //randomize the length of the message
        size = rand_r(&seed)%(SHM_SIZE-1);   
        //randomize the message
        generate_random_shm(shared_mem, size);
        write(fifo2to1, &size, sizeof(int));
    }
    
    close(fifo1to2);
    close(fifo2to1);

    //detach the memory
    shmdt(shared_mem);

    exit(1);
}

void generate_random_shm(char* shm, unsigned int len){
    int i;
    int seed = getpid();
    int rand;

    for(i=0; i<len; i++){
        rand = rand_r(&seed)%28;
        if(rand < 26)
            shm[i] = rand + 'a';
        else if(rand == 26)
            shm[i] = ' ';
        else if(rand == 27)
            shm[i] = '\n';
        //printf("rand: %d char: %c\n", rand, shm[i]);
    }
    shm[i] = '\0';
}

void read_shm(char* shm, unsigned int len){
    int i;

    for(i=0; i<len; i++){
        if(shm[i]!='\n' && shm[i]!=' ')
            printf("%c", shm[i]-'a'+'A');
        else
            printf("%c", shm[i]);
         
    }
}





