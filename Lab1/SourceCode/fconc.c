#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

int open_file(const char *file1, int oflags){  //anoigma arxeiou
	int fd;
	fd = open(file1,  oflags , S_IRUSR|S_IWUSR);
	if (fd == -1){ //error 
		return -1;
	}
	return fd;//epistrofi pointer sto arxeio
}


int check_args (int argc){
	if (argc < 3 || argc > 4) {
		fprintf(stderr, "Usage: ./fconc infile1 infile2 [outfile (default:fconc.out)]\n");
		return -1;}
	return 0;
}

void doWrite(int fd, const char *buff, int len){
	int idx, wcnt;
	idx = 0;
	
	do {
		wcnt = write(fd, buff + idx, len - idx);
		if (wcnt == -1) {
			perror("write");
			exit(1);
		}
		idx += wcnt;
	} while (idx < len) ;
} 
		
void write_file(int fd, const char *infile){
	int infd, rcnt;
	char buff[1024];
	infd = open_file(infile, O_RDONLY);
	if (infd == -1) {
		perror(infile);
		exit(1);
	}
	for (;;) {
		rcnt = read(infd, buff, sizeof(buff));
		if (rcnt == 0) break;
		if (rcnt == -1) {
			perror("write");
			exit(1);
		}
		
		doWrite(fd, buff, rcnt);
	}
	close(infd);
}


int main(int argc, char **argv)
{
	char  *outfile;
	int fileout;
	
	if (argc==4){
		if ((strcmp(argv[1], argv[3]) == 0) || (strcmp(argv[2], argv[3]) == 0)){
			printf("Input and output have to be different files!\n");
			exit(1);
		}
	}

	if (check_args(argc) != 0) exit(1);

	if (argc == 3){
		outfile = "fconc.out";
		fileout = open_file(outfile,  O_CREAT | O_WRONLY | O_TRUNC);
		if (fileout == -1) exit(1);
		
		write_file(fileout, argv[1]);
		write_file(fileout, argv[2]);
		close(fileout);
}
	if (argc == 4) {
		
		outfile = argv[3];
		fileout = open_file(outfile,  O_CREAT | O_WRONLY | O_TRUNC);
		if (fileout == -1) exit(1);

		write_file(fileout, argv[1]);
		write_file(fileout, argv[2]);
		close(fileout);
	}
	return 0;
}
		
