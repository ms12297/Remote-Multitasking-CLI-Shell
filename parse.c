#include "parse.h"
#include <stdio.h> // for printf
#include <unistd.h> // for fork
#include <stdlib.h> // for exit
#include <sys/wait.h> // for wait
#include <sys/types.h> // for pid_t
#include <fcntl.h> // for flags
#include <sys/stat.h> // for permission flags
#include <string.h> // for strtok


void printAll() {
    printf("Welcome to our mini-shell!\n");
    printf("You may use the following commands:\n\n");
    // pwd ls cat grep less touch rm mkdir rmdir date find mv sort tail ps
    printf("pwd: print working directory\n");
    printf("ls: list directory contents\n");
    printf("cat: print on the standard output\n");
    printf("grep: print lines matching a pattern\n");
    printf("less: interactive file viewer\n");
    printf("touch: create a file\n");
    printf("rm: remove files or directories\n");
    printf("mkdir: create directories\n");
    printf("rmdir: remove directories\n");
    printf("date: print system date and time\n");
    printf("find: search for files in a directory hierarchy\n");
    printf("mv: move or rename files\n");
    printf("sort: sort lines of text files\n");
    printf("tail: output the last part of files\n");
    printf("ps: report a snapshot of the current processes\n");
    printf("clear: clear the terminal screen\n");
    printf("exit: exit the shell\n");
    printf("\nYou may also use the following redirections:\n");
    printf("<\n");
    printf(">\n");
    printf("2>\n");
    printf("With piped commands, the only redirections allowed are \"<\" in the first command and \">\" or \"2>\" in the last command, with no redirections in between.\n");
    printf("\nHere are the piped commands we support:\n");
    printf("find name | xargs cat : display the contents of all files/folders found with specified name using find\n");
    printf("ls -l | grep .txt | wc -l : lists files with a .txt extension and count them\n");
    printf("ps aux | grep user | sort -h | tail -n 1 : display the process with the highest memory usage for a specific user\n");
    printf("\nYou may use up to 3 pipes and any valid combination of redirection\n\n");
}

char* getUserInput() { // get user input
    char* input = NULL; // initialize input to NULL
    size_t bufsize = 0; // initialize bufsize to 0

    printf("$ "); // print prompt
    ssize_t bytesRead = getline(&input, &bufsize, stdin); // read input from stdin and dynamically allocate memory for input

    if (bytesRead == -1) { // if error reading input, print error message and exit
        perror("Error reading input"); 
        exit(EXIT_FAILURE);
    }

    if (input[bytesRead - 1] == '\n') { // if input ends with newline, replace newline with null character
        input[bytesRead - 1] = '\0';
    }

    return input; // return input
}

int countPipes(char *input) { // count number of pipes in input
    int count = 0;
    for (int i = 0; input[i] != '\0'; i++) { // loop through input
        if (input[i] == '|') { // if pipe is found, increment count
            count++;
        }
        // check for cases of redirection other than <, >, or 2> and print error message
        if (input[i] == '<' || input[i] == '>') {
            if (input[i + 1] == '>' || input[i + 1] == '<') {
                return -1;
            }
        }
    }
    return count;
}

char** parseCMD(char *input) { // parse input into commands separated by pipes
    int numPipes = countPipes(input); // count number of pipes in input
    char **cmd = (char **)malloc((numPipes + 1) * sizeof(char *)); // allocate memory for commands
    char *token = strtok(input, "|"); // tokenize input by pipes
    int i = 0; // initialize index to 0
 
    while (token != NULL && i < numPipes + 1) { // loop through tokens and store in cmd
        cmd[i] = strdup(token); // strdup duplicates the string token and stores it in cmd
        token = strtok(NULL, "|"); // continue tokenizing input by pipes
        i++; // increment index
    }
    return cmd; // return cmd
}

char** parseARGS(char *cmd) { // parse command into arguments
    char **args = (char **)malloc(8 * sizeof(char *)); // allocate memory for arguments (max 8 arguments)
    char *token = strtok(cmd, " "); // tokenize command by spaces
    int i = 0; // initialize index to 0

    while (token != NULL) { // loop through tokens and store in args
        args[i] = strdup(token); // strdup duplicates the string token and stores it in args
        token = strtok(NULL, " "); // continue tokenizing command by spaces
        i++; // increment index
    }
    args[i] = NULL; // set last argument to NULL
    return args; // return args
}

void parseRedir(char*** args, int* numPipes, char*** flags, char*** files) {

    // allocate memory for flags and files
    *flags = (char **)malloc((*numPipes + 1) * sizeof(char *));
    *files = (char **)malloc((*numPipes + 1) * sizeof(char *));

    // initialize flags and files to NULL
    for (int i = 0; i < *numPipes + 1; i++) {
        (*flags)[i] = NULL;
        (*files)[i] = NULL;
    }
    
    int numFlags = 0; // initialize number of flags to 0
    for (int i = 0; i < *numPipes + 1; i++) { // loop through args
        int j = 0;
        while (args[i][j] != NULL) {
            if (strcmp(args[i][j], "<") == 0 || strcmp(args[i][j], ">") == 0 || strcmp(args[i][j], "2>") == 0) { // if redirection is found, increment numFlags
                numFlags++;
            }
            j++;
        }
    }

    if (numFlags == 0) { // if no redirection, return
        // printf("No redirection\n");
        return;
    }

    for (int i = 0; i < *numPipes + 1; i++) { // loop through args
        int j = 0;
        while (args[i][j] != NULL) { // loop through args[i]
            if (strcmp(args[i][j], "<") == 0 || strcmp(args[i][j], ">") == 0 || strcmp(args[i][j], "2>") == 0) { // if redirection is found, store flag and file
                (*flags)[i] = strdup(args[i][j]); // store flag
                (*files)[i] = strdup(args[i][j + 1]); // store file
                args[i][j] = NULL; // remove flag from args
                args[i][j + 1] = NULL; // remove file from args
            }
            j++;
        }
    }    
}
