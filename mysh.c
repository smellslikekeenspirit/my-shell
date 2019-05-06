/*
** Author: Prionti Nasir
** Email: pdn3628@rit.edu
** This file is under git.
*/


#define _DEFAULT_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include "mysh.h"


// statics for running the main process or shell
static int cmd_num;
static int verbose_status = 0;
static int bang_num = 0;

/***************************************QUEUE IMPLEMENTATION***************************************/

// a node structure, used to define a queue unit
typedef struct Node{
	char *command;
	int cmd_num;
  	struct Node *next;
}Node;


/// a constructor for a node
Node *create_node(char *command){
        Node *node = NULL;
        node = malloc(sizeof(Node));
        node->command = strdup(command);
	node->cmd_num = cmd_num;
        node->next = NULL;
        return node;
}

// a queue structure, used as a FIFO history-keeping line

typedef struct Queue{ 
	Node *back;
	Node *front;
	int max_size;
	int size;
}Queue;

// static queue to maintain history for a lifetime of the shell

static Queue *history_queue;


///creates an empty queue and returns a pointer

Queue *create_queue(int max_size){
  Queue *queue = malloc(sizeof(Queue));
  queue->front = NULL;
  queue->back = NULL;
  queue->max_size = max_size;
  queue->size = 0;
  return queue;
}


/// adds a node to the back of a queue

void enqueue(Queue *queue, char *command){
        Node *node = create_node(command);
	if (queue->size == 0){
            queue->front = node;
            queue->back = queue->front;
	}
	else{
            queue->back->next = node;
            queue->back = node;
	}
	queue->size++;
}

/// removes a node from the front of a queue

void dequeue(Queue *queue){
        if (queue->size == 0) return;

        free(queue->front->command);
        Node *node = queue->front->next;
        free(queue->front);
        queue->front = NULL;
        queue->front = node;

        if(queue->size == 1){
                queue->back = NULL;
                queue->front = NULL;
        }

        queue->size--;
}

/// frees memory for a queue and sets pointers to NULL

void destroy_queue(Queue *queue){
        while(queue->size){
            dequeue(queue);
        }
        free(queue);
        queue = NULL;
}

/**************************************************************************************************/


/// parses shell commands sent by user

char **parse(int *tokens_size, char *command){
        char *cmd = strdup(command);
        char **tokens = malloc(sizeof(char *) * sizeof(char **));
        char *next_token;
        int index = 0;
        next_token = strtok(cmd, " ");
        while (next_token){
                tokens[index] = next_token;
                index++;
                (*tokens_size)++;
                next_token = strtok(NULL, " ");
        }
        
        tokens[index] = NULL;
        return tokens;
}

/// handles main command, i.e. before shell has been evoked

void command_handler(int argc, char *argv[]){

    int index;
    int history_handled = 0;
	for (index = 1; index < argc-1; index++){
		if (strcmp(argv[index], "-v") == 0){
			verbose_status = 1;
		}
		else if (strcmp(argv[index], "-h") == 0){
			if (atoi(argv[index+1]) > 0){
				history_queue = create_queue(atoi(argv[index+1]));
				history_handled = 1;
			}
			else{
				fprintf(stderr,"usage: mysh [-v] [-h pos_num]");
				exit(EXIT_FAILURE);
			}
		}
	}
	if (!history_handled) {
            history_queue = create_queue(10);
        }
}

/// switches on verbose mode for the shell

int verbose(int argc, char *argv[]) {
    (void) argv;
    verbose_status = argc;
    return verbose_status;
}

/// checks if a command is internal or external
 
int is_external(int *argc, char *argv[]){
    int status = 1;
    if (strcmp(argv[0], "quit") == 0){
	quit(0, argv);
	    status = 0;
    }
    else if (strcmp(argv[0], "help") == 0){
	help(*argc, argv);
	status = 0;
    }
    else if (strcmp(argv[0], "history") == 0){
	history(history_queue->size, argv);
        status = 0;
    }
    else if (strcmp(argv[0], "verbose") == 0){
        if (strcmp(argv[1], "on") == 0){
	    verbose(1, argv);
	}
	else if (strcmp(argv[0], "off") == 0){
            verbose(0, argv);
	}
	    status = 0;
    }
    else if (**argv == '!'){
	    char *p = (*argv) + 1;
	    bang_num = atoi(p);
            
	    bang(*argc, argv);
	    status = 0;
    }
    return status;
		
}

///produces a usage guide to the user

