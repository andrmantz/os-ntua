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
#define SHELL_EXECUTABLE_NAME "shell" /* executable for shell */

typedef struct Node{
        pid_t pid;
	int prio;
        char* name;
        int id;
	struct Node * next;
}Node;
typedef Node * nodeptr;

/*Definition of my list nodes*/
nodeptr head=NULL;
nodeptr end=NULL;

int serial_number;

static int
sched_set_high_prio(int id)
{
	nodeptr temp = head;
	while (temp!=NULL){
		if (temp->id == id){
			temp->prio = 1;
			return id;
		}
		else{
			if (temp == end && end->id!=id) break;
			else temp= temp->next;
		}
	}
	return -ENOSYS;
}


static int
sched_set_low_prio(int id)
{
        nodeptr temp = head;
        while (temp!=NULL){
                if (temp->id == id){
                        temp->prio = 0;
                        return id;
                }
                else{
                        if (temp == end && end->id!=id) break;
                        else temp= temp->next;
                }
        }
        return -ENOSYS;
}




/* Print a list of all tasks currently being scheduled.*/  
static void
sched_print_tasks(void)
{
	nodeptr temp=head;
	if (temp->next == NULL){
		if (temp->prio == 0){ 		
			printf("ID: %d PID: %i Name: %s Priority: Low \n",temp->id,temp->pid,temp->name);
		}
		else {
			printf("ID: %d PID: %i Name: %s Priority: High \n",temp->id,temp->pid,temp->name);
		}
	}
		
	else{
		while (temp!=NULL){
			if (temp->prio == 0){
                        	printf("ID: %d PID: %i Name: %s Priority: Low \n",temp->id,temp->pid,temp->name);
                	}
                	else {
                        	printf("ID: %d PID: %i Name: %s Priority: High \n",temp->id,temp->pid,temp->name);
                	}
			temp = temp->next;
		}
	
	}
}

/* Send SIGKILL to a task determined by the value of its
 * scheduler-specific id.
 */

static int
sched_kill_task_by_id(int id)
{
	nodeptr temp=head;
	while (temp!=NULL){
		if (temp->id == id ){
			kill(temp->pid,SIGKILL);
			return id;
		}
		else	
			if (temp == end && end->id != id) break;
			else temp = temp->next;	
	}
	return -ENOSYS;
}


/* Create a new task.  */
static void
sched_create_task(char *executable)//
{ 		
		pid_t mypid;
                mypid=fork();
                if (mypid < 0){
                        perror("fork");
                }
                if(mypid == 0){

			char *newargv[] = {executable, NULL, NULL, NULL};
			char *newenviron[] = {NULL};

                        raise(SIGSTOP);
                        printf("I am %s, PID = %ld\n",
                        executable, (long)getpid());
                        printf("About to replace myself with the executable %s...\n",executable);
                        sleep(2);
                        execve(executable, newargv, newenviron);
                        /* execve() only returns on error */
                        perror("execve");
                        exit(1);
                        }
                else{ 
			/* Insert the new process to the list */
		        nodeptr newnode=(nodeptr)malloc(sizeof(Node));
                        if (newnode == NULL){
		        	perror("malloc");
				exit(2);
                  	}
		        newnode->pid=mypid;
                        newnode->next=NULL;
                        newnode->id=serial_number;
			newnode->name= strdup(executable);
                        end->next=newnode;
                        end=newnode;
			newnode->prio = 0;
			serial_number ++;        
                }

}

/* Process requests by the shell.  */
static int
process_request(struct request_struct *rq)
{
	switch (rq->request_no) {
		case REQ_PRINT_TASKS:
			sched_print_tasks();
			return 0;

		case REQ_KILL_TASK:
			return sched_kill_task_by_id(rq->task_arg);

		case REQ_EXEC_TASK:
			sched_create_task(rq->exec_task_arg);
			return 0;
			
		case REQ_HIGH_TASK:
			return sched_set_high_prio(rq->task_arg);

		case REQ_LOW_TASK:
			return sched_set_low_prio(rq->task_arg);

		default:
			return -ENOSYS;
	}
}

/* 
 * SIGALRM handler
 */
static void
sigalrm_handler(int signum)
{
        if ((kill(head->pid,SIGSTOP))<0){
		perror("sigalrm: kill");
		exit(51);
	}
}


/* 
 * SIGCHLD handler
 */
