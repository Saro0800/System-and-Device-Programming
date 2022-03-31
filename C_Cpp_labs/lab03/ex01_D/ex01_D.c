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

struct rwStruct{        //structure for all the I/O operations
    int childID;
    unsigned int len;
    char *string;
};

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

    struct aiocb aiocbS;        //define the control block
    struct sigevent sigevS;     //struct to define how to be notified
    int bytesRead;

    //set fileds of the sigevent structure
    sigevS.sigev_notify = SIGEV_SIGNAL;     //notification trough signal
    sigevS.sigev_signo = SIGUSR1;

    //request all asynchronous read
    for(int i=0; i<STR_NUM; i++){
        //set the aiocb for reading pipe between child 1
        aiocbS.aio_fildes = pipesF2C[0][0];
        aiocbS.aio_offset = bytesRead;
    }

    

	wait(NULL);
	wait(NULL);
}

//function designed for the child
void childFunct(int *pipeF_C, int timeToWait){

	int j, bytesWritten=0;
	unsigned int seed = timeToWait*getpid();
    struct rwStruct rwS;    //structure for reading/writing
    struct aiocb aiocbS;    //structure for asynchronous I/O
    struct sigevent sigevS; //structure to notify the completion of AIO

    //set fields for the sigevent structre
    sigevS.sigev_notify = SIGEV_NONE;   //do not send a notification when the write
                                        //a write operation i completed. All other fields are ignored 

    //set the fields of the AIO control block that remain always the same
    aiocbS.aio_fildes = pipeF_C[1];     //specify the file descriptor
    aiocbS.aio_reqprio = 0;             //set a priority
    aiocbS.aio_sigevent = sigevS;       //specify how to be notified when the write is completed   
    aiocbS.aio_lio_opcode = 0;

    //set the child identifier
    rwS.childID = timeToWait == WAIT_TIME_1 ? 1 : 2;

	//close reading terminal of the pipe
	close(pipeF_C[0]);
	
	//generate STR_NUM strings and write them on the pipe 
	for(int i=0; i<STR_NUM; i++){
		sleep(timeToWait);		//sleep
		
		rwS.len = rand_r(&seed)%STR_SIZE + 1;                   //randomize string lenght

		rwS.string = (char *)malloc((rwS.len+1) * sizeof(char));//allocate the string
	
		for(j=0; j<rwS.len; j++)                                //generate a random string
			rwS.string[j] = rand_r(&seed)%26 + 'a';
		rwS.string[j] = '\0';
	
        //set all fields of the aiocb
        aiocbS.aio_offset = bytesWritten;               //set starting point equal to last position written
        aiocbS.aio_buf = &rwS;                          //write len and string together
        aiocbS.aio_nbytes = sizeof(struct rwStruct);    //set size of the buff
        
        //request an AIO write
        if(aio_write(&aiocbS) < 0){
            printf("Error in a child while requesting an AIO write\n");
            exit(1);
        }      
		
        bytesWritten += sizeof(struct rwStruct);        //increment the number of bytes written, so the offset
	
        printf("Child %d generated: %s\n", rwS.childID, rwS.string);
    }

	exit(0);	//after having creted all the strings, terminate the process
}
