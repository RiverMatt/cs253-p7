#include <stdio.h>
#include <string.h>
#include "smash.h"
#include "history.h"
#include <signal.h>

#define MAXLINE 4096
int main(void) {
	char bfr[MAXLINE];
	fputs("$ ", stderr);
	init_history();
	setvbuf(stdin, NULL, _IONBF, 0);
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
	signal(SIGQUIT,myHandler);
	signal(SIGINT, myHandler);

	while (fgets(bfr, MAXLINE, stdin) != NULL) {
		bfr[strlen(bfr) - 1] = '\0'; 		// replace the newline char with NUL
		if (strlen(bfr) != 0) {
			parsePipes(bfr);
		}
		fputs("$ ", stderr);
	}

	return 0;
}

void myHandler(int _signal) {
	if (_signal == SIGINT) {
		fprintf(stderr, "^C\n");
	}
}
