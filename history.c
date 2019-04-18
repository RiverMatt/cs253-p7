#include <stdio.h>
#include <string.h>
#include "history.h"
#include <stdlib.h>

static struct Cmd* hist;	//pointer to a Cmd struct called hist
static int counter = 0;	//counter for the number of history entries

void init_history(void) {			//Builds data structures for recording cmd history
	hist = (struct Cmd*) malloc(sizeof(struct Cmd) * MAXHISTORY);
}

void add_history(char *cmd, int exitStatus) {	//Adds an entry to the history
	
	if (counter >= MAXHISTORY) {
		free(hist[0].cmd);
		/* Shift all entries down one */
		for (int i = 0; i < MAXHISTORY - 1; i++) {
			hist[i].cmd = hist[i+ 1].cmd;
			hist[i].exitStatus = hist[i + 1].exitStatus;

		}
		hist[MAXHISTORY - 1].cmd = cmd;
		hist[MAXHISTORY - 1].exitStatus = exitStatus;

	} else {

		hist[counter].cmd = cmd;
		hist[counter].exitStatus = exitStatus;

		counter++;
	
	}
}

void clear_history(void) {			//Frees all malloc’d memory in the history
	for (int i = 0; i < MAXHISTORY; i++) {
		free(hist[i].cmd);
	}
	free(hist);
}

void print_history(int firstSequenceNumber) {	//Prints the history to stdout
	for (int i = 0; i < counter; i++) {
		printf("%d [%d] %s\n", firstSequenceNumber++, hist[i].exitStatus, hist[i].cmd);
	}
}
