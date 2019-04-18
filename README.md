
NAME
	smash

SYNOPSIS
	#include <stdio.h>
	#include <string.h>
	#include "smash.h"
	#include "history.h"
	#define MAXLINE 4096
	void executeCommand(char* str);
	int executeExternalCommand(char* args[1024]);

DESCRIPTION
	smash is a shell that allows execution of commands. There are three build in commands, cd <directory>, history,
	and exit. cd <directory> will change the current working directory to the desired directory. The history command
	displays a history of the last ten commands entered and an integer representing their process type. The exit 
	command closes the shell and exits the program.
	Outside of these commands, smash allows the execution of outside bash commands, and will return	a message if 
	an invalid command is entered or the external process fails.

AUTHOR
	Matt Arnold
