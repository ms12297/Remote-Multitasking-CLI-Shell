#ifndef EXECUTE_H
#define EXECUTE_H

int redirHandle(char* flag, char* file); // handle redirection
char* singleCMDArgs(char** args, char** flag, char** file); // execute single command
char* onePipe(char*** args, char** flag, char** file); // execute one pipe
char* twoPipe(char*** args, char** flag, char** file); // execute two pipes 
char* threePipe(char*** args, char** flag, char** file); // execute three pipes

#endif
