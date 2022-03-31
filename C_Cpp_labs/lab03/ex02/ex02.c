//ciascun thread ha il suo fd per il file e il suo mapping del file.
//devo acquisire la dimensione effettive del file.
//in ogni thread leggo una riga alla volta (tanto conosco la formattazione delle righe)
//fino a quando non raggiungo una dimensione di byte letti uguale alla dimensione del file.
//alla fine di ogni ciclo devo aggiornare il numero di byte letti

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

struct thread_arg{      //struct to be passed to each thread
    int id;                 //id assigned by the creator
    pthread_t TID;          //id assigned by the OS
    int fd;                 //file to read from
    unsigned int fileSize;  //size of the file
};

#define MAX_LENGTH 30

struct records_struct{  //struct for each line of the file
	int id;                 //line identifier
	long regNumber;         //identifier of each person
	char surn[MAX_LENGTH];  //suranme
    char name[MAX_LENGTH];  //name
	int mark;               //mark
};

void *threadFunct(void *arg);

int main(int argc, char *argv[]){

    struct thread_arg threadArgs[2];        //array with threads' arguments
    char *filePath;
    int fd;
    struct stat sb;     //struct to retrieve all information of the file
    void *status;

    if(argc < 2){
        printf("Missing parameter\n");
        exit(1);
    }

    filePath = argv[1];             //get the path of the input file
    if((fd = open(filePath, O_RDWR)) < 0){      //open the file for reading and writing
        printf("Error opening file %s\n", filePath);
        exit(1);
    }    

    if(stat(filePath, &sb) < 0){    //retrieve information about the file
        printf("Error retrieving information about %s\n", filePath);
        exit(1);
    }

    for(int i=0; i<2; i++){         //set the argument for each thread and crate it
        threadArgs[i].id = i+1;
        threadArgs[i].fd = fd;
        threadArgs[i].fileSize = sb.st_size;

        pthread_create(&threadArgs[i].TID, NULL, threadFunct, (void *)&threadArgs[i]);
    }

    pthread_join(threadArgs[0].TID, &status);   //wait the termination of the first thread
    pthread_join(threadArgs[1].TID, &status);   //wait the termination of the second thread

    if(close(fd) < 0){      //close the file
        printf("Error closing %s\n", filePath);
        exit(1);
    }

    return 0;

}

void *threadFunct(void *arg){
    struct thread_arg *myArg = (struct thread_arg *) arg;
    struct records_struct *record;
    int bytesDone = 0;
    size_t len = sizeof(struct records_struct);
    off_t off;

    printf("fileSize: %d\n", myArg->fileSize);

    //loop untill we analized the entire file
    while(bytesDone < myArg->fileSize){
        if(myArg->id == 1)          //first thread start from the top of the file
            off = bytesDone;
        else if(myArg->id == 2)     //second thread start from the bottm of the file
            off = myArg->fileSize - bytesDone - len;

        printf("id: %d Offset %ld\n\n", myArg->id, off);

        //map the file
        if((record = mmap(0, len, PROT_READ | PROT_WRITE, MAP_SHARED, myArg->fd, off)) == MAP_FAILED){
            printf("Failed mapping the file in T%d\n", myArg->id);
            exit(1);
        }

        //manipulate the file
        if(myArg->id == 1){
            printf("id:%d  regNumber: %ld before\n", myArg->id, record->regNumber);
            record->regNumber++;
            printf("id:%d  regNumber: %ld after\n", myArg->id, record->regNumber);
        }
        else if(myArg->id == 2){
            printf("id:%d  mark: %d before\n", myArg->id, record->mark);
            record->mark--;
            printf("id:%d  mark: %d after\n", myArg->id, record->mark);
        }

        //unmap the file region
        if(munmap(record, len) < 0){
            printf("Error unmapping the file in T%d\n", myArg->id);
            exit(1);
        }

        bytesDone += len;       //update the amount of bytesProcessed
    }

    pthread_exit(NULL);
}




