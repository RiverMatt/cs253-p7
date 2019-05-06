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
#include <sys/stat.h>
#include <fcntl.h>

#define IN_REDIRECT 10		// these are like enums and make life easier
#define OUT_REDIRECT 9
#define NO_REDIRECT -1
#define MAXPIPES 1024
#define MAXFILENAME 1024

static int cmdcount = 0;
static int firstSequenceNumber;
static char* copy;

/**
 * Entry point for this file, parses for pipes then
 * passes the command(s) on for execution.
 */
void parsePipes(char* str) {
	
	/* For history */
	firstSequenceNumber = 1;

	if (cmdcount >= MAXHISTORY) {
		firstSequenceNumber = cmdcount - MAXHISTORY + 1;
	}  
	cmdcount++;

	copy = strndup(str, strlen(str));		// copy of the command string

	/* Parse for pipes */
	char* pipes[MAXPIPES];
	char* token = strtok(str, "|");
	
	int stepindex = 0;
	while (token != NULL) {
		pipes[stepindex] = token;
		stepindex++;
		token = strtok(NULL, "|");
	}
	pipes[stepindex] = NULL;
	
	int numCommands = stepindex;
	int numPipes = numCommands-1;
	static int exitStatus;
	
	if (numPipes == 0) {
		exitStatus = runCommand(pipes[0]);
	} else {

		/* Setting up the pipeline for the children to traverse */
		int pipeline[2*numPipes];
		for (int i = 0; i < numPipes; i++) {
			pipe(&pipeline[2*i]);
		}

		/* Forking the children and traversing the pipeline */
		for (int i = 0; i < numCommands; i++) {
			int child = fork();
			if (child < 0) {
				perror("child fork failed!");
				exit(-1);
			} else if (child == 0) {
			
				if (i == 0) {
					/* pipeline[0] is the outlet */
					close(pipeline[0]);

					dup2(pipeline[1], 1);
					
					/* Close unused pipes */
					for (int j = 2; j < numPipes*2; j++) {
						close(pipeline[j]);
					}
					exitStatus = runCommand(pipes[i]);

					/* Close the inlet after the process finishes writing to it */
					close(pipeline[1]);
					
				} else if (i+1 == numCommands) {
					
					int readindex = (i-1)*2;
					dup2(pipeline[readindex], 0);
				
					for (int j = 0; j < numPipes*2; j++) {
						if (j != readindex) {
							close(pipeline[j]);
						}
					}
						
					exitStatus = runCommand(pipes[i]);
					close(pipeline[readindex]);
				} else {
					int readindex = (i-1)*2;
					int writeindex = readindex+3;
					dup2(pipeline[readindex], 0);
					dup2(pipeline[writeindex], 1);
				
					for (int j = 0; j < numPipes*2; j++) {
						if (j != readindex && j != writeindex) {
							close(pipeline[j]);
						}
					}
						
					exitStatus = runCommand(pipes[i]);
					close(pipeline[readindex]);
					close(pipeline[writeindex]);

				} 
				
				exit(exitStatus);
			}

			printf("PID %5d, exit status: %d\n", child, exitStatus);
		}
		/* Close the pipes and wait for the children to finish */
		for (int i = 0; i < numPipes*2; i++) {
			close(pipeline[i]);
		}
		for (int i = 0; i < numCommands; i++) {
			wait(&exitStatus);
		}
	
	if (exitStatus != -1) {
		exitStatus = 127;
	}
	add_history(copy, exitStatus);
	}
}

/**
 * Applies the I/O redirect, if any, and runs the command.
 */
int runCommand(char* str) {
	char filename[MAXFILENAME];
	int red = parseRedirect(str, filename);
	if (red == IN_REDIRECT) {
		int infd = open(filename, O_RDONLY, 0600);
		int stdin_saved = dup(0);
		close(0);
		dup2(infd, 0);
		int exitStatus = executeCommand(str);
		dup2(stdin_saved, 0);
		close(stdin_saved);
		return exitStatus;
	} else if (red == OUT_REDIRECT) {
		int outfd = open(filename, O_CREAT|O_RDWR, 0600);
		int stdout_saved = dup(1);
		close(1);
		dup2(outfd, 1);
		close(outfd);
		int exitStatus = executeCommand(str);
		dup2(stdout_saved, 1);
		close(stdout_saved);
		return exitStatus;
	} else {
		int exitStatus = executeCommand(str);
		return exitStatus;
	}

	/* Cleaning up */
	fflush(stdin);
	fflush(stdout);

	for (int i = 0; i < strlen(filename); i++) {
		filename[i] = '\0';
	}
}

/**
 * Parser for I/O redirects. This places a NULL after the command
 * part of the string to remove the redirect char and file name.
 */
int parseRedirect(char* str, char* filename) {
	for (int i = 0; i < strlen(str); i++) {
		if (str[i] == '<') {
			str[i] = '\0';
			while (str[i+1] == ' ') {
				i++;
			}
			
			/* Trim leading white space */
			while (str[i+1] == ' ') {
				i++;
			}
			strncat(filename, &str[i+1], strlen(&str[i+1]));

			/* Trim trailing white space */
			for (int j = 0; j < strlen(filename); j++) {
				if (filename[j] == ' ') {
					filename[j] = '\0';
				}
			}
		
			return IN_REDIRECT;
		}
		
		if (str[i] == '>') {
			str[i] = '\0';
		
			/* Trim leading white space */
			while (str[i+1] == ' ') {
				i++;
			}
			strncat(filename, &str[i+1], strlen(&str[i+1]));
		
			/* Trim trailing white space */
			for (int j = 0; j < strlen(filename); j++) {
				if (filename[j] == ' ') {
					filename[j] = '\0';
				}
			}
		
			return OUT_REDIRECT;
		}
	}
	return NO_REDIRECT;
}

int executeCommand(char* str) {
	
	char* args[MAXLINE];				// array to store argument strings
	
	char* token = strtok(str, " ");			// token for the first arg
	
	/* Putting the args into an args array */
	int stepindex = 0;
	while (token != NULL) {
		args[stepindex] = token;
		stepindex++;
		token = strtok(NULL, " ");		// get the next token
	}
	args[stepindex] = NULL;
	
	/* Internal commands */
	if (strcmp(args[0], "exit") == 0) {
		free(copy);
		clear_history();
		exit(0);
	} else if (strcmp(args[0], "cd") == 0) {
		if (stepindex == 2) {
			int cdir = chdir(args[1]);

			if (cdir != 0) {
				add_history(copy, 1);
				perror(args[stepindex]);
				return -1;
			} else {
				add_history(copy, 0);
				fprintf(stdout, "%s\n", args[1]);
				return 0;
			} 
		} else {
			fprintf(stderr, "Invalid command\n");
			add_history(copy, 1);
			return 1;
		}
	} else if (strcmp(args[0], "history") == 0) {
		
		print_history(firstSequenceNumber);
		add_history(copy, 0);
		return 0;

	/* External commands */
	} else {
		int extCmd = executeExternalCommand(args);
		add_history(copy, extCmd);
		return extCmd;
	}

	
}

int executeExternalCommand(char* args[1026]) { // return status
	
	int pid = fork();

	/** Forked processes **/
	
	/* Child process */
	if (pid == 0) {		
		execvp(args[0], args);	// first parameter is the file descriptor (command), second is args array
		perror(args[0]);
		exit(127);
	
	/* Parent process */
	} else if (pid > 0) {
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
