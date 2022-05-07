#include "AppThread.h"
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>

extern int errno;

void *threadFunc(void *arg){
    thread_arg_t *myArg = (thread_arg_t *)arg;
    int fd, i;
    struct stat st;

    pthread_detach(myArg->tid);

    //open the file
    fd = open(myArg->filePath, O_RDONLY);
    if(fd < 0){
        perror("T failed opening input file \n");
        exit(1);
    }

    //retrieve information about the file
    if(stat(myArg->filePath, &st) < 0){
        perror("T failed retrieving information about the input file \n");
        exit(1);
    }

    //map the file into the memory
    myArg->file_content = (int *)mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, SEEK_SET);
    if(myArg->file_content == MAP_FAILED){
        perror("Thread failed mapping the file in memory:\n");
        exit(1);
    }

    //read the number of elements in the file
    myArg->n_elem = *myArg->file_content;
    myArg->file_content++;  //point to the first element

    //alloc the array of integers
    myArg->elements = (int *)malloc(myArg->n_elem * sizeof(int));
    if(myArg->elements == NULL){
        perror("T failed allocating the array of integers\n");
        exit(1);
    }

    //get the array of integers
    for(i=0; i<myArg->n_elem; i++){
        myArg->elements[i] = *myArg->file_content;
        myArg->file_content++;
    }

    //close the file
    close(fd);

    //order the array using the bubble sort algorithm
    int max, pos_max, j;
    for(i=0; i<myArg->n_elem; i++){
        max = -1;
        for(j=0; j<myArg->n_elem-i; j++)
            if(myArg->elements[j] > max){
                max = myArg->elements[j];
                pos_max = j;
            }
        myArg->elements[pos_max] = myArg->elements[j-1]; 
        myArg->elements[j-1] = max;
    }

    pthread_mutex_lock(myArg->mutex_thSignaling);
    myArg->th_finished[myArg->id] = 1;
    pthread_mutex_unlock(myArg->mutex_thSignaling);

    pthread_exit(NULL);
}