static void
sigchld_handler(int signum)
{
        pid_t p;
        int status;
	int flag = 0; //boool 0 = false/den uparxoun high priorities, 1= true/uparxoun high
	for(;;){
                p=waitpid(-1,&status,WNOHANG|WUNTRACED);

                if (p<0){
                        perror("waitpid");
                        exit(1);
                }
                if(p==0) break;

                /* a child has changed his status, lets check what happened */
                explain_wait_status(p,status);


                if (WIFEXITED(status) || WIFSIGNALED(status)){
                        /* If i am here means that a child is terminated OR killed by a signal */
                        /* Because of that i have to remove it from the list */
                        nodeptr previous = NULL;
                        nodeptr current = head;

                        while(current != NULL){
                                if (current->pid == p && current == head){
                                        /* I have to delete the head of my list */
                                                if (current->next == NULL){
                                                   
                                                         free(current);
                                                         printf("The list is empty...I don't have any more work to do\n");
                                                         exit(0);
                                                }
                                                else{
                                                        head=current->next;
                                                        free(current);
                                                }
                                }
                                else if (current->pid == p && current == end){
                                        /* I have to delete the last node of my list */
                                        end=previous;
                                        end->next=NULL;
                                        free(current);
                                }
                                else if(current->pid == p){
                                        /* I have to delete a random node of the list but sure its not head or tail */
                                        previous->next=current->next;
                                        free(current);
                                }
                                else{
                                        /* I will continue searching*/
                                        previous=current;
                                        current=current->next;
                                        continue;
                                }
                                break;
			}
			
			nodeptr temp = head;
			nodeptr hd=head;
                        //kano rotate ti lista mexri na bro high priority kai na to fero sto head
			while (temp != NULL){
                                        if (temp->prio!=1){
                                                /* In this case i have to put the process at the end of the list */
                                                end->next=head;
                                                end=head;
                                                head=head->next;
                                                end->next=NULL;
                                                temp=head;
                                                if (temp == hd){
							flag = 0;  //den uparxoun high priorities
							break;
						}
                                        }
                                        else{
                                                 flag = 1 ; //head->prio == 1
                                                 break;
                                        }
                        }
	
		}
                            
                if (WIFSTOPPED(status)){		
			/* I am here if a process is stopped */
			/* I have to put this process to the end of the list and continue with the next one process according to the priorities */
			if (head == NULL) {
				printf("Empty list\n"); 
				exit(0);
			}
			if (head->next != NULL){ //an head->next == NULL, tote exo mono mia diergasia kai sunexizo stin ektelesi tis
				//prota tha steilo to head sto telos kai epeita tha psakso an uparxoun high priorities
				//akolouthontas to idio skeptiko me prin
				end->next=head;
                        	end=head;
				head=head->next;
				end->next=NULL;
				nodeptr hd=head;
				nodeptr temp = head;
			
				while (temp != NULL){
					if (temp->prio!=1){ 
						/* In this case i have to put the process at the end of the list */
						end->next=head;
						end=head;
						head=head->next;
						end->next=NULL;
						temp=head;
						if (temp == hd){
							flag = 0;
							break;
						}
					}
					else{
						 flag = 1 ;
						 break;
					}
				 }
			}
                }

	//gia na mpoume sto shell prepei na erthei i seira tou + na exei prio 1 i na mhn uparxei diergasia me prio 1
		if( head->id == 0 && (flag == 0 || head->prio == 1)  ){
				printf("I am in Shell again:\n");
				alarm(5*SCHED_TQ_SEC);
		}	
		else alarm(SCHED_TQ_SEC);
        
		/* Send SIGCONT to the head of the list */
                if ((kill(head->pid,SIGCONT))<0){
			perror("sigchld: kill");
			exit(3);
		}
        }

}


/* Disable delivery of SIGALRM and SIGCHLD. */
static void
signals_disable(void)
{
	sigset_t sigset;

	sigemptyset(&sigset);
	sigaddset(&sigset, SIGALRM);
	sigaddset(&sigset, SIGCHLD);
	if (sigprocmask(SIG_BLOCK, &sigset, NULL) < 0) {
		perror("signals_disable: sigprocmask");
		exit(1);
	}
}

