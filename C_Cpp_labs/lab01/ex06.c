#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

int lastSIG, consecSIG=0;

void signalHandler(int SIG){
	if(consecSIG==0 || SIG != lastSIG){
		if(SIG == SIGUSR1)
			printf("Success\nRecevied signal: SIGUSR1\n");
		else
			printf("Success\nRecevied signal: SIGUSR2\n");

		lastSIG = SIG;
		consecSIG=1;
		
	}else{
		if(SIG == SIGUSR1)
			printf("Failure\nRecevied signal: SIGUSR1\n");
		else
			printf("Failure\nRecevied signal: SIGUSR2\n");

		consecSIG++;

		if(consecSIG==3){
			printf("3 consecutive signales receveid.\nExecution terminated \n\n"),
			exit(0);
		}
	}

}
int main(int argc, char *argv[]){
	setbuf(stdout, 0);	
	
	signal(SIGUSR1, signalHandler);
	signal(SIGUSR2, signalHandler);
	while(1);


	exit(0);
}