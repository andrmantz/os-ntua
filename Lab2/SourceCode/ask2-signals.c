#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "tree.h"
#include "proc-common.h"

#define SLEEP_PROC_SEC  10
#define SLEEP_TREE_SEC  3

void fork_procs( struct tree_node *root){

        change_pname(root->name);
        int status;
        int i=0;

        if (root->nr_children ==0){ //If it doesn't have children case
                printf("%s: Sleeping...\n", root->name);
                sleep (SLEEP_PROC_SEC);

                printf("%s: Exiting...\n", root->name);
                exit(1);
        }

        else{ //        Case it has children
                pid_t pid;
                for (i = 0; i < root->nr_children; i++) { //For each child...
                        pid = fork();
                        if (pid < 0){
                                perror("procs: fork");
                                exit(1);
                        }
                        if (pid==0) {
                                printf("%s: Creating child with PID = %ld\n", root->name, (long)getpid());
                                fork_procs(root->children+i);
                                exit(1);
                        }
                }

                for (i = 0; i < root->nr_children; i++){
                        printf("%s: Waiting...\n", root->name);
                       pid = wait(&status);
                        explain_wait_status(pid,status);
                }
                printf("%s: Exiting...\n", root->name);
        }
}

int main(int argc, char *argv[])
{
        pid_t pid;
        struct tree_node *root;
        int status;
        if (argc != 2) {
                fprintf(stderr, "Usage: %s <input_tree_file>\n\n", argv[0]);
                exit(1);
        }

        root = get_tree_from_file(argv[1]);
        pid= fork();//make root

        if (pid< 0) {
                perror("main: fork");
        }

        else if (pid == 0) {
                fork_procs(root);
                exit(1);
        }

        show_pstree(pid);
        pid = wait(&status);
        explain_wait_status(pid, status);
        return 0;
}

