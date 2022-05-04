#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/types.h>


#define deb_print_lv1 0
#define deb_print_lv2 0

#define SHM_SIZE 1024
#define TIMES 3

void P1_fcn(int shm_id, int P1toP2[2], int P2toP1[2]);
void P2_fcn(int shm_id, int P1toP2[2], int P2toP1[2]);
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
    int P1toP2[2], P2toP1[2];         //pipe for synchronizing the proc
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

    //create the pipe
    if(pipe(P1toP2) < 0){
        printf("Main failed creating the P1toP2\n");
        exit(1);
    }

    if(pipe(P2toP1) < 0){
        printf("Main failed creating the P2toP1\n");
        exit(1);
    }

    //create the processes
    if(!fork())
        P2_fcn(shm_id, P1toP2, P2toP1);    //child process

    P1_fcn(shm_id, P1toP2, P2toP1);        //parent process

    wait(status);

    return 0;
}

void P1_fcn(int shm_id, int P1toP2[2], int P2toP1[2]){

    int seed = getpid();
    int randomized;
    int i;
    unsigned int size;
    char *shared_mem;       //chunk of shared memory

    //attach the shared memory
    shared_mem = (char *) shmat(shm_id, 0, 0);
    if(shared_mem == (char *)-1){
        printf("P1 failed attacching the chunck of shared memory\n");
        exit(1);
    }
    for(int j=0; j<TIMES; j++){
        //randomize the length of the message
        size = rand_r(&seed)%(SHM_SIZE-1);   
        //randomize the message
        generate_random_shm(shared_mem, size);
        write(P1toP2[1], &size, sizeof(int));

        //sleep(1);

        read(P2toP1[0], &size, sizeof(int));
        printf("\n\nP1: size: %d\n", size);
        read_shm(shared_mem, size);
    }
    
    //detach the memory
    shmdt(shared_mem);

}

void P2_fcn(int shm_id, int P1toP2[2], int P2toP1[2]){
    int seed = getpid();
    int randomized;
    int i;
    unsigned int size;
    char *shared_mem;       //chunk of shared memory

    //attach the shared memory
    shared_mem = (char *) shmat(shm_id, 0, 0);
    if(shared_mem == (char *)-1){
        printf("P2 failed attacching the chunck of shared memory\n");
        exit(1);
    }

    for(int j=0; j<TIMES; j++){
        read(P1toP2[0], &size, sizeof(int));
        printf("\n\nP2: size: %d\n", size);
        read_shm(shared_mem, size);

        //sleep(1);

        //randomize the length of the message
        size = rand_r(&seed)%(SHM_SIZE-1);   
        //randomize the message
        generate_random_shm(shared_mem, size);
        write(P2toP1[1], &size, sizeof(int));
    }
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





