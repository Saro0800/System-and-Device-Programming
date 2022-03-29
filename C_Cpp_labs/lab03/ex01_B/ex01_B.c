#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <time.h>

#define WAIT_TIME_1 3
#define WAIT_TIME_2 8
#define STR_NUM 5
#define STR_SIZE 32

void fatherFunct(int pipesF2C[][2]);
void child1Funct(int *pipeF_C1);
void child2Funct(int *pipeF_C2);
void set_fcntl(int fd, int flag);

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
	int i, j, k, len[2], bytesRead[2], haveToReadLen[2];
    struct tm timeS;
    time_t T;

    haveToReadLen[0] = haveToReadLen[1] = 1;
    bytesRead[0] = bytesRead[1] = 0;

    //modify the flag associated to read terminals to set the file NONBLOCKING
    set_fcntl(pipesF2C[0][0], O_NONBLOCK);  //set reading-termial form C1 non blocking
    set_fcntl(pipesF2C[1][0], O_NONBLOCK);  //set reading-termial form C2 non blocking

    //loop untile STR_NUM strings are read
	for(j=0; j<2*STR_NUM; )
		for(i=0; i<2; i++){
			//if it has to read len, try to read it from the pipe
			if(haveToReadLen[i]==1 && (bytesRead[i] = read(pipesF2C[i][0], &len[i], sizeof(int))) <= 0)
                //it didn't read anything
				//fprintf(stderr, "Error in Father reading the length from the pipe %d\n", i+1);
				haveToReadLen[i] = 1;
            else if(bytesRead[i]>0){     
                //enter here only if it had to read and effectivelly read len
                haveToReadLen[i] = 0;   //it already read len, so it doen't have to do it again until
                                        //the string is read
                
                //allocate the string
                string = (char *)malloc((len[i]+1) * sizeof(char));

                //try read the string from the pipe
                if(read( pipesF2C[i][0], string, (len[i]+1)*sizeof(char)) <= 0 )
                    //it didn't read anything
                    fprintf(stderr, "Error in Father reading the string from the pipe %d\n", i+1);
                else{
                    //it read a string. It can read a new len
                    bytesRead[i] = 0;       
                    haveToReadLen[i] = 1;
                    
                    //update the counter
                    j++;
                    
                    //convert to upper case
                    for(k=0; k<len[i]; k++)
                        string[k] =  toupper(string[k]);

                    T = time(NULL);
                    timeS = *localtime(&T);
                    //print on the screen
                    printf("[%02d:%02d:%02d] len=%d\tChild %d: ",timeS.tm_hour, timeS.tm_min,
                                                             timeS.tm_sec, len[i], i+1);
                    if(write(STDOUT_FILENO, string, len[i]+1) < len[i]+1){
                        fprintf(stderr, "Error in Father writing on screen\n");
                        exit(1);
                    }
                    printf("\n");
                }

                free(string);

            }

		}

	wait(NULL);
	wait(NULL);
}

//function designed for the child 1
void child1Funct(int *pipeF_C1){

	char *string;
	int len, j;
	unsigned int seed = WAIT_TIME_1*WAIT_TIME_1;

	//close reading terminal of the pipe
	close(pipeF_C1[0]);

    //set writing terminal non blocking
    set_fcntl(pipeF_C1[1], O_NONBLOCK);
	
	//generate STR_NUM strings and write them on the pipe 
	for(int i=0; i<STR_NUM;){
        //sleep
		sleep(WAIT_TIME_1);
		
		//randomize string length
		len = rand_r(&seed)%STR_SIZE + 1;

		//allocate the string
		string = (char *)malloc((len+1) * sizeof(char));
		
		//generate a random string
		for(j=0; j<len; j++)	
			string[j] = rand_r(&seed)%26 + 'a';
		string[j] = '\0';

        //tries to write both len and string on the pipe
		if(write(pipeF_C1[1], &len, sizeof(int))<0 || write(pipeF_C1[1], string, len+1)<0)
			fprintf(stderr, "Error in Child 1 writing 'len' or 'string' on the pipe\n");
		else
        //since the write is non-blocking, we have to update the counter only
        //if a string has been written, that is if both the previous writes have succeeded
            i++;
	
		//printf("Child 1 generated: %s\n", string);
	}

	exit(0);	//after having written all the strings, terminate the process
}

//function designed for the child 2
void child2Funct(int *pipeF_C2){

	char *string;
	int len, j;
	unsigned int seed = WAIT_TIME_2*WAIT_TIME_2;

	//close reading terminal of the pipe
	close(pipeF_C2[0]);

    //set writing terminal non blocking
    set_fcntl(pipeF_C2[1], O_NONBLOCK);
	
	//generate STR_NUM strings and write them on the pipe 
	for(int i=0; i<STR_NUM;){
        //sleep
		sleep(WAIT_TIME_2);
		
		//randomize string length
		len = rand_r(&seed)%STR_SIZE + 1;

		//allocate the string
		string = (char *)malloc((len+1) * sizeof(char));
		
		//generate a random string
		for(j=0; j<len; j++)	
			string[j] = rand_r(&seed)%26 + 'a';
		string[j] = '\0';

        //tries to write both len and string on the pipe
		if(write(pipeF_C2[1], &len, sizeof(int))<0 || write(pipeF_C2[1], string, len+1)<0)
			fprintf(stderr, "Error in Child 2 writing 'len' or 'string' on the pipe\n");
		else
        //since the write is non-blocking, we have to update the counter only
        //if a string has been written, that is if both the previous writes have succeeded
            i++;
	
		//printf("Child 2 generated: %s\n", string);
	}

	exit(0);	//after having written all the strings, terminate the process
}

//set desired flag for the specified file
void set_fcntl(int fd, int flag){
    int val;

    //retrieve current flags
    if((val = fcntl(fd, F_GETFL, 0)) == -1){
        fprintf(stderr, "Error getting flags for non blocking file\n");
        exit(1);
    }

    //compute the desired flags
    val |= flag;

    //set flags
    if(fcntl(fd, F_SETFL, val) == -1){
        fprintf(stderr, "Error settig flags for non blocking file\n");
        exit(1);
    }

}