int help(int argc, char *argv[]) {
    (void) argc;
    (void) argv;
    printf("1. help: Print out a list of Internal Commands:\n");
    printf("2. !N: Execute the Nth command within the history list\n");
    printf("3. history: Print out a list of commands that have been executed so far\n");
    printf("4. quit: Clean up all the memory and gracefully terminate the shell program\n");
    printf("5. verbose on/off: When verbose status is on, the shell should print a list of the commands and their arguments\n");
    return 0;

}

/// provides a history of the last history->max_size or less commands

int history(int argc, char *argv[]) {
    (void) argc;
    (void) argv;
    if (history_queue->size == history_queue->max_size){
        dequeue(history_queue);
    }
    enqueue(history_queue, "history");
    cmd_num++;

    int i;
    Node *new = history_queue->front;
    printf("%d commands\n", cmd_num);
    for(i = 1; i <= history_queue->size; i++) {
        printf("\t[%d]: ", i);
        printf("%s\n", new->command);
	new = new->next;
    }
    return 1;

}

/// quits the process

int quit(int argc, char *argv[]) {
    (void) argv;
    printf("Quitting the program!\n");
    destroy_queue(history_queue);
    exit(argc);  
    return 0;
}


/// handles shell commands i.e. those meant for this shell

int shell_command_handler(char *input, char **tokens, int *tokens_size){
	if (verbose_status == 1){
		printf("\tcommand: %s\n", input);
		printf("\tinput command tokens:\n");
	        int index;
	        for (index = 0; index < *tokens_size; index++){
		    printf("\t%d: %s\n", index, tokens[index]);
		   
                }
	}
	
    pid_t child_pid;
    int cmd_processed = 0;
    if (is_external(tokens_size, tokens) == 1){
	child_pid = fork();
	if (0 == child_pid){
            // We're the child process
	    if (verbose_status == 1){
	        printf("\twait for pid %d: %s\n", getpid(), tokens[0]);
	        printf("\texecvp: %s\n", tokens[0]);
            }
	    if (execvp(tokens[0], tokens) < 0){
                printf("Error: %s\n", strerror(errno));
                quit(1, NULL);
            }
            
        }
        else if (child_pid > 0){
            // We're the parent process
	    int status;
            wait(&status);
	    /*you made a exit call in child you need to wait on exit status of child*/
            if(WIFEXITED(status)){
	        printf("child exited with status %d\n",WEXITSTATUS(status));
                if (WEXITSTATUS(status)){
			cmd_processed = 0;
		}else{
			cmd_processed = 1;
		}
	    }
	}
    }
    
    else{
	cmd_processed = 1;
    }
			
    if ( child_pid != 0 && !cmd_processed) {
        printf("command failed\n");
    }
    return cmd_processed;
}


// INTERNAL COMMANDS

/// executes the nth command, where n is the number entered by user

int bang(int argc, char *argv[]){
        (void) argc;
        (void) argv;
        int tokens_s;
        int *tokens_size = &tokens_s;

	if (history_queue->size >= bang_num){
	    Node *new = malloc(sizeof(Node));
	    new = history_queue->front;
	    while (new->cmd_num != bang_num){
		new = new->next;
            }
	    char **tokens;
	    
	    tokens = parse(tokens_size, new->command);
            shell_command_handler(new->command, tokens, tokens_size);
	}
        return 1;
}

					
// MAIN METHOD: LOOP FOR SHELL

int main(int argc, char *argv[]) {

    char **tokens;
    int tokens_s = 0;
    int *tokens_size = &tokens_s;
    cmd_num = 1;
    size_t len = 100;
    command_handler(argc, argv);
    char *buf = malloc(len * sizeof(char));
    printf("mysh\n");
    while (1) {
	printf("\033[0;32m")
        printf("mysh[%d]", cmd_num);
        fgets(buf, len, stdin);
        char *pos;
        if ((pos=strchr(buf, '\n')) != NULL) *pos = '\0';
	if (buf == NULL) break;
        tokens = parse(tokens_size, buf);
        if (1 == shell_command_handler(buf, tokens, tokens_size) && (strcmp(tokens[0], "history") != 0)){
            if (history_queue->size == history_queue->max_size){
                dequeue(history_queue);
            }
            enqueue(history_queue, buf);
            cmd_num++;
        }
	if (buf) free(buf);
        if (tokens) free(tokens);
        tokens_s = 0;
    }

    return EXIT_SUCCESS;

}

