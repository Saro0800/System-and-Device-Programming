#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/select.h>
#include <string.h>

#define WAIT_TIME_1 1
#define WAIT_TIME_2 3
#define STR_NUM 5
#define STR_SIZE 32
#define CHLD_NUM 2

void fatherFunct(int pipesF2C[][2]);
void childFunct(int *pipeF_C, int timeToWait);


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

	char *string;
	int bytesRead, i, j, k, len, maxfdp1, ready;
	int childIsTerminated[CHLD_NUM];
	struct tm timeS;
	time_t T;
    fd_set fdsetRead;
    struct timeval timevalS;

    //set the amount of time to wait before going to the next descriptor
    timevalS.tv_sec = timevalS.tv_usec = 0;

	//clear flags for child termination
	for(i=0; i<CHLD_NUM; i++)
		childIsTerminated[i] = 0;
    
	//determine the maximum between all fds
    maxfdp1 = pipesF2C[0][0]>pipesF2C[1][0] ? pipesF2C[0][0] : pipesF2C[1][0];
    maxfdp1 += 1;

	for(j=0; j<2*STR_NUM; ){
        //clear fdset
        FD_ZERO(&fdsetRead);

        //set multiplexing for file descriptors to read from
        if(childIsTerminated[0] != 1)	FD_SET(pipesF2C[0][0], &fdsetRead);
        if(childIsTerminated[1] != 1)	FD_SET(pipesF2C[1][0], &fdsetRead);

        //check if at least one terminal is ready to be read
        if((ready = select(maxfdp1, &fdsetRead, NULL, NULL, &timevalS)) < 0)
            printf("Error select\n");
        else if(ready > 0){
            for(i=0; i<2; i++)
                if(FD_ISSET(pipesF2C[i][0], &fdsetRead)){
					j++;
                    //if the pipe is full, read from it the size of the string
                    if(read(pipesF2C[i][0], &len, sizeof(int)) != sizeof(int)){
                        fprintf(stderr, "Error in Father reading the length from the pipe %d\n", i+1);
                        //exit(1);
                    }

                    //allocate the string
                    string = (char *)malloc((len+1) * sizeof(char));
                    
                    //if the pipe is full, read from it the string
                    if(( bytesRead = read( pipesF2C[i][0], string, (len+1)*sizeof(char) )) < (len+1) ){
                        fprintf(stderr, "Error in Father reading the string from the pipe %d\n", i+1);
                        //exit(1);
                    }

					//check if it is equal to the termination string
					if(strcmp(string, "#")==0){
						childIsTerminated[i] = 1;	//set the flag
						j--;	//don't count this string
					}
					else{
						//convert to upper case
						for(k=0; k<len; k++)
							string[k] =  toupper(string[k]);
						
						T = time(NULL);
						timeS = *localtime(&T);

						//print on the screen
						printf("[%02d:%02d:%02d] len=%d\tChild %d: ",timeS.tm_hour, timeS.tm_min,
																timeS.tm_sec, len, i+1);
						if(write(STDOUT_FILENO, string, len+1) < len+1){
							fprintf(stderr, "Error in Father writing on screen\n");
							exit(1);
						}
						printf("\n");

					}
                    
                    free(string);
                    //break;
                }
        }
    }

	wait(NULL);
	wait(NULL);
}

//function designed for the child
void childFunct(int *pipeF_C, int timeToWait){

	char *string;
	int len, j;
	unsigned int seed = timeToWait*getpid();

	//close reading terminal of the pipe
	close(pipeF_C[0]);
	
	//generate STR_NUM strings and write them on the pipe 
	for(int i=0; i<STR_NUM; i++){
		sleep(timeToWait);		//sleep
		
		//randomize string lenght
		len = rand_r(&seed)%STR_SIZE + 1;
		//printf("Child 1 n=%d\n", len);
		
		//write len on the pipe
		if(write(pipeF_C[1], &len, sizeof(int)) != sizeof(int)){
			fprintf(stderr, "Error in Child 1 writing the length on the pipe\n");
			exit(1);
		}

		//allocate the string
		string = (char *)malloc((len+1) * sizeof(char));
		
		//generate a random string
		for(j=0; j<len; j++)	
			string[j] = rand_r(&seed)%26 + 'a';
		string[j] = '\0';
		
		//write the string on the pipe
		if(write(pipeF_C[1], string, len+1) < len+1){
			fprintf(stderr, "Error in Child 1 writing on the pipe\n");
			exit(1);
		}
	
		//printf("Child 1 generated: %s\n", string);
	}

	//before closing the write descriptor, the child has to signal this situation to the father,
	//otherwise the descriptor will be considered always ready.
	//In order to signal the termination of the child, it writes on the pipe a single special char.
	len = 1;	//set len=1
	if(write(pipeF_C[1], &len, sizeof(int)) != sizeof(int)){	//write len on the pipe
		fprintf(stderr, "Error in Child 1 writing the length on the pipe\n");
		exit(1);
	}
	string = (char *)malloc(2*sizeof(char));	//allocate the string
	string[0] = '#';
	string[1] = '\0';
	if(write(pipeF_C[1], string, len+1) < len+1){	//write the string on the pipe
		fprintf(stderr, "Error in Child 1 writing on the pipe\n");
		exit(1);
	}

	exit(0);	//after having creted all the strings, terminate the process
}