/* Enable delivery of SIGALRM and SIGCHLD.  */
static void
signals_enable(void)
{
	sigset_t sigset;

	sigemptyset(&sigset);
	sigaddset(&sigset, SIGALRM);
	sigaddset(&sigset, SIGCHLD);
	if (sigprocmask(SIG_UNBLOCK, &sigset, NULL) < 0) {
		perror("signals_enable: sigprocmask");
		exit(1);
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

static void
do_shell(char *executable, int wfd, int rfd)
{
	char arg1[10], arg2[10];
	char *newargv[] = { executable, NULL, NULL, NULL };
	char *newenviron[] = { NULL };

	sprintf(arg1, "%05d", wfd);
	sprintf(arg2, "%05d", rfd);
	newargv[1] = arg1;
	newargv[2] = arg2;

	raise(SIGSTOP);
	execve(executable, newargv, newenviron);

	/* execve() only returns on error */
	perror("scheduler: child: execve");
	exit(1);
}

/* Create a new shell task.
 *
 * The shell gets special treatment:
 * two pipes are created for communication and passed
 * as command-line arguments to the executable.
 */
static void
sched_create_shell(char *executable, int *request_fd, int *return_fd)
{
	pid_t p;
	int pfds_rq[2], pfds_ret[2];

	if (pipe(pfds_rq) < 0 || pipe(pfds_ret) < 0) {
		perror("pipe");
		exit(1);
	}

	p = fork();
	if (p < 0) {
		perror("scheduler: fork");
		exit(1);
	}

	if (p == 0) {
		/* Child */
		close(pfds_rq[0]);
		close(pfds_ret[1]);
		do_shell(executable, pfds_rq[1], pfds_ret[0]);
		assert(0);
	}
	/* Parent */
	close(pfds_rq[1]);
	close(pfds_ret[0]);
	*request_fd = pfds_rq[0];
	*return_fd = pfds_ret[1];

	/* I have to insert the Shell as the head of my list */
	head->id=0;
	head->pid=p;
	head->prio = 0;
	head->name="shell";
	head->next=NULL;
}

static void
shell_request_loop(int request_fd, int return_fd)
{
	int ret;
	struct request_struct rq;

	/*
	 * Keep receiving requests from the shell.
	 */
	for (;;) {
		if (read(request_fd, &rq, sizeof(rq)) != sizeof(rq)) {
			perror("scheduler: read from shell");
			fprintf(stderr, "Scheduler: giving up on shell request processing.\n");
			break;
		}

		signals_disable();
		ret = process_request(&rq);
		signals_enable();

		if (write(return_fd, &ret, sizeof(ret)) != sizeof(ret)) {
			perror("scheduler: write to shell");
			fprintf(stderr, "Scheduler: giving up on shell request processing.\n");
			break;
		}
	}
}

int main(int argc, char *argv[])
{
	
	/* Two file descriptors for communication with the shell */
	static int request_fd, return_fd;

	/* Initialize the head and the end of the list to put the Shell on the head and the end as well because at the start Shell will be the only node-process */	
	head=(nodeptr)malloc(sizeof(Node));
	if(head == NULL){
		perror("malloc");
		exit(2);
	}
	end=head;

	/* Create the Shell and put Shell at the head of the list*/

	sched_create_shell(SHELL_EXECUTABLE_NAME, &request_fd, &return_fd);

	/* TODO: add the shell to the scheduler's tasks */


	/*
	 * For each of argv[1] to argv[argc - 1],
	 * create a new child process, add it to the process list.
	 */
	int nproc;
	int i;
	nproc = argc-1; /* number of proccesses goes here */

        if (nproc < 0) {
                fprintf(stderr, "Scheduler: No tasks. Exiting...\n");
                exit(1);
        }

	nodeptr newnode;
        pid_t mypid;

        /*Fork and create the list*/
        for (i=1;i<=nproc;i++){
                mypid=fork();
                if (mypid<0){
                        perror("main: fork");
                }
                if(mypid==0){
			char executable[] = "prog";
			char *newargv[] = {executable, NULL, NULL, NULL};
			char *newenviron[] = {NULL};


                        raise(SIGSTOP);
                        printf("I am %s, PID = %ld\n",
                        argv[0], (long)getpid());
                        printf("About to replace myself with the executable %s...\n",
                                executable);
                        sleep(2);

                        execve(executable, newargv, newenviron);

                        /* execve() only returns on error */
                        perror("execve");
                        exit(1);
                        }
                else{
                        newnode=(nodeptr)malloc(sizeof(Node));
                        if (newnode == NULL){
				perror("malloc");
				exit(2);
			} 
                        newnode->pid=mypid;
                        newnode->next=NULL;
                        newnode->id=i;
                        newnode->name=argv[i];
			newnode->prio = 0;
                        end->next=newnode; 
                        end=newnode;
                }
        }

	serial_number = i;
	/* Wait for all children to raise SIGSTOP before exec()ing. */
	wait_for_ready_children(nproc);
	/* Install SIGALRM and SIGCHLD handlers. */
	install_signal_handlers();
        /*Set the alarm on*/
        alarm(SCHED_TQ_SEC);

        /*Start the first process*/
	kill(head->pid,SIGCONT); //wake up the shell

	shell_request_loop(request_fd, return_fd);
	/* Now that the shell is gone, just loop forever
	 * until we exit from inside a signal handler.
	 */
	while (pause())
		;

	/* Unreachable */
	fprintf(stderr, "Internal error: Reached unreachable point\n");
	return 1;
}
