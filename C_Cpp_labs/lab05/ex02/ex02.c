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

pthread_mutex_t mutex_thFinished = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_thSignaling = PTHREAD_MUTEX_INITIALIZER;

typedef struct ord_th_s{    //structure for the oredering threads
    int id;             //id assigned by the main
    int n_elem;         //number of elements
    int *file_cont;     //content of the file
    int *elements;      //array of the elements
    int *th_finished;   //array of flags to know which one finished
    pthread_t TID;      //real TID
    char *filePath;     //path of the file
}ord_th_t;

void *thFcnOrder(void *);       //function executed by the ordering threads
int *merge2arr(int *, int, int *, int); //merge 2 arrays

int main(int argc, char *argv[]){
    setbuf(stdout, 0);
    //check if at least 1 in e 1 out files have been inserted
    if(argc < 3){
        printf("Not enough paramaters: %d passed\n", argc-1);
        exit(1);
    }

    int i, ret, merg_active = 0;
    int msg_received = 0;
    int num_in_files = argc - 2;    //num of inpt files
    int *th_finished;               //array of flags to know which one finished
    char *outFile;                  //output files
    ord_th_t *th_args;              //array of structures containing parameters to be passed to ord th
    int tot_elem = 0;               //total number of elements read from files

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

    int *taken = (int *)calloc(num_in_files, sizeof(int));
    int *finish_seq = (int *)calloc(num_in_files, sizeof(int));
    int *tmp;

    //until all threads end
    while(msg_received < num_in_files){
        pthread_mutex_lock(&mutex_thSignaling);
        //keep polling the array of flags, searching the first thread terminated
        for(i=0; i<num_in_files; i++)
            if(th_finished[i]==1 && taken[i]==0){   //taken[i]=1 means 'termination already seen'
                finish_seq[msg_received] = i;       //if a new thread terminated, put it in the queue
                msg_received++;                     //update num of thread terminated
                tot_elem += th_args[i].n_elem;      //update num of total elements
                taken[i]=1;                         //save that thread termination has been already seen
                /*printf("%d\n", th_args[i].n_elem);
                for(int j=0; j<th_args[i].n_elem; i++)
                    printf("%d ", th_args[i].elements[j]);*/
                
                break;
            }
        pthread_mutex_unlock(&mutex_thSignaling);

        if(msg_received == 2)       //first time merge 2 'original' arrays
            tmp = merge2arr(th_args[finish_seq[0]].elements, th_args[finish_seq[0]].n_elem, 
                            th_args[finish_seq[1]].elements, th_args[finish_seq[1]].n_elem);
        else if(msg_received > 2)   //next times merge 1 'original' array and the previous result
            tmp = merge2arr(th_args[finish_seq[msg_received-1]].elements, 
                            th_args[finish_seq[msg_received-1]].n_elem, 
                            tmp, tot_elem - th_args[finish_seq[msg_received-1]].n_elem);

    }
/*
    for(i=0; i<tot_elem; i++)
        printf("%d ", tmp[i]);
*/    
    int fd = open(outFile, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG);
    if(fd < 0){
        perror("Error opening the file for writing\n");
        exit(1);
    }
    for(i=0; i<tot_elem; i++)
        write(fd, &tmp[i], sizeof(int));
    close(fd);
/*
    fd = open(outFile, O_RDONLY);
    if(fd < 0){
        perror("Error opening the file for reading\n");
        exit(1);
    }
    for(i=0; i<tot_elem; i++){
        read(fd, &tmp[i], sizeof(int));
        printf("%d ", tmp[i]);
    }
        
    close(fd);
*/

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
    //printf("%d\n", arg->n_elem);

    //allocate and init the array of elements
    arg->elements = (int *)malloc(arg->n_elem * sizeof(int));
    if(arg->elements == NULL){
        perror("Thread failed allocating the array of elements: ");
        exit(1);
    }
    //arg->elements = arg->file_cont;
    for(i=0; i<arg->n_elem; i++){
        arg->elements[i] = arg->file_cont[i];
        printf("%d ", arg->elements[i]);
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
    pthread_mutex_lock(&mutex_thSignaling);
    arg->th_finished[arg->id] = 1;
    pthread_mutex_unlock(&mutex_thSignaling);

    //pthread_exit(0);

}

int *merge2arr(int *arr1, int size1, int *arr2, int size2){
    int *tmp = (int *)malloc((size1+size2) * sizeof(int));
    int taken1=0, taken2=0, min;

    if(tmp == NULL){
        perror("Allocating tmp array: ");
        exit(1);
    }

    for(int i=0;i<size1+size2; i++){
        if(taken2 >= size2)             //if there are no other values from arr2
            tmp[i] = arr1[taken1++];    //take directly form arr1
        else if(taken1 >= size1)        //if there are no other values from arr1
            tmp[i] = arr2[taken2++];    //take directly form arr2
        else if(arr1[taken1] < arr2[taken2])
            tmp[i] = arr1[taken1++];
        else if(arr2[taken2] < arr1[taken1])
            tmp[i] = arr2[taken2++];
        //printf("%d ", tmp[i]);
    }/*
    free(arr1);
    free(arr2);
*/
    return tmp;
}