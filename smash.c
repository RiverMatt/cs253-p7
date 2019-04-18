#include <stdio.h>
#include <string.h>
#include "smash.h"
#include "history.h"

#define MAXLINE 4096
int main(void) {
	char bfr[MAXLINE];
	fputs("$ ", stderr);
	init_history();
	setvbuf(stdin, NULL, _IONBF, 0);
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	while (fgets(bfr, MAXLINE, stdin) != NULL) {
		bfr[strlen(bfr) - 1] = '\0'; 		// replace the newline char with NUL
		if (strlen(bfr) != 0) {
			executeCommand(bfr);
		}
		fputs("$ ", stderr);
	}

	return 0;
}
