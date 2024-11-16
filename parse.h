#ifndef PARSE_H
#define PARSE_H

void printAll(); // print welcome message and list of all commands
char* getUserInput(); // get user input
int countPipes(char *input); // count number of pipes in input
char** parseCMD(char *input); // parse input into commands separated by pipes
char** parseARGS(char *cmd); // parse command into arguments
void parseRedir(char*** args, int* numPipes, char*** flags, char*** files); // parse redirections into flags and files, if any

#endif
