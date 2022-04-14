#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>

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
}thread_arg_t;

pthread_mutex_t mutex_to_print = PTHREAD_MUTEX_INITIALIZER;

void *threadFunc(void *arg);
int getMin(int num, int *taken, int *max, int **arrays);

int main(int argc, char *argv[]){

    //check for parameters
    if(argc < 3){
        printf("Not enough parameters\n");
        exit(1);
    }

    int i, min, num_in_files = argc - 2;    //first one is the name of the program, last one is the out file
    int **ordered_arrays, *max, *taken;
    int n_total, fd;
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

    //allocate the array of arguments for the threads
    args_array = (thread_arg_t *)malloc(num_in_files * sizeof(thread_arg_t));
    //init the arguments for each thread and create it
    for(i=0; i<num_in_files; i++){
        args_array[i].id = i;
        args_array[i].filePath = in_files[i];
        pthread_create(&args_array[i].tid, NULL, threadFunc, (void *)&args_array[i]);
    }    

    //wait for all thread to finish
    for(i=0; i<num_in_files; i++){
        pthread_join(args_array[i].tid, NULL);
        if(DEBUG_PRINT){
            pthread_mutex_lock(&mutex_to_print);
            printf("\nT%d finished", args_array[i].id);
            pthread_mutex_unlock(&mutex_to_print);
        }
    }

    //open output file
    fd = open(out_file, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO);
    if(fd < 0){
        printf("Error  in main opening the output file\n");
        exit(1);
    }

    //compute the total number of elements in the output file
    n_total = 0;
    for(i=0; i<num_in_files; i++)
        n_total += args_array[i].n_elem;

    //alloc and erase the array of counters
    taken = (int *)calloc(num_in_files, sizeof(int));
    
    ordered_arrays = (int **)malloc(num_in_files * sizeof(int *));  //alloc and init the array of ordered arrays
    max = (int *)malloc(num_in_files * sizeof(int ));   //alloc the array containing size of the arrays
    for(i=0; i<num_in_files; i++){
        ordered_arrays[i] = args_array[i].elements;
        max[i] = args_array[i].n_elem;
    }

    //merge all files
    if(write(fd, &n_total, sizeof(int)) < 0){
        printf("Error in main writing total size\n");
        exit(1);
    }
    for(i=0; i<n_total; i++){
        min = getMin(num_in_files, taken, max, ordered_arrays);
        if(write(fd, &min, sizeof(int)) < 0){
            printf("Error in main merging the file\n");
            exit(1);
        }
    }

    //close the output file
    close(fd);

    if(DEBUG_PRINT_3){
        fd = open(out_file, O_RDONLY);
        if(fd < 0){
            printf("Error in main opening merged file for printing\n");
            exit(1);
        }
        printf("\nAll merged files :\n");
        for(i=0; i<n_total; i++){
            if(read(fd, &min, sizeof(int)) < 0){
                printf("Error in main reading size of merged files\n");
                exit(1);
            }
            if(read(fd, &min, sizeof(int)) < 0){
                printf("Error in main printing merged file\n");
                exit(1);
            }
            printf("%d ", min);
        }
        printf("(%d elements)\n", n_total);
        close(fd);
    }

    return 0;
}

void *threadFunc(void *arg){
    thread_arg_t *myArg = (thread_arg_t *)arg;
    int fd, i;
    struct stat st;

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
/*
    //after having allocated and get the content of the file, it can be unmapped
    if(munmap(myArg->file_content, st.st_size) < 0){
        printf("T%d failed unmapping the file\n", myArg->id);
        pthread_exit(NULL);
    }
*/
    //close the file
    close(fd);

    if(DEBUG_PRINT_1){
        pthread_mutex_lock(&mutex_to_print);
        printf("\nT%d file:\n", myArg->id);
        for(i=0; i<myArg->n_elem; i++)
            printf("%d ", myArg->elements[i]);
        printf("\n\n");
        pthread_mutex_unlock(&mutex_to_print);
    }

    //order the array using the bubble sort algorithm
    int max, pos_max, j;
    for(i=0; i<myArg->n_elem; i++){
        max = -1;
        for(j=0; j<myArg->n_elem-i; j++)
            if(myArg->elements[j] > max){
                max = myArg->elements[j];
                pos_max = j;
            }
        myArg->elements[pos_max] = myArg->elements[myArg->n_elem - 1 - i]; 
        myArg->elements[myArg->n_elem - 1 - i] = max;
    }

    if(DEBUG_PRINT_2){
        pthread_mutex_lock(&mutex_to_print);
        printf("\nT%d ordered file:\n", myArg->id);
        for(i=0; i<myArg->n_elem; i++)
            printf("%d ", myArg->elements[i]);
        printf("\n");
        pthread_mutex_unlock(&mutex_to_print);
    }

    pthread_exit(NULL);
}

int getMin(int num, int *taken, int *max, int **arrays){
    int i, min = 0xFFFFFF, arr_min;
    for(i=0; i<num; i++){
        if(taken[i] < max[i] && arrays[i][taken[i]] < min){
            min = arrays[i][taken[i]];
            arr_min = i;
        }
    }

    if(DEBUG_PRINT_4) printf("min: %d\n", min);
    taken[arr_min]++;
    return min;
}



