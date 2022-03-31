#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>
#include <aio.h>
#include <signal.h>

#define WAIT_TIME_1 1
#define WAIT_TIME_2 3
#define STR_NUM 5
#define STR_SIZE 32
#define CHLD_NUM 2

struct rwStruct{            //structure for all the I/O operations
    unsigned int len;
    char *string;
};

struct rwStruct rC1[STR_NUM];       //buff for reading messages from C1
unsigned int indexC1 = 0;           //index to access rC1
struct rwStruct rC2[STR_NUM];       //buff for reading messages from C2
unsigned int indexC2 = 0;           //index to access rC2

void fatherFunct(int pipesF2C[][2]);
void childFunct(int *pipeF_C, int timeToWait);
void printString(int SIGNUM);


int main(int argc, char *argv[]){

	int pipesF2C[2][2];
	pid_t pid;
	struct tm timeS;
	time_t T;

	T = time(NULL);
	timeS = *localtime(&T);

	setbuf(stdout, NULL);
	printf("starting at [%02d:%02d:%02d]\n", timeS.tm_hour, timeS.tm_min, timeS.tm_sec);
	
	//the father creates 2 childs
	for(int i=0; i<2; i++){	
		//father creates the pipe
		if(pipe(pipesF2C[i]) != 0){
			fprintf(stderr, "Error creating pipe number %d\n", i+1);
			exit(1);
		}
		
		pid = fork();			//create the new process
		
		if(pid==0 && i==0)		//child 1
			childFunct(pipesF2C[i], WAIT_TIME_1);
		else if(pid==0 && i==1)	//child 2
			childFunct(pipesF2C[i], WAIT_TIME_2);

		//the father will only read from the pipe, the child will only
		//write on it. So the father can close the write terminals
		close(pipesF2C[i][1]);
	}
	
	fatherFunct(pipesF2C);	//enter the function designed for the father

	return 0;

}

//function designed for the father
void fatherFunct(int pipesF2C[][2]){

    struct aiocb *aiocbSC1, *aiocbSC2;        //arrays of control blocks
    struct sigevent sigevS;     //how to be de notified
    int bytesRead;              //offset 

    //instantiate the signal handler
    signal(SIGUSR1, printString);
    signal(SIGUSR2, printString);

    //allocate arrays
    if((aiocbSC1 = (struct aiocb *)malloc(STR_NUM * sizeof(struct aiocb))) == NULL){
        printf("Error allocationg aiocbS1\n");
        exit(1);
    }
    if((aiocbSC2 = (struct aiocb *)malloc(STR_NUM * sizeof(struct aiocb))) == NULL){
        printf("Error allocationg aiocbS2\n");
        exit(1);
    }

    //request all asynchronous read
    for(int i=0; i<STR_NUM; i++){
        //set sigevS fields
        sigevS.sigev_notify = SIGEV_SIGNAL;
        sigevS.sigev_signo = SIGUSR1;
        
        //set the aiocb for reading pipe between child 1
        aiocbSC1[i].aio_fildes = pipesF2C[0][0];
        aiocbSC1[i].aio_offset = bytesRead;
        aiocbSC1[i].aio_buf = &(rC1[i]);
        aiocbSC1[i].aio_nbytes = sizeof(struct rwStruct);
        aiocbSC1[i].aio_sigevent = sigevS;
        
        //request read
        if(aio_read(&aiocbSC1[i]) < 0){
            printf("i: %d\tError requesting read for pipe 1\n", i);
            exit(1);
        }

        //set sigevS fields
        sigevS.sigev_notify = SIGEV_SIGNAL;
        sigevS.sigev_signo = SIGUSR2;
        
        //set the aiocb for reading pipe between child 2
        aiocbSC2[i].aio_fildes = pipesF2C[1][0];
        aiocbSC2[i].aio_offset = bytesRead;
        aiocbSC2[i].aio_buf = &rC1[i];
        aiocbSC2[i].aio_nbytes = sizeof(struct rwStruct);
        aiocbSC2[i].aio_sigevent = sigevS;

        //request read
        if(aio_read(&aiocbSC2[i]) < 0){
            printf("Error requesting read for pipe 2\n");
            exit(1);
        }

        //update the offset
        bytesRead += sizeof(struct rwStruct);
    }

	wait(NULL);
	wait(NULL);
}

//function designed for the child
void childFunct(int *pipeF_C, int timeToWait){

	int j, bytesWritten=0;
	unsigned int seed = timeToWait*getpid();
    struct rwStruct *rwS;    //structure for reading/writing
    struct aiocb *aiocbS;    //structure for asynchronous I/O
    struct sigevent sigevS; //structure to notify the completion of AIO

    //allocate the array of rwStruct
    rwS = malloc(STR_NUM * sizeof(struct rwStruct));

    //allocate the array of aiocbS
    aiocbS = malloc(STR_NUM * sizeof(struct aiocb));


    //set fields for the sigevent structre
    sigevS.sigev_notify = SIGEV_NONE;   //do not send a notification when the write
                                        //a write operation i completed. All other fields are ignored 
    
    //close reading terminal of the pipe
	close(pipeF_C[0]);
	
	//generate STR_NUM strings and write them on the pipe 
	for(int i=0; i<STR_NUM; i++){
		sleep(timeToWait);		//sleep
		
		rwS[i].len = rand_r(&seed)%STR_SIZE + 1;                   //randomize string lenght

		rwS[i].string = (char *)malloc((rwS[i].len+1) * sizeof(char));//allocate the string
	
		for(j=0; j<rwS[i].len; j++)                                //generate a random string
			rwS[i].string[j] = rand_r(&seed)%26 + 'a';
		rwS[i].string[j] = '\0';
	
        //set all fields of the aiocb
        aiocbS[i].aio_fildes = pipeF_C[1];                 //specify the file descriptor
        aiocbS[i].aio_reqprio = 0;                         //set a priority
        aiocbS[i].aio_sigevent = sigevS;                   //specify how to be notified when the write is completed   
        aiocbS[i].aio_lio_opcode = 0;
        aiocbS[i].aio_offset = bytesWritten;               //set starting point equal to last position written
        aiocbS[i].aio_buf = &rwS[i];                          //write len and string together
        aiocbS[i].aio_nbytes = sizeof(struct rwStruct);    //set size of the buff
      
        //request an AIO write
        if(aio_write(&aiocbS[i]) < 0){
            printf("Error in a child while requesting an AIO write\n");
            exit(1);
        }      
	
        bytesWritten += sizeof(struct rwStruct);        //increment the number of bytes written, so the offset
	
        //printf("Child %d generated: %s\n", getpid(), rwS[i].string);
    }

	exit(0);	//after having creted all the strings, terminate the process
}

//signal handler
void printString(int SIGNUM){
    struct tm timeS;
    time_t T;

    T = time(NULL);
    timeS = *localtime(&T);

    if(SIGNUM == SIGUSR1){
        printf("[%02d:%02d:%02d] len=%d\tChild 1: %s\n",timeS.tm_hour, timeS.tm_min,
                                                        timeS.tm_sec, rC1[indexC1].len, 
                                                        rC1[indexC1].string);
        indexC1++;
    }
    else if(SIGNUM == SIGUSR2){
        printf("[%02d:%02d:%02d] len=%d\tChild 1: %s\n",timeS.tm_hour, timeS.tm_min,
                                                        timeS.tm_sec, rC1[indexC2].len, 
                                                        rC1[indexC2].string);
        indexC2++;
    }
    else{
        printf("Father received an unexpected signal\n");
        exit(1);
    }

}