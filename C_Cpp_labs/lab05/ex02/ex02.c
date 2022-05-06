/*
    Version 2 : memory mapped I/O
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>

#define DEBUG_PRINT 0
#define DEBUG_PRINT_1 0
#define DEBUG_PRINT_2 0
#define DEBUG_PRINT_3 1
#define DEBUG_PRINT_4 0

typedef struct thread_arg_s{
    unsigned int id, n_elem;
    pthread_t tid;
    int *file_content, *elements;
    char *filePath;
    int *th_finished;
}thread_arg_t;

pthread_mutex_t mutex_to_print = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_thSignaling = PTHREAD_MUTEX_INITIALIZER;

void *threadFunc(void *arg);
int *merge2arr(int *arr1, int size1, int *arr2, int size2);

int main(int argc, char *argv[]){

    //check for parameters
    if(argc < 3){
        printf("Not enough parameters\n");
        exit(1);
    }

    int i, min, num_in_files = argc - 2;    //first one is the name of the program, last one is the out file
    int **ordered_arrays, *max, *taken;
    int *th_finished_arr;
    int n_total = 0, fd;
    char **in_files, *out_file;
    thread_arg_t *args_array;

    in_files = (char **)malloc(num_in_files * sizeof(char *));  //allocate the array of strings
    for(i=0; i<num_in_files; i++)
        in_files[i] = strdup(argv[i+1]);    //allocate and init each input string

    out_file = strdup(argv[argc-1]);        //allocate and init the output string

    if(DEBUG_PRINT){
        printf("Input files:\n");
        for(i=0; i<num_in_files; i++)
            printf("%s\n", in_files[i]);
        printf("\nOutput file:\n%s\n\n", out_file);
    }

    //allocate the array of flags
    th_finished_arr = (int *)malloc(num_in_files * sizeof(int));

    //allocate the array of arguments for the threads
    args_array = (thread_arg_t *)malloc(num_in_files * sizeof(thread_arg_t));
    //init the arguments for each thread and create it
    for(i=0; i<num_in_files; i++){
        args_array[i].id = i;
        args_array[i].filePath = in_files[i];
        args_array[i].th_finished = th_finished_arr;
        pthread_create(&args_array[i].tid, NULL, threadFunc, (void *)&args_array[i]);
    }    

    int th_finished = 0;
    int *end_order;
    int *tmp;
    int *merg_done;

    taken = (int *)calloc(num_in_files, sizeof(int));
    end_order = (int *)calloc(num_in_files, sizeof(int));
    merg_done = (int *)calloc(num_in_files-1, sizeof(int));

    while(th_finished < num_in_files){
        pthread_mutex_lock(&mutex_thSignaling);
        for(i=0; i<num_in_files; i++)
            if(th_finished_arr[i]==1 && taken[i]==0){
                taken[i] = 1;
                end_order[th_finished] = i;
                n_total += args_array[i].n_elem;
                th_finished++;
                /*
                printf("T%d finished: \n", i);
                for(int j=0; j<args_array[i].n_elem; j++)
                    printf("%d ", args_array[i].elements[j]);
                printf("\n\n");
                */
                break;
            }
        pthread_mutex_unlock(&mutex_thSignaling);

        if(th_finished == 2 && merg_done[th_finished-2]!=1){
            tmp = merge2arr(args_array[end_order[0]].elements, args_array[end_order[0]].n_elem,
                            args_array[end_order[1]].elements, args_array[end_order[1]].n_elem);
            //printf("merging T%d and T%d\n", end_order[0], end_order[1]);
            merg_done[th_finished-2] = 1;
        }
        else if(th_finished > 2 && merg_done[th_finished-2]!=1){
            tmp = merge2arr(tmp, n_total - args_array[end_order[th_finished-1]].n_elem,
                            args_array[end_order[th_finished-1]].elements, 
                            args_array[end_order[th_finished-1]].n_elem);
            //printf("merging TMP and T%d\n", end_order[th_finished-1]);
            merg_done[th_finished-2] = 1;
        }

    }
    
    //./ex02_v2 ./tmp/file1.bin ./tmp/file2.bin ./tmp/file3.bin ./tmp/file4.bin ./tmp/file5.bin ./tmp/file6.bin ./tmp/file7.bin ./tmp/file8.bin ./tmp/file9.bin ./tmp/file10.bin ./tmp/fileOut.bin 

    //open output file
    fd = open(out_file, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO);
    if(fd < 0){
        printf("Error  in main opening the output file\n");
        exit(1);
    }

    for(i=0; i<n_total; i++)
        write(fd, &tmp[i], sizeof(int));

    return 0;
}

void *threadFunc(void *arg){
    thread_arg_t *myArg = (thread_arg_t *)arg;
    int fd, i;
    struct stat st;

    pthread_detach(myArg->tid);

    //open the file
    fd = open(myArg->filePath, O_RDONLY);
    if(fd < 0){
        printf("T%d failed opening file %s\n", myArg->id, myArg->filePath);
        pthread_exit(NULL);
    }
    else if(DEBUG_PRINT)
        printf("T%d correctly opened file %s\n", myArg->id, myArg->filePath);

    //retrieve information about the file
    if(stat(myArg->filePath, &st) < 0){
        printf("T%d failed retrieving information about %s\n", myArg->id, myArg->filePath);
        pthread_exit(NULL);
    }

    //map the file into the memory
    myArg->file_content = (int *)mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, SEEK_SET);

    //read the number of elements in the file
    myArg->n_elem = *myArg->file_content;
    myArg->file_content++;  //point to the first element
    if(DEBUG_PRINT)
        printf("T%d has to read %d elements from file %s\n", myArg->id, myArg->n_elem, myArg->filePath);

    //alloc the array of integers
    myArg->elements = (int *)malloc(myArg->n_elem * sizeof(int));
    if(myArg->elements == NULL){
        printf("T%d failed allocating the array of integers\n", myArg->id);
        pthread_exit(NULL);
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

    pthread_mutex_lock(&mutex_thSignaling);
    myArg->th_finished[myArg->id] = 1;
    pthread_mutex_unlock(&mutex_thSignaling);

    //pthread_exit(NULL);
}

int *merge2arr(int *arr1, int size1, int *arr2, int size2){
    int *tmp = (int *)malloc((size1+size2) * sizeof(int));
    int taken1=0, taken2=0, min;

    if(tmp == NULL){
        perror("Failed allocating tmp array: ");
        exit(1);
    }

    for(int i=0;i<size1+size2; i++){
        if(taken1 >= size1)
            tmp[i] = arr2[taken2++];
        else if(taken2 >= size2)
            tmp[i] = arr1[taken1++];
        else if(arr1[taken1] <= arr2[taken2])
            tmp[i] = arr1[taken1++];
        else 
            tmp[i] = arr2[taken2++];
    }
    free(arr1);
    free(arr2);

    return tmp;
}



