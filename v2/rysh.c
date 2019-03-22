#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
//#define argSize 2


void removeChar(char *str, char *remove);
int countNumOfArgs(char *str);
void parseCommand(char *typedLine, char *originalPathVariable, char *subArgument,
		  char *fullArgument, char *newPathVariable, char *parsedArgument[], int argumentSize);

int main(int argc, char **argv)
{
	char *line = NULL;
	size_t linesize = 0;
	ssize_t linelen;

	char *path = "/bin";
	char *argex[] = {"/bin/ls",NULL};

	char *subArg = (char *)malloc(100);
	char *fullArg = (char *)malloc(100);
	char *newPath = (char *)malloc(100);

	printf("rysh> ");
	while ((linelen = getline(&line, &linesize, stdin)) != -1) {

		int argSize = countNumOfArgs(line);
		//printf("argSize = %d\n", argSize);
		char *arg[argSize];
		for(int i = 0; i < argSize; i++) {
			arg[i] = (char *)malloc(100);
		}
		
		removeChar(line, "\n");
		if (strncmp("exit", line, 4) == 0) {
			// user wants to exit
			for(int i = 0; i < argSize; i++) {
				free(arg[i]);
			}
			exit(0);
		}
		else if (strncmp("cd", line, 2) == 0) {
			printf("cd read\n");
		}
		else {
			int rc = fork();
			if(rc < 0) {
				printf("Fork Failed.\n");
			} else if(rc == 0) {
				parseCommand(line, path, subArg, fullArg, newPath, arg, argSize);
				for(int i = 0; i < argSize; i++) {
					printf("arg[%d] = %s\n", i, arg[i]);
				}
				int z = execv(arg[0], arg);
				if(z == -1) printf("execv error occurred\n");
				//execv(argex[0], argex);
				for(int i = 0; i < argSize; i++) {
					free(arg[i]);
				}
				exit(0);
			} else {
				int rc_wait = wait(NULL);
				//printf("Hello, I am parent of %d (rc_wait: %d) (pid: %d)\n", rc, rc_wait, (int) getpid());
			}
		}

		//fwrite(line, linelen, 1, stdout);
			// fork
			// if child, exec
			// if parent, wait
		for(int i = 0; i < argSize; i++) {
			free(arg[i]);
		}
		printf("rysh> ");
		
	}
	
	free(subArg);
	free(fullArg);
	free(newPath);
	free(line);
	if (ferror(stdin))
		err(1, "getline");
}

void removeChar(char *str, char *remove) {
	char *src, *dst;
	for(src = dst = str; *src != '\0'; src++) {
		*dst = *src;
		if(*dst != *remove) dst++;
	}
	*dst = '\0';
}

int countNumOfArgs(char *str) {
	int argumentCount = 2;
	char *charIndex;
	char *charToRemove = " ";
	for(charIndex = str; *charIndex != '\0'; charIndex++) {
		if(*charIndex == *charToRemove) argumentCount++;
	}
	return argumentCount;
}

void parseCommand(char *typedLine, char *originalPathVariable, char *subArgument,
		  char *fullArgument, char *newPathVariable, char *parsedArgument[], int argumentSize) {
	int i = 1;	
	strcpy(fullArgument, typedLine);
	subArgument = strsep(&fullArgument, " ");
	strcpy(newPathVariable, originalPathVariable);
	strcat(newPathVariable, "/");
	strcat(newPathVariable, subArgument);
	strcpy(parsedArgument[0], newPathVariable);
	while(((subArgument = strsep(&fullArgument, " ")) != NULL) && i < argumentSize - 1) {
		strcpy(parsedArgument[i++],subArgument);
	}
	parsedArgument[argumentSize - 1] = NULL;
}

