#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX_LENGTH 31

typedef struct{
	int id;
	long int regNumber;
	char surn[MAX_LENGTH], name[MAX_LENGTH];
	int mark;
}records_struct;

void text_2_Binary(char *file_1, char *file_2);
void binary_2_text(char *file_2, char *file_3);

int main(int argc, char *argv[]){

	char *file_1, *file_2, *file_3;
	//check presence of parameters
	if(argc != 4){
		fprintf(stdout, "Missing parameters (%d/3)\n", argc-1);
		exit(1);
	}

	//getting parameter
	file_1 = argv[1];
	file_2 = argv[2];
	file_3 = argv[3];
	
	text_2_Binary(file_1, file_2);	//converting ASCII file to a binary one
	binary_2_text(file_2, file_3);	//converting a binary file to an ASCII one

	return 0;
}

void text_2_Binary(char *file_1, char *file_2){
	records_struct records;		//struct to read/write files
	FILE *fp1;		//file pointer for the ISO C reading
	int fd2;		//file descriptor for POSIX write		
		
	fp1 = fopen(file_1,"r");	//open file_1 (ISO C)
	if(fp1 == NULL){
		fprintf(stdout, "(text_2_Binary) Error opening %s\n",file_1);
		exit(1);
	}
	
	//open file_2 (POSIX)
	fd2 = open(file_2, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if(fd2 == -1){
		fprintf(stdout, "(text_2_Binary) Error opening %s\n",file_2);
		exit(1);
	}

	while(fscanf(fp1, "%d %ld %s %s %d\n",
				 &records.id, &records.regNumber, records.surn,
				 records.name, &records.mark) != EOF ){
		if(write(fd2, &records, sizeof(records_struct)) != sizeof(records_struct)){
			printf("Error printing text_2_binary\n");
			exit(1);
		}
	}
	
	fclose(fp1);
	close(fd2);

	return ;
}

void binary_2_text(char *file_2, char *file_3){
	records_struct records;		//struct to read/write files
	FILE *fp3;		//file pointer for the ISO C write
	int fd2;		//file descriptor for POSIX read
	int nR;

	fp3 = fopen(file_3,"w");	//open file_3 (ISO C)
	if(fp3 == NULL){
		fprintf(stdout, "(binary_2_text) Error opening %s\n",file_3);
		exit(1);
	}
	
	//open file_2 (POSIX)
	fd2 = open(file_2, O_RDONLY);
	if(fd2 == -1){
		fprintf(stdout, "(binary_2_text) Error opening %s\n",file_2);
		exit(1);
	}
	
	while( (nR = read(fd2, &records, sizeof(records_struct))) > 0 )
		fprintf(fp3, "%d %ld %s %s %d\n",
				 records.id, records.regNumber, records.surn,
				 records.name, records.mark );
	fclose(fp3);
	close(fd2);

	return ;
}


