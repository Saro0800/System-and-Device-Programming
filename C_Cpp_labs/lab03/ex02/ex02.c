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

typedef struct thread_args_s{      //struct to be passed to each thread
    int id;                 //id assigned by the creator
    pthread_t TID;          //id assigned by the OS
    void *src;              //portion of the memory to be read
    size_t fileSize;  //size of the file
}thread_args_t;

#define MAX_LENGTH 30

typedef struct{  //struct for each line of the file
	int id;                 //line identifier
	long regNumber;         //identifier of each person
	char surn[MAX_LENGTH];  //suranme
    char name[MAX_LENGTH];  //name
	int mark;               //mark
}student_t;

void *threadFunct(void *arg);
void printBinFile(char *file);

int main(int argc, char *argv[]){

    thread_args_t threadArgs[2];        //array with threads' arguments
    char *filePath;
    int fd;
    struct stat sb;     //struct to retrieve all information of the file
    void *status, *src;

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

    printBinFile("orig_file");

    //before creating the threads we must map the file in memory.
    //we map the entire file. We cast the pointer returned from mmap into a student_t pointer.
    //In this way we can deal with that region of memory as an array of elements of type
    //student_t.
    src = (student_t *) mmap(0, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (src == MAP_FAILED) {
        fprintf(stderr, "Error in mmpap\n");
        exit(0);
    }

    for(int i=0; i<2; i++){         //set the argument for each thread and crate it
        threadArgs[i].id = i+1;
        threadArgs[i].src = src;
        threadArgs[i].fileSize = sb.st_size;

        pthread_create(&threadArgs[i].TID, NULL, threadFunct, (void *)&threadArgs[i]);
    }

    pthread_join(threadArgs[0].TID, &status);   //wait the termination of the first thread
    pthread_join(threadArgs[1].TID, &status);   //wait the termination of the second thread

    if(close(fd) < 0){      //close the file
        printf("Error closing %s\n", filePath);
        exit(1);
    }

    //unmap the region
    munmap(src, sb.st_size);

    //print the file
    //printBinFile(filePath);

    return 0;

}

void *threadFunct(void *arg){
    thread_args_t *myArg = (thread_args_t *) arg;
    student_t *student_d;
    int n, nrecord, up_down;

    //since we already mapped the file in memory, we can handle it as an array
    student_d = (student_t *)myArg->src;
    nrecord = myArg->fileSize / sizeof(student_t);
    up_down = myArg->id == 1 ? 1 : 0;

    if(up_down == 1)   //first thread reads and writes top down
        for(n=0; n<nrecord; n++){
            student_d->regNumber++;     //modify regNumber
            student_d++;        //move the pointer to the next element in array
        }
    else if(up_down == 0){  //second thread reads bottom up
        student_d = student_d + nrecord - 1;    //since it reads bottom up, we start from the
                                                //last element of the array
        for(n=0; n<nrecord; n++){
            student_d->mark--;  //modify mark
            student_d--;        //move the pointer to the previous element in array
        }
    }

    pthread_exit(NULL);
}

void printBinFile(char *file){
    student_t stud;
    int fd;

    if((fd = open(file, O_RDONLY)) < 0){
        fprintf(stderr, "Error trying to read the file %s\n", file);
        exit(1);
    }

    while(read(fd, &stud, sizeof(student_t)) > 0){
        fprintf(stdout, "%d %ld %s %s %d\n", stud.id, stud.regNumber, stud.name, stud.surn, stud.mark);
    }

    if(close(fd) < 0){
        fprintf(stderr, "Error trying to close the file %s\n", file);
        exit(1);
    }
}



