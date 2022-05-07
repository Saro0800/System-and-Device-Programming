#include "AppThread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>
#include <sys/mman.h>

int *merge2arr(int *arr1, int size1, int *arr2, int size2);

int main(int argc, char *argv[]){

    //check for parameters
    if(argc < 3){
        printf("Not enough parameters\n");
        exit(1);
    }

    int i, num_in_files = argc - 2;    //first one is the name of the program, last one is the out file
    int *taken;
    int *th_finished_arr;
    int n_total = 0, fd;
    int th_finished = 0;
    int *end_order;
    int *tmp;
    int *merg_done;
    char **in_files, *out_file;
    thread_arg_t *args_array;
    pthread_mutex_t *mutex_thSignaling;

    in_files = (char **)malloc(num_in_files * sizeof(char *));  //allocate the array of strings
    for(i=0; i<num_in_files; i++)
        in_files[i] = strdup(argv[i+1]);    //allocate and init each input string

    out_file = strdup(argv[argc-1]);        //allocate and init the output string

    //allocate the array of flags
    th_finished_arr = (int *)malloc(num_in_files * sizeof(int));

    //allocate the mutex
    mutex_thSignaling = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(mutex_thSignaling, NULL);

    //allocate the array of arguments for the threads
    args_array = (thread_arg_t *)malloc(num_in_files * sizeof(thread_arg_t));
    //init the arguments for each thread and create it
    for(i=0; i<num_in_files; i++){
        args_array[i].id = i;
        args_array[i].filePath = in_files[i];
        args_array[i].th_finished = th_finished_arr;
        args_array[i].mutex_thSignaling = mutex_thSignaling;
        pthread_create(&args_array[i].tid, NULL, threadFunc, (void *)&args_array[i]);
    }    

    taken = (int *)calloc(num_in_files, sizeof(int));
    end_order = (int *)calloc(num_in_files, sizeof(int));
    merg_done = (int *)calloc(num_in_files-1, sizeof(int));

    while(th_finished < num_in_files){
        pthread_mutex_lock(mutex_thSignaling);
        for(i=0; i<num_in_files; i++)
            if(th_finished_arr[i]==1 && taken[i]==0){
                taken[i] = 1;
                end_order[th_finished] = i;
                n_total += args_array[i].n_elem;
                th_finished++;
                break;
            }
        pthread_mutex_unlock(mutex_thSignaling);

        if(th_finished == 2 && merg_done[th_finished-2]!=1){
            tmp = merge2arr(args_array[end_order[0]].elements, args_array[end_order[0]].n_elem,
                            args_array[end_order[1]].elements, args_array[end_order[1]].n_elem);
            printf("merging T%d and T%d\n", end_order[0], end_order[1]);
            merg_done[th_finished-2] = 1;
        }
        else if(th_finished > 2 && merg_done[th_finished-2]!=1){
            tmp = merge2arr(tmp, n_total - args_array[end_order[th_finished-1]].n_elem,
                            args_array[end_order[th_finished-1]].elements, 
                            args_array[end_order[th_finished-1]].n_elem);
            printf("merging TMP and T%d\n", end_order[th_finished-1]);
            merg_done[th_finished-2] = 1;
        }

    }
    
    //./ex02 ./tmp/file1.bin ./tmp/file2.bin ./tmp/file3.bin ./tmp/file4.bin ./tmp/file5.bin ./tmp/file6.bin ./tmp/file7.bin ./tmp/file8.bin ./tmp/file9.bin ./tmp/file10.bin ./tmp/fileOut.bin 

    //open output file
    fd = open(out_file, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO);
    if(fd < 0){
        perror("Error  in main opening the output file:\n");
        exit(1);
    }

    //size the file
    if(ftruncate(fd, n_total*sizeof(int))){
        perror("Error defining the size of the output file:\n");
        exit(1);
    }

    //map the file into the memory
    merg_done = (int *)mmap(NULL, n_total*sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd, SEEK_SET);
    if(merg_done == MAP_FAILED){
        perror("main failed mapping the output file in memory");
        exit(1);
    }

    for(i=0; i<n_total; i++)
        merg_done[i] = tmp[i];

    return 0;
}

int *merge2arr(int *arr1, int size1, int *arr2, int size2){
    int *tmp = (int *)malloc((size1+size2) * sizeof(int));
    int taken1=0, taken2=0;

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



