#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "proc-common.h"

#define SLEEP_PROC_SEC  10
#define SLEEP_TREE_SEC  3

/*
 * Create this process tree:
 * A-+-B---D
 *   `-C
 */

const char* A = "A";
const char* B = "B";
const char* C = "C";
const char* D = "D";



void fork_procs(const char *x)
{
	/*
	 * initial process is A.
	 */
	if (x == A){	
	change_pname("A");
	printf("A: Sleeping...\n");
	sleep(SLEEP_PROC_SEC);
	}

	if (x  == B){
	change_pname("B");
	printf("B: Sleeping...\n");
	sleep(SLEEP_PROC_SEC);
	}
	
	if (x == C) { 
	change_pname("C");
	printf("C: Sleeping...\n");
	sleep(SLEEP_PROC_SEC);

	printf("C: Exiting...\n");
	exit(17);
	}
	
	if (x == D) {
	change_pname("D");
	printf("D: Sleeping...\n");
	sleep(SLEEP_PROC_SEC);
	
	printf("D: Exiting...\n");
	exit(13);
	}
}

/*
 * The initial process forks the root of the process tree,
 * waits for the process tree to be completely created,
 * then takes a photo of it using show_pstree().
 *
 `* How to wait for the process tree to be ready?
 * In ask2-{fork, tree}:
 *      wait for a few seconds, hope for the best.
 * In ask2-signals:
 *      use wait_for_ready_children() to wait until
 *      the first process raises SIGSTOP.
 */
int main(void)
{
	pid_t pid;
	int status;

	/* Fork root of process tree */
	pid = fork();
	if (pid < 0) {
		perror("main: fork");
	}
	else if (pid == 0) {
		/* Child */
		pid = fork();
		if (pid < 0){
			perror("main: fork");
		} 
		else if (pid ==0) { 
			//create D
			pid = fork();
			if (pid < 0){
				perror("main: fork");
			}
			else if (pid==0){
				fork_procs("D");
			}
		
			else{	fork_procs("B");
				pid = wait(&status);
         			explain_wait_status(pid,status);//We need B to wait for it's child

         			printf("B: Exiting...\n");
         			exit(19);
			}

		}
		else {
			pid = fork();//C 
			if (pid < 0) {
			perror("main: fork");
			}
			else if (pid == 0) {
				fork_procs("C");
			}
			else {
		
				fork_procs("A");
				pid = wait(&status);
				explain_wait_status(pid,status);
				pid = wait(&status);
                		explain_wait_status(pid,status);//We need A to wait for both B and C

       				printf("A: Exiting...\n");
        			exit(16);
			}
		}
	}
	else {	
	/*
	 * Father
	 */
	/* for ask2-signals */
	/* wait_for_ready_children(1); */

	/* for ask2-{fork, tree} */
		sleep(SLEEP_TREE_SEC);

	/* Print the process tree root at pid */
		show_pstree(pid);
		printf("\n");
		show_pstree(getpid());
	/* for ask2-signals */
	/* kill(pid, SIGCONT); */

	/* Wait for the root of the process tree to terminate */
		pid = wait(&status);
		explain_wait_status(pid, status);
	}
	return 0;
}
