#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX_LENGTH 15

typedef struct{
	int id;
	long regNumber;
	char surn[MAX_LENGTH], name[MAX_LENGTH];
	int mark;
}records_struct;

int main(int argc, char *argv[]){
	
	char *inputFile, cmd;
	int n, fd, nRW;	
	records_struct rec;

	
	if(argc != 2){
		fprintf(stdout, "Missing parameters (%d/1)\n", argc-1);
		exit(1);
	}
	
	inputFile = argv[1];	//getting paramter
	
	//open POSIX file
	fd = open(inputFile, O_RDWR);
	if(fd == -1){
		fprintf(stdout, "Error opening %s\n",inputFile);
		exit(1);
	}
	
	while(1){
		fprintf(stdout, "Insert command: ");
		fscanf(stdin, "%c %d%*c", &cmd, &n);
		//fflush(stdin);
		
		if(n>0) n--;
		
		switch(cmd){
		case 'R':
			lseek(fd, n*sizeof(records_struct), SEEK_SET);
			nRW = read(fd, &rec, sizeof(records_struct));
			if(nRW != sizeof(records_struct) || rec.id == 0){
				fprintf(stdout, "Read error (student may not exist)\n");
				break;			
			}
			fprintf(stdout, "%d %li %s %s %d\n", rec.id, rec.regNumber, rec.surn, rec.name, rec.mark);
			break;

		case 'W':
			lseek(fd, n*sizeof(records_struct), SEEK_SET);
			fscanf(stdin, "%d %li %s %s %d%*c", &rec.id, &rec.regNumber, rec.surn, rec.name, &rec.mark);
			nRW = write(fd, &rec, sizeof(records_struct));
			if(nRW != sizeof(records_struct)){
				fprintf(stdout, "Write error\n");
				break;			
			}
			fprintf(stdout, "Write success.\n");
			//fflush(stdin);
			break;

		case 'E':
			fprintf(stdout, "Execution terminated\n");
			return 0;
		
		default:
			fprintf(stdout, "Error\n");
			return -1;
		}
	}
	
}











