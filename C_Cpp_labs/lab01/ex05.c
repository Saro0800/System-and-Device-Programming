#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[]){
	setbuf(stdout, 0);	

	int i, n= atoi (argv[1]), nChild=0;
  	int *vet;
	char *vetS;
	
	vet = (int *)malloc(n*sizeof(int));
	if(vet == NULL){
		printf("Allocation Error.\n");
		exit(1);
	}

	vetS = (char *)malloc((n+1)*sizeof(char));
	if(vetS == NULL){
		printf("Allocation Error.\n");
		exit(1);
	}

//binary numbers generation
	for(i=0; i<n; i++)
		if( fork() ) {vet[i] = 0; nChild++;}	//father puts 0
		else {vet[i] = 1; nChild=0;}			//child puts 1

//print created number on a String
	for(i=0; i<n; i++)
		sprintf(&vetS[i], "%d", vet[i]);
	sprintf(&vetS[i], "\0");

//print the string
	fprintf(stdout, "%s\n", vetS);

	for(i=0; i<nChild; i++)
		wait(NULL);

	exit(0);
}