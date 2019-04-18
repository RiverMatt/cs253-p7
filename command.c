#ifdef __STDC_ALLOC_LIB__
#define __STDC_WANT_LIB_EXT2__ 1
#else
#define _POSIX_C_SOURCE 200809L
#endif
#include "smash.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "history.h"
#include <sys/types.h>
#include <sys/wait.h>


static int cmdcount = 0;

void executeCommand(char* str) {
	
	/* For history */
	int firstSequenceNumber = 1;

	if (cmdcount >= MAXHISTORY) {
		firstSequenceNumber = cmdcount - MAXHISTORY + 1;
	}  
	cmdcount++;

	char* copy = strndup(str, strlen(str));	// copy of the command string
	
	char* args[MAXLINE];			// array to store argument strings

	char* token = strtok(str, " ");		// token for the first arg
	
	/* Putting the args into an args array */
	int stepindex = 0;
	while (token != NULL) {
		args[stepindex] = token;
		stepindex++;
		token = strtok(NULL, " ");	// get the next token

	}
	args[stepindex] = NULL;
	
	/* I/O redirect */
	for (int i = 0; i < stepindex; i++) {
		if (strcmp(args[i], "<") == 0) {
			freopen(args[i + 1], "r", stdin);
		}
		if (strcmp(args[i], ">") == 0) {
			freopen(args[i + 1], "r", stdout);
		}
	}
	/* Internal commands */
	if (strcmp(args[0], "exit") == 0) {
		free(copy);
		clear_history();
		exit(0);
	} else if (strcmp(args[0], "cd") == 0) {
		if (stepindex == 2){
			int cdir = chdir(args[1]);

			if (cdir != 0) {
				add_history(copy, 1);
				perror(args[stepindex]);
			} else {
				add_history(copy, 0);
				fprintf(stdout, "%s\n", args[1]);
			} 
		} else {
			
			fprintf(stderr, "Invalid command\n");
			add_history(copy, 1);
		}
	} else if (strcmp(args[0], "history") == 0) {
		
		print_history(firstSequenceNumber);
		add_history(copy, 0);

	/* External commands */
	} else {
		
		int extCmd = executeExternalCommand(args);
		
		add_history(copy, extCmd);
	}

	
}

int executeExternalCommand(char* args[1026]) {
	
	int pid = fork();

	if (pid == 0) {			// Child process
		execvp(args[0], args);	// first parameter is the file descriptor (command), second is args array
		perror(args[0]);
		exit(127);
	} else if (pid > 0) {		// Parent process
		int exitStatus;
		wait(&exitStatus);
		if (exitStatus != -1) {
			return 127;
		} else {
			return exitStatus;
		}
	} else {
		perror("Fork failed!");
	}

	return -1;
	
}
