#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


void removeChar(char *str, char *remove);
int countForParsing(char *str, char *charToCount);
void parseCommand(char *lineToParse, char *passedParsedCommand[], char *passedCommand, int numberOfCommandElements);
void parsePath(char *pathVariable, char *passedParsedPath[], int numberOfPathElements);
void combinePathAndCommand(char *passedParsedCommand[], char *passedParsedPath[], int parsedPathIndex);
char* moveCurrentDirBack(char *str);
void shiftCommandToRedirectCommand(char *passedParsedCommand[], char *passedParsedRedirectionCommand[], int redirectionPositionIndex);
void findAmpersandPosition(char *passedParsedCommand[], int passedAmpersandPositions[],
			   int numberOfAmpersands, int numberOfCommandElements);
void errorMessage();

int main(int argc, char **argv)
{
	FILE *fp;
	char *line = NULL;
	size_t linesize = 0;
	ssize_t linelen;
	ssize_t fp_linesize = 0;
	ssize_t fp_linelen;
	int exec_error = 0;
	int accept_error = 0;

	char *path = (char *)malloc(100);
	strcpy(path, "/bin");
	int redirectionFound = 0;

	int redirectPosition = 0;
	char *command = (char *)malloc(100);
	char *pathArgs = (char *)malloc(100);
	char *currentDir = (char *)malloc(1000);
	strcpy(currentDir,"/home");

	if(argc == 1) {
		printf("rysh> ");
		chdir(currentDir);
		while ((linelen = getline(&line, &linesize, stdin)) != -1) {

			int numOfAmpersands = countForParsing(line, "&") + 2;
			int ampersandPositions[numOfAmpersands];

			int numOfCommElems = countForParsing(line, " ") + 2;
			char *parsedComm[numOfCommElems];
			for(int i = 0; i < numOfCommElems; i++) {
				parsedComm[i] = (char *)malloc(100);
			}

			int numOfPathElems;
			
			if(strlen(path) == 0) {
				numOfPathElems = 0;
			}
			else {
				numOfPathElems = countForParsing(path, " ") + 1;
			}
			char *parsedPath[numOfPathElems];
			for(int i = 0; i < numOfPathElems; i++) {
				parsedPath[i] = (char *)malloc(100);
			}
		
			removeChar(line, "\n");
			removeChar(line, "\t");
		
			parseCommand(line, parsedComm, command, numOfCommElems);
			parsePath(path, parsedPath, numOfPathElems);

			findAmpersandPosition(parsedComm, ampersandPositions, numOfAmpersands, numOfCommElems);

			if(countForParsing(line, ">") > 1) {
				errorMessage();
			}

			if (strncmp("exit", line, 4) == 0) {
				// user wants to exit
				if(strncmp("exit\0", line, 5) != 0) {
					errorMessage();
				}
				else {
					for(int i = 0; i < numOfCommElems; i++) {
						free(parsedComm[i]);
					}
					exit(0);
				}
			}
			else if (strncmp("cd", line, 2) == 0) {
				if(strncmp("cd\0", line, 3) == 0) {}
				else if(strncmp("..", parsedComm[1], 2) == 0) {			//enter "cd .." to move up 1 directory
					currentDir = moveCurrentDirBack(currentDir);
					if(chdir(currentDir) == -1) {errorMessage();}
				} 
				else {
					strcat(currentDir, "/");
					strcat(currentDir, parsedComm[1]);
					if(chdir(currentDir) == -1) {errorMessage();}
				}
			}
			else if (strncmp("path", line, 4) == 0) {
				if(strncmp("path\0", line, 5) == 0) {
					strcpy(path, "");
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
			
					for(int j = 0; j < numOfAmpersands - 1; j++) {
						int rc2 = fork();
						int k = 0;
						int parsedAmpCommSize = ampersandPositions[j + 1] - ampersandPositions[j];
						int parsedAmpIndexBegin = ampersandPositions[j];
						int parsedAmpIndexEnd = ampersandPositions[j + 1] - 1;

						char *parsedAmpComms[parsedAmpCommSize];
						for(int i = 0; i < parsedAmpCommSize; i++) {
							parsedAmpComms[i] = (char *)malloc(100);
						}
				
						for(int i = parsedAmpIndexBegin; i < parsedAmpIndexEnd; i++) {
					
							if(k < parsedAmpCommSize) {
								strcpy(parsedAmpComms[k++], parsedComm[i]);
							}
						}
						k = 0;
						parsedAmpComms[parsedAmpCommSize - 1] = NULL;

						int saved_stdout = dup(STDOUT_FILENO);

						for(int i = 0; i < parsedAmpCommSize - 1; i++) {
							if(strncmp(">", parsedAmpComms[i], 1) == 0) {
								redirectPosition = i;
								redirectionFound = 1;
							}
						}
				
						if(redirectionFound == 1) {
							int out = open(parsedAmpComms[parsedAmpCommSize - 2], O_TRUNC | O_RDWR |
													      O_CREAT, S_IRWXU);
							dup2(out, STDOUT_FILENO);
							close(out);
							redirectionFound = 0;
						}				

						redirectPosition++;
						if(redirectPosition == 1) redirectPosition = parsedAmpCommSize;

						char *parsedRedirectComm[redirectPosition];
						for(int i = 0; i < redirectPosition; i++) {
							parsedRedirectComm[i] = (char *)malloc(100);
						}

						shiftCommandToRedirectCommand(parsedAmpComms, parsedRedirectComm, redirectPosition);

						for(int i = 0; i < numOfPathElems; i++) {
							combinePathAndCommand(parsedRedirectComm, parsedPath, i);
							if(access(parsedRedirectComm[0], X_OK) == 0) {
								if(rc2 == 0) {
									if(execv(parsedRedirectComm[0], parsedRedirectComm) != -1) {
										errorMessage();
									}
								}
								else {
									int rc_wait2 = wait(NULL);
								}
							}
							else {
								accept_error++;
								if(accept_error == (numOfPathElems - 1)) {
									errorMessage();
									accept_error = 0;
								}
							}
							
							strcpy(parsedRedirectComm[0], command);
						}
						dup2(saved_stdout, STDOUT_FILENO);
						close(saved_stdout);
						redirectPosition = 0;

						for(int i = 0; i < parsedAmpCommSize; i++) {
							free(parsedAmpComms[i]);
						}
						for(int i = 0; i < redirectPosition; i++) {
							free(parsedRedirectComm[i]);
						}
					}
				exit(0);

				} else {
					int rc_wait = wait(NULL);
				}
			}

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
	else if(argc == 2) {
		chdir(currentDir);

		fp = fopen(argv[1], "r");
		if(fp == NULL) exit(EXIT_FAILURE);
		while ((fp_linelen = getline(&line, &fp_linesize, fp)) != -1) {

			int numOfAmpersands = countForParsing(line, "&") + 2;
			int ampersandPositions[numOfAmpersands];

			int numOfCommElems = countForParsing(line, " ") + 2;
			char *parsedComm[numOfCommElems];
			for(int i = 0; i < numOfCommElems; i++) {
				parsedComm[i] = (char *)malloc(100);
			}

			int numOfPathElems;
			
			if(strlen(path) == 0) {
				numOfPathElems = 0;
			}
			else {
				numOfPathElems = countForParsing(path, " ") + 1;
			}
			char *parsedPath[numOfPathElems];
			for(int i = 0; i < numOfPathElems; i++) {
				parsedPath[i] = (char *)malloc(100);
			}
		
			removeChar(line, "\n");
			removeChar(line, "\t");
		
			parseCommand(line, parsedComm, command, numOfCommElems);
			parsePath(path, parsedPath, numOfPathElems);

			findAmpersandPosition(parsedComm, ampersandPositions, numOfAmpersands, numOfCommElems);
			if(countForParsing(line, ">") > 1) {
				errorMessage();
			}

			if (strncmp("exit", line, 4) == 0) {
				// user wants to exit
				if(strncmp("exit\0", line, 5) != 0) {
					errorMessage();
				}
				else {
					for(int i = 0; i < numOfCommElems; i++) {
						free(parsedComm[i]);
					}
					exit(0);
				}
			}
			else if (strncmp("cd", line, 2) == 0) {
				if(strncmp("..", parsedComm[1], 2) == 0) {
					currentDir = moveCurrentDirBack(currentDir);
					if(chdir(currentDir) == -1) {errorMessage();}
				} else {
					strcat(currentDir, "/");
					strcat(currentDir, parsedComm[1]);
					if(chdir(currentDir) == -1) {errorMessage();}
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
			
					for(int j = 0; j < numOfAmpersands - 1; j++) {
						int rc2 = fork();
						int k = 0;
						int parsedAmpCommSize = ampersandPositions[j + 1] - ampersandPositions[j];
						int parsedAmpIndexBegin = ampersandPositions[j];
						int parsedAmpIndexEnd = ampersandPositions[j + 1] - 1;

						char *parsedAmpComms[parsedAmpCommSize];
						for(int i = 0; i < parsedAmpCommSize; i++) {
							parsedAmpComms[i] = (char *)malloc(100);
						}
				
						for(int i = parsedAmpIndexBegin; i < parsedAmpIndexEnd; i++) {
					
							if(k < parsedAmpCommSize) {
								strcpy(parsedAmpComms[k++], parsedComm[i]);
							}
						}
						k = 0;
						parsedAmpComms[parsedAmpCommSize - 1] = NULL;

						int saved_stdout = dup(STDOUT_FILENO);

						for(int i = 0; i < parsedAmpCommSize - 1; i++) {
							if(strncmp(">", parsedAmpComms[i], 1) == 0) {
								redirectPosition = i;
								redirectionFound = 1;
							}
						}
				
						if(redirectionFound == 1) {
							int out = open(parsedAmpComms[parsedAmpCommSize - 2], O_TRUNC | O_RDWR |
													      O_CREAT, S_IRWXU);
							dup2(out, STDOUT_FILENO);
							close(out);
							redirectionFound = 0;
						}				

						redirectPosition++;
						if(redirectPosition == 1) redirectPosition = parsedAmpCommSize;

						char *parsedRedirectComm[redirectPosition];
						for(int i = 0; i < redirectPosition; i++) {
							parsedRedirectComm[i] = (char *)malloc(100);
						}

						shiftCommandToRedirectCommand(parsedAmpComms, parsedRedirectComm, redirectPosition);

						for(int i = 0; i < numOfPathElems; i++) {
							combinePathAndCommand(parsedRedirectComm, parsedPath, i);
							if(access(parsedRedirectComm[0], X_OK) == 0) {
								if(rc2 == 0) {
									int exec_error = execv(parsedRedirectComm[0], parsedRedirectComm);
									if(execv(parsedRedirectComm[0], parsedRedirectComm) != -1) {
										errorMessage();
									}
								}
								else {
									int rc_wait2 = wait(NULL);
								}
							}
							else {
								accept_error++;
								if(accept_error == (numOfPathElems - 1)) {
									errorMessage();
									accept_error = 0;
								}
							}
							strcpy(parsedRedirectComm[0], command);
						}
						if(numOfPathElems == 0) {
							errorMessage();
						}
						dup2(saved_stdout, STDOUT_FILENO);
						close(saved_stdout);
						redirectPosition = 0;
						if(exec_error == -1) printf("execv error occurred\n");

						for(int i = 0; i < parsedAmpCommSize; i++) {
							free(parsedAmpComms[i]);
						}
						for(int i = 0; i < redirectPosition; i++) {
							free(parsedRedirectComm[i]);
						}
					}
				exit(0);

				} else {
					int rc_wait = wait(NULL);
				}
			}

			for(int i = 0; i < numOfCommElems; i++) {
				free(parsedComm[i]);
			}
			for(int i = 0; i < numOfPathElems; i++) {
				free(parsedPath[i]);
			}
		
		}
		fclose(fp);
	
		free(path);
		free(command);
		free(pathArgs);
		free(currentDir);
		free(line);
		if (ferror(stdin))
			err(1, "getline");
		exit(0);
	}
	else {
		errorMessage();
		exit(1);
	}
}

void removeChar(char *str, char *remove) {
	char *src, *dst;
	for(src = dst = str; *src != '\0'; src++) {
		*dst = *src;
		if(*dst != *remove) dst++;
	}
	*dst = '\0';
}

int countForParsing(char *str, char *charToCount) {
	int argumentCount = 0;
	char *charIndex;
	for(charIndex = str; *charIndex != '\0'; charIndex++) {
		if(*charIndex == *charToCount) argumentCount++;
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

void findAmpersandPosition(char *passedParsedCommand[], int passedAmpersandPositions[],
			   int numberOfAmpersands, int numberOfCommandElements) {
	int ampersandPositionIndex = 1;
	passedAmpersandPositions[0] = 0;
	for(int i = 0; i < numberOfCommandElements - 1; i++) {
		if((strncmp("&", passedParsedCommand[i], 1) == 0) && ampersandPositionIndex < numberOfAmpersands - 1) {
			passedAmpersandPositions[ampersandPositionIndex++] = i + 1;
		}
	}
	passedAmpersandPositions[numberOfAmpersands - 1] = numberOfCommandElements;
}

void errorMessage() {
	char error_message[30] = "An error has occurred\n";
	write(STDERR_FILENO, error_message, strlen(error_message));
}
