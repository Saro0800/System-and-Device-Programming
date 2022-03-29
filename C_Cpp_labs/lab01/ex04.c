#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]){
	
	setbuf(stdout, 0);

	if(argc < 3){
		printf("Missing parameters (%d/3)\n",argc);
		return 1;
	}

	int h=atoi(argv[1]), n=atoi(argv[2]);
	
/*	drawing down the CFG, i found out that at each outher iteration, there must be
	extactly n processes. It's important to notice that after a fork we have 2
	processes: one is the father, the other is the newly generated process.
	So in general, if at each outher iteration we want n processes (including the father)
	coming from each process, each process has to generate exactly n-1 processes, and all
	the children must break and go to the next iteration.
*/	
	for(int i=0; i<h; i++)
		for(int j=0; j<n-1; j++)
			if(!fork())	break;

	printf("My PID is: %d\n", getpid());
	return 0;
}