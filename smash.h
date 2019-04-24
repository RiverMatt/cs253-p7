/**
 * Function prototype for the executeCommand function.
 * This function will handle parsing the command string.
 */
#define MAXLINE 4096
void executeCommand(char* str);
int executeExternalCommand(char* args[1024]);
void parsePipes(char* str);
void applyRedirect(char* str);
int parseRedirect(char* str, char* filename);
