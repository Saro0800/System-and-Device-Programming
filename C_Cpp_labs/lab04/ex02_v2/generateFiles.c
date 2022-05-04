#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>

#define DEBUG_PRINT 0

typedef struct thread_arg_s{
    int id;
    pthread_t TID;
    char *filePath;     
}thread_arg_t;

void *generateFile(void *);

int main(int argc, char *argv[]){

    setbuf(stdout, 0);

    //check for parameter
    if(argc < 2){
        printf("Missing parameter\n");
        exit(1);
    }

    int n_files = atoi(argv[1]), fd, tmp;
    char *tempPath;
    thread_arg_t *argsArray;

    printf("Generation of %d file requested\n", n_files);

    //allocate the array and generate all the threads
    argsArray = (thread_arg_t *)malloc(n_files * sizeof(thread_arg_t));
    for(int i=0; i<n_files; i++){
        argsArray[i].id = i;
        tempPath = (char *)malloc(15 * sizeof(char));
        sprintf(tempPath, "./tmp/file%d.bin", i+1);
        argsArray[i].filePath = tempPath;
        pthread_create(&argsArray[i].TID, NULL, generateFile, (void *)&argsArray[i]);
    }

    //wait for the creation of all files
    for(int i=0; i<n_files; i++){
        pthread_join(argsArray[i].TID, NULL);
        if(DEBUG_PRINT) printf("%s has been successfully created\n",argsArray[i].filePath);
    }

    //print all files for debug purpouse
    if(DEBUG_PRINT){
        for(int i=0; i<n_files; i++){
            printf("\n%s:\n", argsArray[i].filePath);
            fd = open(argsArray[i].filePath, O_RDONLY);
            while(read(fd, &tmp, sizeof(int)) != 0)
                printf("%d ", tmp);
            printf("\n");
        }
    }

    printf("All %d files created\n", n_files);

    return 0;
}

void *generateFile(void *arg){
    //retrieve argument
    thread_arg_t *myArg = (thread_arg_t *)arg;
    unsigned int seed = myArg->TID;                 //create seed to randomly generate all numbers
    unsigned int n_elem = rand_r(&seed) % 100000 + 1;    //generate the number of elements
    int fd;     //file descriptor
    int nw, tmp;

    fd = open(myArg->filePath, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO);
    if(fd < 0){
        printf("T:%d failed creating file %s\n", myArg->id, myArg->filePath);
        pthread_exit(NULL);
    }
    
    if(DEBUG_PRINT) printf("T:%d successfully created file %s\n", myArg->id, myArg->filePath);

    if((nw = write(fd, (const void*)&n_elem, sizeof(int))) != sizeof(int)){
        printf("T:%d failed writing on file the number of elements\n", myArg->id);
        pthread_exit(NULL);
    }
    
    for(int i=0; i<n_elem; i++){
        tmp = rand_r(&seed) % 1000 + 1;
        if((nw = write(fd, &tmp, sizeof(int))) != sizeof(int)){
            printf("T:%d failed writing on file a number\n", myArg->id);
            pthread_exit(NULL);
        }
    }

    close(fd);

    pthread_exit(NULL);
}








