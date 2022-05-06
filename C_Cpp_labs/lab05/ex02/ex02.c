#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>

extern int errno;

typedef struct ord_th_s{    //structure for the oredering threads
    int id;             //id assigned by the main
    int n_elem;         //number of elements
    int *file_cont;     //content of the file
    int *elements;      //array of the elements
    int *th_finished;   //array of flags to know which one finished
    pthread_t TID;      //real TID
    char *filePath;     //path of the file
}ord_th_t;

void *thFcnOrder(void *);    //function executed by the ordering threads

int main(int argc, char *argv[]){
    setbuf(stdout, 0);
    //check if at least 1 in e 1 out files have been inserted
    if(argc < 3){
        printf("Not enough paramaters: %d passed\n", argc-1);
        exit(1);
    }

    int i, ret;
    int msg_received = 0;
    int num_in_files = argc - 2;    //num of inut files
    int *th_finished;               //array of flags to know which one finished
    char *outFile;                  //output files
    ord_th_t *th_args;              //array of structures containing parameters to be passed to ord th

    //take the output file
    outFile = strdup(argv[argc-1]);

    //allocate the array of flags
    th_finished = (int *)malloc(num_in_files * sizeof(int));
    if(th_finished == NULL){
        perror("Main failed allocating th_finished: ");
        exit(1);
    }
    
    //allocate the argument array
    th_args = (ord_th_t *)malloc(num_in_files * sizeof(ord_th_t));
    if(th_args == NULL){
        perror("Main failed allocating th_args: ");
        exit(1);
    }

    //init the array and launch the threads
    for(i=0; i<num_in_files; i++){
        th_args[i].id = i;
        th_args[i].filePath = strdup(argv[i+1]);
        th_args[i].th_finished = th_finished;
        //printf("%s\n", th_args[i].filePath);
        pthread_create(&th_args[i].TID, NULL, thFcnOrder, (void *)&th_args[i]);
    }

    while(msg_received < num_in_files){
        for(i=0; i<num_in_files; i++){
            if(th_finished[i] == 1){
                printf("Thread %d finished\n", i);
                msg_received++;
            }
            else
                printf("Thread %d hasn't finished yet\n", i);
            sleep(1);
            if(msg_received >= 2){
                
            }
        }
    }

    return 0;
}

void *thFcnOrder(void *args){
    int fd, ret;
    int max, pos_max, i, j;
    struct stat st;                     //structure for file information
    ord_th_t *arg = (ord_th_t *)args;   //retrieve the argument

    //main thread will not wait orgering threads, so we can detach them
    pthread_detach(arg->TID);

    //open the file
    fd = open(arg->filePath, O_RDONLY);
    if(fd < 0){
        perror("Thread failed opening file: ");
        exit(1);
    }

    //retrieve information about the file
    ret = stat(arg->filePath, &st);
    if(ret < 0){
        perror("Thread failed retrieving information about input file: ");
        exit(1);
    }

    //map the file in memory
    arg->file_cont = (int *)mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, SEEK_SET);
    if(arg->file_cont == MAP_FAILED){
        perror("Thread failed mapping the file in memory: ");
        exit(1);

    }

    //retrieve the number of elements
    arg->n_elem = *(arg->file_cont++);

    //allocate and init the array of elements
    arg->elements = (int *)malloc(arg->n_elem * sizeof(int));
    if(arg->elements == NULL){
        perror("Thread failed allocating the array of elements: ");
        exit(1);
    }
    //arg->elements = arg->file_cont;
    for(i=0; i<arg->n_elem; i++){
        arg->elements[i] = arg->file_cont[i];
        //printf("%d ", arg->elements[i]);
    }
    //printf("%d\n", i);

    //order the array using the bubble sort algorithm
    for(i=0; i<arg->n_elem; i++){
        max = -1024;
        for(j=0; j<arg->n_elem-i; j++)
            if(arg->elements[j] > max){
                max = arg->elements[j];
                pos_max = j;
            }
        arg->elements[pos_max] = arg->elements[j - 1];
        arg->elements[j - 1] = max;
    }
/*
    printf("%d\n", i);
    for(int i=0; i<arg->n_elem; i++)
        printf("%d ", arg->elements[i]);
*/

    //the thread signals that it finished
    arg->th_finished[arg->id] = 1;

    pthread_exit(0);

}




