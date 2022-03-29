#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/select.h>

#define WAIT_TIME_1 8
#define WAIT_TIME_2 3
#define STR_NUM 5
#define STR_SIZE 32

void fatherFunct(int pipesF2C[][2]);
void child1Funct(int *pipeF_C1);
void child2Funct(int *pipeF_C2);


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
			child1Funct(pipesF2C[i]);
		else if(pid==0 && i==1)	//child 2
			child2Funct(pipesF2C[i]);

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
	struct tm timeS;
	time_t T;
    fd_set fdsetRead;
    struct timeval timevalS;

    //set the amount of time to wait before going to the next descriptor
    timevalS.tv_sec = 1;
    timevalS.tv_usec = 0;

    

    maxfdp1 = pipesF2C[0][0]>pipesF2C[1][0] ? pipesF2C[0][0] : pipesF2C[1][0];
    maxfdp1 += 1;

	for(j=0; j<2*STR_NUM; ){
        //clear fdset
        FD_ZERO(&fdsetRead);

        //set multiplexing for file descriptors to read from
        FD_SET(pipesF2C[0][0], &fdsetRead);
        FD_SET(pipesF2C[1][0], &fdsetRead);

        //check if at least one terminal is ready to be read
        if((ready = select(maxfdp1, &fdsetRead, NULL, NULL, &timevalS)) < 0)
            printf("Error select\n");
        else if(ready > 0){
            j++;
            for(i=0; i<2; i++)
                if(FD_ISSET(pipesF2C[i][0], &fdsetRead)){
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

                    free(string);
                    break;
                }
        }



    }

	wait(NULL);
	wait(NULL);
}

//function designed for the child 1
void child1Funct(int *pipeF_C1){

	char *string;
	int len, j;
	unsigned int seed = WAIT_TIME_1;

	//close reading terminal of the pipe
	close(pipeF_C1[0]);
	
	//generate STR_NUM strings and write them on the pipe 
	for(int i=0; i<STR_NUM; i++){
		sleep(WAIT_TIME_1);		//sleep
		
		//randomize string lenght
		len = rand_r(&seed)%STR_SIZE + 1;
		//printf("Child 1 n=%d\n", len);
		
		//write len on the pipe
		if(write(pipeF_C1[1], &len, sizeof(int)) != sizeof(int)){
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
		if(write(pipeF_C1[1], string, len+1) < len+1){
			fprintf(stderr, "Error in Child 1 writing on the pipe\n");
			exit(1);
		}
	
		//printf("Child 1 generated: %s\n", string);
	}

	exit(0);	//after having creted all the strings, terminate the process
}

//function designed for the child 2
void child2Funct(int *pipeF_C2){

	char *string;
	int len, j;
	unsigned int seed = WAIT_TIME_2;

	//close reading terminal of the pipe
	close(pipeF_C2[0]);
	
	//generate STR_NUM strings and write them on the pipe 
	for(int i=0; i<STR_NUM; i++){
		sleep(WAIT_TIME_2);		//sleep
		
		//randomize string lenght
		len = rand_r(&seed)%STR_SIZE + 1;
		//printf("Child 2 n=%d\n", len);

		if(write(pipeF_C2[1], &len, sizeof(int)) != sizeof(int)){
			fprintf(stderr, "Error in Child 2 writing the length on the pipe\n");
			exit(1);
		}

		//allocate the string
		string = (char *)malloc((len+1) * sizeof(char));
		
		//generate a random string
		for(j=0; j<len; j++)	
			string[j] = rand_r(&seed)%26 + 'a';
		string[j] = '\0';
		
		//write the string on the pipe
		if(write(pipeF_C2[1], string, len+1) < len+1){
			fprintf(stderr, "Error in Child 2 writing on the pipe\n");
			exit(1);
		}
	
		//printf("Child 2 generated: %s\n", string);
	}

	exit(0);	//after having creted all the strings, terminate the process
}










