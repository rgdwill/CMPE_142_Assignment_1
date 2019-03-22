#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
//#define argSize 2


void removeChar(char *str, char *remove);
int countForParsing(char *str);
void parseCommand(char *lineToParse, char *passedParsedCommand[], char *passedCommand, int numberOfCommandElements);
void parsePath(char *pathVariable, char *passedParsedPath[], int numberOfPathElements);
void combinePathAndCommand(char *passedParsedCommand[], char *passedParsedPath[], int parsedPathIndex);
void shiftCommandToRedirectCommand(char *passedParsedCommand[], char *passedParsedRedirectionCommand[], int redirectionPositionIndex);
char* moveCurrentDirBack(char *str);

int main(int argc, char **argv)
{
	char *line = NULL;
	size_t linesize = 0;
	ssize_t linelen;

	char *path = (char *)malloc(100);
	strcpy(path, "/bin");
	char *argex[] = {"/bin/ls",NULL};
	int redirectionFound = 0;

	int redirectPosition = 0;
	char *command = (char *)malloc(100);
	char *pathArgs = (char *)malloc(100);
	char *currentDir = (char *)malloc(500);
	strcpy(currentDir,"/home/ryan/Documents/CMPE_142/CMPE_142_Assignment_#1/v5");

	printf("rysh> ");
	chdir(currentDir);
	while ((linelen = getline(&line, &linesize, stdin)) != -1) {

		int numOfCommElems = countForParsing(line) + 2;
		//printf("numOfCommElems = %d\n", numOfCommElems);
		char *parsedComm[numOfCommElems];
		for(int i = 0; i < numOfCommElems; i++) {
			parsedComm[i] = (char *)malloc(100);
		}

		int numOfPathElems = countForParsing(path) + 1;
		//printf("numOfPathElems = %d\n", numOfPathElems);
		char *parsedPath[numOfPathElems];
		for(int i = 0; i < numOfPathElems; i++) {
			parsedPath[i] = (char *)malloc(100);
		}
		
		removeChar(line, "\n");

		parseCommand(line, parsedComm, command, numOfCommElems);
		parsePath(path, parsedPath, numOfPathElems);
		//printf("parsedPath[0] = %s\n", parsedPath[0]);
		//printf("parsedPath[1] = %s\n", parsedPath[1]);
		//printf("parsedPath[2] = %s\n", parsedPath[2]);

		if (strncmp("exit", line, 4) == 0) {
			// user wants to exit
			for(int i = 0; i < numOfCommElems; i++) {
				free(parsedComm[i]);
			}
			exit(0);
		}
		else if (strncmp("cd", line, 2) == 0) {
			if(strncmp("..", parsedComm[1], 2) == 0) {
				currentDir = moveCurrentDirBack(currentDir);
				chdir(currentDir);
			} else {
				strcat(currentDir, "/");
				strcat(currentDir, parsedComm[1]);
				chdir(currentDir);
			}
		}
		else if (strncmp("path", line, 4) == 0) {
			if(strncmp("path\0", line, 5) == 0) {
				strcpy(path, " ");
			}
			else {
				pathArgs = strchr(line, ' ');
				strcpy(path, pathArgs);
			}
		}
		else {
			int rc = fork();
			if(rc < 0) {
				printf("Fork Failed.\n");
			} else if(rc == 0) {
				//parseCommand(line, path, subArg, fullArg, newPath, arg, argSize);
				//printf("numOfCommElems = %d\n", numOfCommElems);
				/*for(int i = 0; i < numOfCommElems; i++) {
					printf("parsedComm[%d] = %s\n", i, parsedComm[i]);
				}*/
				int saved_stdout = dup(STDOUT_FILENO);
				//printf("parsedComm[last elem] = %s\n", parsedComm[numOfCommElems - 2]);

				for(int i = 0; i < numOfCommElems - 1; i++) {
					if(strncmp(">", parsedComm[i], 1) == 0) {
						redirectPosition = i;
						redirectionFound = 1;
					}
				}
				
				if(redirectionFound == 1) {
					int out = open(parsedComm[numOfCommElems - 2], O_TRUNC | O_RDWR | O_CREAT, S_IRWXU);
					dup2(out, STDOUT_FILENO);
					close(out);
					redirectionFound = 0;
				}				

				redirectPosition++;
				if(redirectPosition == 1) redirectPosition = numOfCommElems;

				char *parsedRedirectComm[redirectPosition];
				for(int i = 0; i < redirectPosition; i++) {
					parsedRedirectComm[i] = (char *)malloc(100);
				}

				shiftCommandToRedirectCommand(parsedComm, parsedRedirectComm, redirectPosition);
				/*for(int i = 0; i < redirectPosition; i++) {
					printf("parsedRedirectComm[%d] = %s\n", i, parsedRedirectComm[i]);
				}*/

				for(int i = 0; i < numOfPathElems; i++) {
					combinePathAndCommand(parsedRedirectComm, parsedPath, i);
					if(access(parsedRedirectComm[0], X_OK) == 0) {
						execv(parsedRedirectComm[0], parsedRedirectComm);
					}
					strcpy(parsedRedirectComm[0], command);
				}
				dup2(saved_stdout, STDOUT_FILENO);
				close(saved_stdout);
				//if(z == -1) printf("execv error occurred\n");

				//execv(argex[0], argex);

				for(int i = 0; i < numOfCommElems; i++) {
					free(parsedComm[i]);
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
		for(int i = 0; i < numOfCommElems; i++) {
			free(parsedComm[i]);
		}
		for(int i = 0; i < numOfPathElems; i++) {
			free(parsedPath[i]);
		}
		printf("rysh> ");
		
	}
	
	free(path);
	free(command);
	free(pathArgs);
	free(currentDir);
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

int countForParsing(char *str) {
	int argumentCount = 0;
	char *charIndex;
	char *charToRemove = " ";
	for(charIndex = str; *charIndex != '\0'; charIndex++) {
		if(*charIndex == *charToRemove) argumentCount++;
	}
	return argumentCount;
}

void parseCommand(char *lineToParse, char *passedParsedCommand[], char *passedCommand, int numberOfCommandElements) {
	int parseIndex = 0;
	char *subCommand = (char *)malloc(100);
	char *commandVarToParse = (char *)malloc(100);
	strcpy(commandVarToParse, lineToParse);
	while(((subCommand = strsep(&commandVarToParse, " ")) != NULL) && parseIndex < numberOfCommandElements - 1) {
		strcpy(passedParsedCommand[parseIndex++], subCommand);
	}
	passedParsedCommand[numberOfCommandElements - 1] = NULL;
	strcpy(passedCommand, passedParsedCommand[0]);
	free(subCommand);
	free(commandVarToParse);
}

void parsePath(char *pathVariable, char *passedParsedPath[], int numberOfPathElements) {
	int parseIndex = 0;
	char *subPath = (char *)malloc(100);
	char *pathVarToParse = (char *)malloc(100);
	strcpy(pathVarToParse, pathVariable);
	while(((subPath = strsep(&pathVarToParse, " ")) != NULL) && parseIndex < numberOfPathElements) {
		strcpy(passedParsedPath[parseIndex++], subPath);
	}
	free(subPath);
	free(pathVarToParse);
}

void combinePathAndCommand(char *passedParsedCommand[], char *passedParsedPath[], int parsedPathIndex) {
	char *parsedPathElement = (char *)malloc(100);
	strcpy(parsedPathElement, passedParsedPath[parsedPathIndex]);
	strcat(parsedPathElement, "/");
	strcat(parsedPathElement, passedParsedCommand[0]);
	strcpy(passedParsedCommand[0], parsedPathElement);
	free(parsedPathElement);
}

char* moveCurrentDirBack(char *str) {
	char *strToManipulate = (char *)malloc(100);
	strcpy(strToManipulate, str);
	int charIndex = 0;
	char *currentChar;
	for(currentChar = strToManipulate; *currentChar != '\0'; currentChar++) {
		charIndex++;
	}
	while(*currentChar != '/') {
		charIndex--;
		currentChar--;
	}
	strToManipulate[charIndex] = '\0';
	return strToManipulate;
}

void shiftCommandToRedirectCommand(char *passedParsedCommand[], char *passedParsedRedirectionCommand[], int redirectionPositionIndex) {
	for(int i = 0; i < redirectionPositionIndex - 1; i++) {
		strcpy(passedParsedRedirectionCommand[i], passedParsedCommand[i]);
	}
	passedParsedRedirectionCommand[redirectionPositionIndex - 1] = NULL;
}
