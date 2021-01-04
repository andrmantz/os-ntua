#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <assert.h>

#include <sys/wait.h>
#include <sys/types.h>

#include "proc-common.h"
#include "request.h"

/* Compile-time parameters. */
#define SCHED_TQ_SEC 2                /* time quantum */
#define TASK_NAME_SZ 60               /* maximum size for a task's name */

typedef struct Node{
	pid_t pid;
	char *name;
	struct Node *next;
	} Node;

typedef Node * nodeptr;

nodeptr head = NULL;
nodeptr end = NULL; //arxika einai keni i lista

void insert( pid_t pid, const char *name){
		
	int flag= 0;
	if (head == NULL){
		flag = 1;
	}
	
	nodeptr new = (Node *) malloc(sizeof(Node));
	new->pid = pid;
	new->name= strdup(name);
	new->next = NULL;
	if (flag == 1){
		head = new;
		end = new;
	}
	else{
		end->next = new;
		end = new;	
	}

}

void delete (pid_t pid) {  //kathe fora i diergasia pou ekteleitai einai to head, ara mono auti mas endiaferei na diagrapsoume
		
	nodeptr temp = head;
	if (head->pid==pid){
		head=temp->next;
		free(temp);
	}
	
	else if (end->pid==pid){
		while(temp->next!=end){
			temp=temp->next;
		}
		end=temp;
		temp=temp->next;
		end->next=NULL;
		free(temp);
	}

	else{
		while(temp->next->pid!=pid){
			temp=temp->next;
		}
		nodeptr del= temp->next;
		temp->next=temp->next->next;
		free(del);
	}
		
		
	
	if (head == NULL){
		printf("List empty! Everything done!\n");
		exit(0);
	}
}

	
/*
 * SIGALRM handler
 */
static void
sigalrm_handler(int signum)
{
	if (kill(head -> pid, SIGSTOP)<0){
		perror("sigalrm");
		exit(10);
	}
}

/* 
 * SIGCHLD handler
 */
static void
sigchld_handler(int signum)
{
	pid_t id;
	int status;
	
	for(;;){
		id = waitpid(-1, &status, WUNTRACED| WNOHANG);
	
		if (id < 0) {
			perror("waitpid");
			exit(1);
		}
		if (id == 0) break;

		explain_wait_status(id,status);

		if (WIFEXITED(status) | WIFSIGNALED(status)){
	/* termatistike i diergasia, ara tin aferoume*/			
			delete(id); 
		}
		if (WIFSTOPPED(status)){
		/*den termatistike, ara topothetoume to head sto telos*/	
			end->next=head;
			end=head;
			nodeptr temp=head;
			head=head->next;
			temp->next=NULL;			
			
		}

		alarm(SCHED_TQ_SEC);
			
		if((kill(head->pid,SIGCONT))<0){
			perror("kill");
			exit(5);
		}
	}	
}	

/* Install two signal handlers.
 * One for SIGCHLD, one for SIGALRM.
 * Make sure both signals are masked when one of them is running.
 */
static void
install_signal_handlers(void)
{
	sigset_t sigset;
	struct sigaction sa;

	sa.sa_handler = sigchld_handler;
	sa.sa_flags = SA_RESTART;
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGCHLD);
	sigaddset(&sigset, SIGALRM);
	sa.sa_mask = sigset;
	if (sigaction(SIGCHLD, &sa, NULL) < 0) {
		perror("sigaction: sigchld");
		exit(1);
	}

	sa.sa_handler = sigalrm_handler;
	if (sigaction(SIGALRM, &sa, NULL) < 0) {
		perror("sigaction: sigalrm");
		exit(1);
	}

	/*
	 * Ignore SIGPIPE, so that write()s to pipes
	 * with no reader do not result in us being killed,
	 * and write() returns EPIPE instead.
	 */
	if (signal(SIGPIPE, SIG_IGN) < 0) {
		perror("signal: sigpipe");
		exit(1);
	}
}

int main(int argc, char *argv[])
{
	int nproc,i;
	pid_t pid;
	if (argc < 2){
		perror("arguments");
		exit(1);
	}
	nproc = argc-1;
	

	char executable[] = "prog";
	char *newargv[] = {executable, NULL, NULL, NULL };
	char *newenviron[] = { NULL};

	for(i =0; i<nproc; i++){
		pid = fork();
		if (pid<0){
			perror("main:fork");
		}
		if (pid == 0) {
			raise(SIGSTOP);
			printf("I am starting. My PID is %ld\n", (long)getpid());
			sleep(2);
			execve(executable, newargv, newenviron);
		}
		else{	
			insert(pid, argv[i+1]);
			
		}
	}
	 
	/* Wait for all children to raise SIGSTOP before exec()ing. */
	wait_for_ready_children(nproc);

	/* Install SIGALRM and SIGCHLD handlers. */
	install_signal_handlers();

	if (nproc == 0) {
		fprintf(stderr, "Scheduler: No tasks. Exiting...\n");
		exit(1);
	}
	alarm(SCHED_TQ_SEC);
	
	kill(head->pid, SIGCONT);	
	/* loop forever  until we exit from inside a signal handler. */
	while (pause())
		;

	/* Unreachable */
	fprintf(stderr, "Internal error: Reached unreachable point\n");
	return 1;
}
