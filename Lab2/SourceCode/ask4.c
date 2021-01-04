#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include "tree.h"
#include "proc-common.h"



#define SLEEP_TREE_SEC 3
#define SLEEP_PROC_SEC 10



void fork_procs(struct tree_node *root, int fd)
{

	change_pname(root->name);

	//if its a leaf
	if(root->nr_children == 0) {
		int num = atoi(root->name);
		if ( write(fd, &num, sizeof(num) ) != sizeof(num) ) {
			perror("Pipe write Failed\n");
			exit(1);
		}
		printf("PID: %ld, %s: Wrote value: %d and exiting..\n", (long)getpid(),root-> name, num);
		printf("%s: Sleeping...\n", root->name);
                sleep(SLEEP_PROC_SEC);
                printf("%s: Exiting...\n", root->name);
		exit(1);
	}
	else {

		//if its NOT a leaf
		pid_t pid1, pid2;
		int fd2[2];
		if( pipe(fd2) < 0 ) {
			perror(" pipe");
			exit(1);
		}
	
		pid1 = fork();
		if (pid1 < 0){
			perror("p1 fork");
			exit(1);
		}
		else if (pid1==0) {
			printf("%s: Created child  with PID=%ld\n", root->name, (long)getpid());
			fork_procs(root->children, fd2[1]);
		}

		pid2 = fork();
		if (pid2 < 0){
			perror("p2 fork");
			exit(1);
		}
	
		else if (pid2 ==0){
			printf("%s: Created child with PID=%ld\n", root->name, (long)getpid());
			fork_procs(root->children + 1, fd2[1]);
		}

		int num1, num2;
	
		int result;
	
		if (read (fd2[0], &num1, sizeof(num1))!=sizeof(num1)) {
       	                perror("Read from pipe");
                        exit(1);
	        }
		

		if (read (fd2[0], &num2, sizeof(num2))!=sizeof(num2)) {
                        perror("Read from pipe");
                        exit(1);
        	}
	

		if (strcmp(root->name,"+") == 0){
		        result = num1 + num2;
		}
		else if (strcmp(root->name, "*") ==0) {
		        result = num1 * num2;
		}
		

		if ( write(fd, &result, sizeof(result) ) != sizeof(result) ) {
		        perror("Pipe Write");
	        	exit(1);
		}

		 printf("Wrote %d to the pipe!\n", result);
	
		sleep(SLEEP_PROC_SEC);
		exit(1);
	}
}

int main(int argc, char *argv[])
{
        pid_t pid;
        int status;
        struct tree_node *root;

        int fd[2];

        if (argc < 2){
                fprintf(stderr, "Usage: %s <tree_file>\n", argv[0]);
                exit(1);
        }

        /* Read tree into memory */
        root = get_tree_from_file(argv[1]);

        if( pipe(fd) < 0 ){
                perror("main: pipe");
		exit(1);
        }

        /* Fork root of process tree */
        pid = fork();
        if (pid < 0) {
                perror("main: fork");
                exit(1);
        }

        if (pid == 0) {
                /* Child */
                change_pname(root->name);
                printf("%s: Created\n", root->name);
		close(fd[0]);
                fork_procs(root, fd[1]);
                exit(1);
        }



        sleep(SLEEP_TREE_SEC);


        show_pstree(pid);

        int result = 0;

        if ( read(fd[0], &result, sizeof(result) ) != sizeof(result) ){
                perror("main: read");
                exit(10);
        }


	wait(&status);
        explain_wait_status(pid, status);

        printf("Total result is: %d\n", result);

        return 0;
}
