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

#define SHM_SIZE 1024       //size of the chunk of shared memory
#define TIMES 10000         //number of loop
#define N_BYTES 20          //length of the messages

typedef struct msg_s{       //structure defining the message
    long mtype;
    char msgtxt[N_BYTES];
}msg_t;

enum msg_type{              //label for the message type
    P1_msg = 1,
    P2_msg = 2
};

void P1_fcn(int shm_id, int msgq_id);
void P2_fcn(int shm_id, int msgq_id);
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
    void *status;

    int shm_id;                         //shared memory id
    int msgq_id;                        //message queue id

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
    shm_id = shmget(key, SHM_SIZE, 0666 | IPC_CREAT);
    if(shm_id == -1){
        printf("Main failed getting shm_id\n");
        exit(1);
    }

    //create or open the mesage queue
    msgq_id = msgget(key, 0644 | IPC_CREAT);
    if(msgq_id == -1){
        perror("Main failed creating or opening the message queue\n");
        exit(1);
    }
    //create the processes
    if(!fork())
        P2_fcn(shm_id, msgq_id);    //child process

    P1_fcn(shm_id, msgq_id);        //parent process

    wait(status);

    //remove the queue from the system
    if(msgctl(msgq_id, IPC_RMID, NULL) < 0){
        perror("Main failed removing the queue\n");
        exit(1);
    }
    

    return 0;
}

void P1_fcn(int shm_id, int msgq_id){

    int seed = getpid();
    int i, ret;
    unsigned int size;
    char *shared_mem;       //chunk of shared memory
    msg_t *msg = (msg_t *)malloc(sizeof(msg_t));    //struct to send an receive messages

    //attach the shared memory
    shared_mem = (char *) shmat(shm_id, 0, 0);
    if(shared_mem == (char *)-1){
        printf("P1 failed attacching the chunck of shared memory\n");
        exit(1);
    }
    for(int i=0; i<TIMES; i++){
        //randomize the length of the message
        size = rand_r(&seed)%(SHM_SIZE-1);   
        //randomize the message
        generate_random_shm(shared_mem, size);

        //wake up P2 by sending it a message
        msg->mtype = (long ) P1_msg;                         //set the message type
        sprintf(msg->msgtxt, "P1_done: %d", size);            //write the message text
        ret = msgsnd(msgq_id, (void *)msg, sizeof(msg_t), 0);   //enqueue the message
        if(ret < 0){        //check for error
            perror("P1 failed sending a message\n");
            exit(1);
        }

        //sleep(1);

        //wait to be waken up by P2
        ret = msgrcv(msgq_id, (void *)msg, sizeof(msg_t), (long )P2_msg, 0);
        if(ret < sizeof(msg_t)){        //check for error
            perror("P1 failed receiving a message\n");
            exit(1);
        }
        sscanf(msg->msgtxt, "P2_done: %d", &size);
        printf("\n\nP1: size: %d\n", size);
        read_shm(shared_mem, size);
    }
    
    //detach the memory
    shmdt(shared_mem);

}

void P2_fcn(int shm_id, int msgq_id){
    int seed = getpid();
    int i, ret;
    unsigned int size;
    char *shared_mem;       //chunk of shared memory
    msg_t *msg = (msg_t *)malloc(sizeof(msg_t));    //struct to send an receive messages

    //attach the shared memory
    shared_mem = (char *) shmat(shm_id, 0, 0);
    if(shared_mem == (char *)-1){
        printf("P2 failed attacching the chunck of shared memory\n");
        exit(1);
    }

    for(int j=0; j<TIMES; j++){
        //wait to be waken up by P1
        ret = msgrcv(msgq_id, (void *)msg, sizeof(msg_t), (long )P1_msg, 0);
        if(ret < sizeof(msg_t)){
            perror("P2 failed receiving a message\n");
            exit(1);
        }
        sscanf(msg->msgtxt, "P1_done: %d", &size);
        //print the message
        printf("\n\nP2: size: %d\n", size);
        read_shm(shared_mem, size);

        //sleep(1);

        //randomize the length of the message
        size = rand_r(&seed)%(SHM_SIZE-1);   
        //randomize the message
        generate_random_shm(shared_mem, size);
        //wake up P1 by sending it a message
        msg->mtype = (long int)P2_msg;                          //set msg type
        sprintf(msg->msgtxt, "P2_done: %d", size);               //set msg text
        ret = msgsnd(msgq_id, (void *)msg, sizeof(msg_t), 0);   //send the message
        if(ret < 0){
            perror("P2 failed sending a message\n");
            exit(1);
        }
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

