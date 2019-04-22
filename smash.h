/**
 * Function prototype for the executeCommand function.
 * This function will handle parsing the command string.
 */
#define MAXLINE 4096
void executeCommand(char* str);
int executeExternalCommand(char* args[1024]);
char** removeToken(char** arr, int index);
int parsePipes(char* str, char** pipes);
void runCommand(char* str);
int parseRedirects(char* str, char* filename);
