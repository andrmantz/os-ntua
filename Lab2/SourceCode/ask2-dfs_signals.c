#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "tree.h"
#include "proc-common.h"

void fork_procs(struct tree_node * root){

        change_pname(root->name); //gia na allaksoume to id ka8e diadikasias sto epi8ymhto onoma
        int i;
        int status;
        pid_t pid2[root->nr_children]; //hold the pid's
        if (root->nr_children == 0){ //if the node has no children
                printf("Process %s started.",root->name);
                printf("Process %s: Waiting for SIGCONT\n",root->name);
                raise(SIGSTOP);
                printf("Process %s: Exiting\n",root->name);
                exit(0);
        }
        else{
                //if the node has children
                pid_t pid;
                printf("Process %s started creating children\n",root->name);
                for (i=0;i<root->nr_children;i++){
                       pid=fork();
                        if (pid<0){
                                perror("fork_procs: fork\n");
                                exit(1);
                        }
                        if (pid == 0){
                                fork_procs(root->children+i); 
                                printf("Process %s : Exiting\n",root->name);
                               exit(0);
                        }
                        else{ 
                                pid2[i]=pid;
                        }
                        wait_for_ready_children(1); //waits for every child
                }
            
                printf("PID= %ld, name = %s is waiting for SIGCONT\n",(long)getpid(),root->name);
                raise(SIGSTOP); 
                printf("PID = %ld, name = %s is awake\n",
                (long)getpid(), root->name);
                //Send SIGCONT to every different pid in the pid2 table
                for(i=0;i<root->nr_children;i++){
                        kill(pid2[i],SIGCONT);
                        wait(&status);
                        explain_wait_status(pid2[i],status);
                }
		exit(0);
	}
}

int main(int argc, char *argv[])
{
	pid_t pid;
	int status;
	struct tree_node *root;

	if (argc < 2){
		fprintf(stderr, "Usage: %s <tree_file>\n", argv[0]);
		exit(1);
	}

	/* Read tree into memory */
	root = get_tree_from_file(argv[1]);

	/* Fork root of process tree */
	pid = fork();
	if (pid < 0) {
		perror("main: fork");
		exit(1);
	}
	if (pid == 0) {
		/* Child */
		/* root of my tree */
		/*for every child I enter this if and 
		decide in let_it_fork if it is a father of another child 
		to fork again until I reach leaves*/
		fork_procs(root);
		exit(0);
	}
	
	/*
	 * Father
	 */
	/* for ask2-signals */
	wait_for_ready_children(1);
	/* for ask2-{fork, tree} */
	/* sleep(SLEEP_TREE_SEC); */
	/* Print the process tree root at pid */
	show_pstree(pid);
	kill(pid, SIGCONT);
	/* Wait for the root of the process tree to terminate */
	wait(&status);
	explain_wait_status(pid, status);
	return 0;
}



