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
	
	if (numPipes == 0) {
		runCommand(pipes[0]);
	} else {
		int stdin_saved = dup(0);
		int stdout_saved = dup(1);
		// what if we set up the pipe here, then only forked one process at a time, as the fds stay active between?

		int pipeFDs[2];
		pipe(pipeFDs);
		
		for (int i = 0; i < numPipes; i++) {
			runPipe(pipes[i], pipeFDs[0], pipeFDs[1]);
		}
		
		close(pipeFDs[0]);
		close(pipeFDs[1]);

		dup2(stdin_saved, 0);
		dup2(stdout_saved, 1);
		fflush(stdin);
		fflush(stdout);

	}
}

void runPipe(char* cmd, int fd0, int fd1) {
	
	int child = fork();
	if (child < 0) {
		perror("child fork failed!");
		exit(-1);
	}
	
	if (child == 0) {
		close(1);
		dup2(fd1, 1);

		close(fd0);
		close(fd1);

		runCommand(cmd);
		exit(127);
	}

	/* Parent */
	int exitStatus;
	wait(&exitStatus);
		
	close(0);
	dup2(fd0, 0);
}

void runPipes(char* cmd0, char* cmd1) {

	/* Pipe File Descriptors */
	int pipeFDs[2];
	
	/* Creating the pipe */
	if (pipe(pipeFDs) < 0) {
		perror("Pipe failed!");
		exit(-1);
	}
	
	/* Fork child0 process */
	int child0 = fork();
	if (child0 < 0) {
		perror("child0 fork failed!");
		exit(-1);
	}
	
	if (child0 == 0) {
		close(1);
		dup2(pipeFDs[1], 1);

		close(pipeFDs[0]);
		close(pipeFDs[1]);

		runCommand(cmd0);
	}

	close(pipeFDs[1]);
	
	/* Fork child1 process */
	int child1 = fork();
	if (child1 < 0) {
		perror("child1 fork failed!");
		exit(-1);
	}

	if (child1 == 0) {
		close(0);
		dup2(pipeFDs[0], 0);

		close(pipeFDs[0]);
		close(pipeFDs[1]);

		runCommand(cmd1);
	}

	/* Parent */
	int exitStatus0, exitStatus1;
	wait(&exitStatus0);
	wait(&exitStatus1);
}

/**
 * Applies the I/O redirect, if any, and runs the command.
 */
void runCommand(char* str) {
	char filename[MAXFILENAME];
	int red = parseRedirect(str, filename);
	if (red == IN_REDIRECT) {
		int infd = open(filename, O_RDONLY, 0600);
		int stdin_saved = dup(0);
		close(0);
		dup2(infd, 0);
		close(infd);
		executeCommand(str);
		dup2(stdin_saved, 0);
		close(stdin_saved);
	} else if (red == OUT_REDIRECT) {
		int outfd = open(filename, O_CREAT|O_RDWR, 0600);
		int stdout_saved = dup(1);
		close(1);
		dup2(outfd, 1);
		close(outfd);
		executeCommand(str);
		dup2(stdout_saved, 1);
		close(stdout_saved);
	} else {
		executeCommand(str);
		
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
//			filename = &str[i+1];
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

void executeCommand(char* str) {
	
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
