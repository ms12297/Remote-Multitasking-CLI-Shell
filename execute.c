#include "execute.h"
#include <stdio.h> // for printf
#include <unistd.h> // for fork
#include <stdlib.h> // for exit
#include <sys/wait.h> // for wait
#include <sys/types.h> // for pid_t
#include <fcntl.h> // for flags
#include <sys/stat.h> // for permission flags
#include <string.h> // for strtok


int redirHandle(char* flag, char* file) { // handles redirection, returns -1 if error, 0 if no error

    if (flag == NULL) { // if no redirection, return
        return 0; 
    }

    if (strcmp(flag, ">") == 0) { // if output redirection, open file and redirect stdout
        int fd = open(file, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR); // open file with write only, create if not exist, truncate if exist, user read and write permission
        if (fd == -1) { // if error opening file, return -1
            perror("File opening error");
            return -1;
        }
        dup2(fd, STDOUT_FILENO); // redirect stdout to file
        close(fd); // close file
    }
    else if (strcmp(flag, "2>") == 0) { // if error redirection, open file and redirect stderr
        int fd = open(file, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR); // open file with write only, create if not exist, truncate if exist, user read and write permission
        if (fd == -1) { // if error opening file, return -1
            perror("File opening error");
            return -1;
        }
        dup2(fd, STDERR_FILENO); // redirect stderr to file
        close(fd); // close file
    }
    else if (strcmp(flag, "<") == 0) { // if input redirection, open file and redirect stdin
        int fd = open(file, O_RDONLY); // open file with read only
        if (fd == -1) { // if error opening file, return -1
            perror("File opening error");
            return -1;
        }
        dup2(fd, STDIN_FILENO); // redirect stdin to file
        close(fd); // close file
    }
    return 0;
}


char* singleCMDArgs(char **args, char **flag, char **file)
{

    int out[2]; // pipe to store final output to stdout if no output redirection

    if (pipe(out) == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid;
    pid = fork(); // fork process

    if (pid < 0) { // if fork failed, print error and exit with failure
        printf("Fork failed\n");
        exit(EXIT_FAILURE);
    }
    if (pid == 0) { // if child process

        close(out[0]); // close read end of pipe

        if (args[0][0] == '.' && args[0][1] == '/') { // if executable is in current directory, remove ./ from executable name
            char *name = args[0] + 2; // name of executable without ./ at the beginning

            // counting the number of elements in args
            int count = 0;
            while (args[count] != NULL) {
                count++;
            }

            // constructing a new array with just the executable and arguments
            char *new_args[count + 1]; // add 1 for the final NULL
            new_args[0] = name; // set the executable name without ./ at the beginning for execvp

            for (int i = 1; i < count; i++) { // copy the arguments
                new_args[i] = args[i];
            }

            new_args[count] = NULL; // add the final NULL terminator

            if (flag[0] == NULL) { // if no flag specified, then redirect output to pipe to send back to client
                dup2(out[1], STDOUT_FILENO); // redirect stdout to write end of pipe
                dup2(out[1], STDERR_FILENO); // redirect stderr to write end of pipe
            }
            else if (redirHandle(flag[0], file[0]) == -1) { // handle redirection, if error, exit with failure
                exit(EXIT_FAILURE);
            }
            close(out[1]); // close write end of pipe

            execvp(args[0], new_args); // execute command with arguments
            perror(name); // if error, print error
            exit(EXIT_FAILURE); // exit with failure if execvp failed
        }  
        
        // else, execute as normal
        else
        {
            if (flag[0] == NULL) { // if no flag specified, then redirect output to pipe to send back to client
                dup2(out[1], STDOUT_FILENO); // redirect stdout to write end of pipe
                dup2(out[1], STDERR_FILENO); // redirect stderr to write end of pipe
            }
            else if (redirHandle(flag[0], file[0]) == -1) { // handle redirection, if error, exit with failure
                exit(EXIT_FAILURE);
            }
            close(out[1]); // close write end of pipe
            execvp(args[0], args); // execute command with arguments
            perror(args[0]); // if error, print error
            exit(EXIT_FAILURE); // exit with failure if execvp failed
        }
    }
    close(out[1]); // close write end of pipe
    wait(NULL); // wait for child process to finish

    if (flag[0] == NULL) { // if no flag specified, then redirect output to pipe to send back to client
        // Read the output from the pipe
        char buffer[4096]; // Adjust the buffer size as needed
        ssize_t bytesRead = read(out[0], buffer, sizeof(buffer));
        close(out[0]); // Close the read end of the pipe

        if (bytesRead == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }

        // Allocate memory for the result and copy the output
        char *output = malloc(bytesRead + 1);
        if (output == NULL) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }

        strncpy(output, buffer, bytesRead);
        output[bytesRead] = '\0'; // Null-terminate the string

        return output;
    }

    close(out[0]); // close read end of pipe
    return NULL;
}

char* onePipe(char*** args, char** flag, char** file) { // execute two commands with one pipe

    int out[2]; // pipe to store final output to stdout if no output redirection

    if (pipe(out) == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid;
    pid = fork(); // fork process

    if (pid < 0) { // if fork failed, print error and exit with failure
        printf("Fork failed\n");
        exit(1);
    }
    if (pid == 0) { // if child process

        int fd1[2]; // file descriptors for pipe

        if(pipe(fd1) == -1){ // create pipe, if error, print error and exit with failure
            perror("pipe");
            exit(EXIT_FAILURE);
        }

        pid_t cmd1; 
        cmd1 = fork(); // fork process
        if (cmd1 < 0) { // if fork failed, print error and exit with failure
            printf("Fork failed\n");
            exit(1);
        }

        if (cmd1 == 0) { // if child process
            
            close(out[0]); // close read end of pipe
            // close(out[1]); // close write end of pipe

            if (flag[0] != NULL && strcmp(flag[0], "<") != 0) { // if flag is not NULL and flag is not input redirection, print error message and exit with failure
                dup2(out[1], STDOUT_FILENO); // redirect stdout to write end of pipe
                dup2(out[1], STDERR_FILENO); // redirect stderr to write end of pipe
                close(out[1]); // close write end of pipe
                printf("Error: Invalid redirection\n");
                exit(EXIT_FAILURE);
            }

            close(out[1]); // close write end of pipe

            if (redirHandle(flag[0], file[0]) == -1) { // handle redirection, if error, exit with failure
                exit(EXIT_FAILURE);
            }
            close(fd1[0]); // close read end of pipe
            dup2(fd1[1], STDOUT_FILENO); // redirect stdout to write end of pipe
            close(fd1[1]); // close write end of pipe as stdout is now redirected to it
            execvp(args[0][0], args[0]); // execute command with arguments
            perror(args[0][0]); // if error, print error
            exit(EXIT_FAILURE); // exit with failure if execvp failed
        }
        else {
            close(out[0]); // close read end of final output pipe
            int status;
            wait(&status); // wait for child process to finish
            if (WEXITSTATUS(status) == 1)
            { // if child process exited with failure, exit with failure
                printf("Error: Invalid combination\n");
                exit(EXIT_FAILURE);
            }

            if (flag[1] != NULL && strcmp(flag[1], ">") != 0 && strcmp(flag[1], "2>") != 0) { // if flag is not NULL and flag is not output redirection, print error message and exit with failure
                dup2(out[1], STDOUT_FILENO); // redirect stdout to write end of pipe
                dup2(out[1], STDERR_FILENO); // redirect stderr to write end of pipe
                close(out[1]); // close write end of pipe
                printf("Error: Invalid redirection\n");
                exit(EXIT_FAILURE);
            }

            if (flag[1] == NULL) { // if no flag specified, then redirect output to pipe to send back to client
                dup2(out[1], STDOUT_FILENO); // redirect stdout to write end of pipe
                dup2(out[1], STDERR_FILENO); // redirect stderr to write end of pipe
            }
            else if (redirHandle(flag[1], file[1]) == -1) { // handle redirection, if error, exit with failure
                exit(EXIT_FAILURE);
            }
            close(out[1]); // close write end of final output pipe
            close(fd1[1]); // close write end of pipe
            
            dup2(fd1[0], STDIN_FILENO); // redirect stdin to read end of pipe
            close(fd1[0]); // close read end of pipe as stdin is now redirected to it
            execvp(args[1][0], args[1]); // execute command with arguments
            perror(args[1][0]); // if error, print error
            exit(EXIT_FAILURE); // exit with failure if execvp failed
        }
    }
    else {
        close(out[1]); // close write end of pipe
        wait(NULL); // wait for child process to finish

        if (flag[1] == NULL || strcmp(flag[1], "<") == 0) { // if no flag specified or flag is incorrect, then redirect output to pipe to send back to client
            // Read the output from the pipe
            char buffer[4096]; // Adjust the buffer size as needed
            ssize_t bytesRead = read(out[0], buffer, sizeof(buffer));
            close(out[0]); // Close the read end of the pipe

            if (bytesRead == -1) {
                perror("read");
                exit(EXIT_FAILURE);
            }

            // Allocate memory for the result and copy the output
            char *output = malloc(bytesRead + 1);
            if (output == NULL) {
                perror("malloc");
                exit(EXIT_FAILURE);
            }

            strncpy(output, buffer, bytesRead);
            output[bytesRead] = '\0'; // Null-terminate the string

            return output;
        }

        close(out[0]); // close read end of pipe
    }
    return NULL;
}

char* twoPipe(char*** args, char** flag, char** file) { // execute three commands with two pipes

    int out[2]; // pipe to store final output to stdout if no output redirection

    if (pipe(out) == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid;
    pid = fork(); // fork process

    if (pid < 0) { // if fork failed, print error and exit with failure
        printf("Fork failed\n");
        exit(1);
    }
    if (pid == 0) { // if child process

        int fd1[2]; // file descriptors for pipe that connects command 2 and 3

        if(pipe(fd1) == -1){ // create pipe, if error, print error and exit with failure
            perror("pipe");
            exit(EXIT_FAILURE);
        }

        pid_t cmd1;
        cmd1 = fork(); // fork process
        if (cmd1 < 0) { // if fork failed, print error and exit with failure
            printf("Fork failed\n");
            exit(1);
        }

        if (cmd1 == 0) { // if child process

            int fd2[2]; // file descriptors for pipe that connects command 1 and 2

            if(pipe(fd2) == -1){ // create pipe, if error, print error and exit with failure
                perror("pipe");
                exit(EXIT_FAILURE);
            }

            pid_t cmd2; 
            cmd2 = fork(); // fork process
            if (cmd2 < 0) { // if fork failed, print error and exit with failure
                printf("Fork failed\n");
                exit(1);
            }

            if (cmd2 == 0) { // if child process

                close(out[0]); // close read end of pipe for final output
                // close(out[1]); // close write end of pipe for final output

                if (flag[0] != NULL && strcmp(flag[0], "<") != 0) { // if flag is not NULL and flag is not input redirection, print error message and exit with failure
                    dup2(out[1], STDOUT_FILENO); // redirect stdout to write end of pipe
                    dup2(out[1], STDERR_FILENO); // redirect stderr to write end of pipe
                    close(out[1]); // close write end of pipe
                    printf("Error: Invalid redirection\n");
                    exit(EXIT_FAILURE);
                }

                close(out[1]); // close write end of pipe for final output

                if (redirHandle(flag[0], file[0]) == -1) { // handle redirection, if error, exit with failure
                    exit(EXIT_FAILURE); 
                }
                close(fd2[0]); // close read end of pipe that connects command 1 and 2
                close(fd1[1]); // close write end of pipe that connects command 2 and 3
                close(fd1[0]); // close read end of pipe that connects command 2 and 3
                dup2(fd2[1], STDOUT_FILENO); // redirect stdout to write end of pipe that connects command 1 and 2
                close(fd2[1]); // close write end of pipe that connects command 1 and 2 as stdout is now redirected to it
                execvp(args[0][0], args[0]); // execute command with arguments
                perror(args[0][0]); // if error, print error
                exit(EXIT_FAILURE); // exit with failure if execvp failed
            }

            else {

                close(out[0]); // close read end of pipe for final output
                // close(out[1]); // close write end of pipe for final output

                int status;
                wait(&status); // wait for child process to finish
                if (WEXITSTATUS(status) == 1)
                { // if child process exited with failure, exit with failure
                    printf("Error: Invalid combination\n");
                    exit(EXIT_FAILURE);
                }

                if (flag[1] != NULL) { // if flag is not NULL, print error message and exit with failure
                    dup2(out[1], STDOUT_FILENO); // redirect stdout to write end of pipe
                    dup2(out[1], STDERR_FILENO); // redirect stderr to write end of pipe
                    close(out[1]); // close write end of pipe
                    printf("Error: Invalid redirection\n");
                    exit(EXIT_FAILURE);
                }

                close(out[1]); // close write end of pipe for final output

                if (redirHandle(flag[1], file[1]) == -1) { // handle redirection, if error, exit with failure
                    exit(EXIT_FAILURE);
                }
                close(fd2[1]); // close write end of pipe that connects command 1 and 2
                close(fd1[0]); // close read end of pipe that connects command 2 and 3
                
                dup2(fd2[0], STDIN_FILENO); // redirect stdin to read end of pipe that connects command 1 and 2
                dup2(fd1[1], STDOUT_FILENO); // redirect stdout to write end of pipe that connects command 2 and 3
                close(fd2[0]); // close read end of pipe that connects command 1 and 2 as stdin is now redirected to it
                close(fd1[1]); // close write end of pipe that connects command 2 and 3 as stdout is now redirected to it
                execvp(args[1][0], args[1]); // execute command with arguments
                perror(args[1][0]); // if error, print error
                exit(EXIT_FAILURE); // exit with failure if execvp failed
            }
            
        }
        else {
            close(out[0]); // close read end of final output pipe
            int status;
            wait(&status); // wait for child process to finish
            if (WEXITSTATUS(status) == 1)
            { // if child process exited with failure, exit with failure
                printf("Error: Invalid combination\n");
                exit(EXIT_FAILURE);
            }

            if (flag[2] != NULL && strcmp(flag[2], ">") != 0 && strcmp(flag[2], "2>") != 0) { // if flag is not NULL and flag is not output redirection, print error message and exit with failure
                dup2(out[1], STDOUT_FILENO); // redirect stdout to write end of pipe
                dup2(out[1], STDERR_FILENO); // redirect stderr to write end of pipe
                close(out[1]); // close write end of pipe
                printf("Error: Invalid redirection\n");
                exit(EXIT_FAILURE);
            }

            if (flag[2] == NULL) { // if no flag specified, then redirect output to pipe to send back to client
                dup2(out[1], STDOUT_FILENO); // redirect stdout to write end of pipe
                dup2(out[1], STDERR_FILENO); // redirect stderr to write end of pipe
            }
            else if (redirHandle(flag[2], file[2]) == -1) { // handle redirection, if error, exit with failure
                exit(EXIT_FAILURE);
            }
            close(out[1]); // close write end of final output pipe
            close(fd1[1]); // close write end of pipe that connects command 2 and 3
            
            dup2(fd1[0], STDIN_FILENO); // redirect stdin to read end of pipe that connects command 2 and 3
            close(fd1[0]); // close read end of pipe that connects command 2 and 3 as stdin is now redirected to it
            execvp(args[2][0], args[2]); // execute command with arguments
            perror(args[2][0]); // if error, print error
            exit(EXIT_FAILURE); // exit with failure if execvp failed
        }
    }
    else {
        close(out[1]); // close write end of pipe
        wait(NULL); // wait for child process to finish

        if (flag[2] == NULL || strcmp(flag[2], "<") == 0) { // if no flag specified or flag is incorrect, then redirect output to pipe to send back to client
            // Read the output from the pipe
            char buffer[4096]; // Adjust the buffer size as needed
            ssize_t bytesRead = read(out[0], buffer, sizeof(buffer));
            close(out[0]); // Close the read end of the pipe

            if (bytesRead == -1) {
                perror("read");
                exit(EXIT_FAILURE);
            }

            // Allocate memory for the result and copy the output
            char *output = malloc(bytesRead + 1);
            if (output == NULL) {
                perror("malloc");
                exit(EXIT_FAILURE);
            }

            strncpy(output, buffer, bytesRead);
            output[bytesRead] = '\0'; // Null-terminate the string

            return output;
        }

        close(out[0]); // close read end of pipe
    }
    return NULL;
}

char* threePipe(char*** args, char** flag, char** file) { // execute four commands with three pipes

    int out[2]; // pipe to store final output to stdout if no output redirection

    if (pipe(out) == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid;
    pid = fork(); // fork process

    if (pid < 0) { // if fork failed, print error and exit with failure
        printf("Fork failed\n");
        exit(1);
    }
    if (pid == 0) { // if child process

        int fd1[2]; // file descriptors for pipe that connects command 3 and 4

        if(pipe(fd1) == -1){ // create pipe, if error, print error and exit with failure
            perror("pipe"); 
            exit(EXIT_FAILURE);
        }

        pid_t cmd1;
        cmd1 = fork(); // fork process
        if (cmd1 < 0) { // if fork failed, print error and exit with failure
            printf("Fork failed\n");
            exit(1);
        }

        if (cmd1 == 0) { // if child process

            int fd2[2]; // file descriptors for pipe that connects command 2 and 3

            if(pipe(fd2) == -1){ // create pipe, if error, print error and exit with failure
                perror("pipe");
                exit(EXIT_FAILURE);
            }

            pid_t cmd2; 
            cmd2 = fork(); // fork process
            if (cmd2 < 0) { // if fork failed, print error and exit with failure
                printf("Fork failed\n");
                exit(1);
            }

            if (cmd2 == 0) { // if child process

                int fd3[2]; // file descriptors for pipe that connects command 1 and 2

                if(pipe(fd3) == -1){ // create pipe, if error, print error and exit with failure
                    perror("pipe");
                    exit(EXIT_FAILURE);
                }

                pid_t cmd3;
                cmd3 = fork(); // fork process
                if (cmd3 < 0) { // if fork failed, print error and exit with failure
                    printf("Fork failed\n");
                    exit(1);
                }

                if (cmd3 == 0) { // if child process

                    close(out[0]); // close read end of pipe for final output
                    // close(out[1]); // close write end of pipe for final output

                    if (flag[0] != NULL && strcmp(flag[0], "<") != 0) { // if flag is not NULL and flag is not input redirection, print error message and exit with failure
                        dup2(out[1], STDOUT_FILENO); // redirect stdout to write end of pipe
                        dup2(out[1], STDERR_FILENO); // redirect stderr to write end of pipe
                        close(out[1]); // close write end of pipe
                        printf("Error: Invalid redirection\n");
                        exit(EXIT_FAILURE);
                    }
                    close(out[1]); // close write end of pipe for final output

                    if (redirHandle(flag[0], file[0]) == -1) { // handle redirection, if error, exit with failure
                        exit(EXIT_FAILURE);
                    }
                    close(fd3[0]); // close read end of pipe that connects command 1 and 2
                    close(fd2[1]); // close write end of pipe that connects command 2 and 3
                    close(fd2[0]); // close read end of pipe that connects command 2 and 3
                    close(fd1[1]); // close write end of pipe that connects command 3 and 4
                    close(fd1[0]); // close read end of pipe that connects command 3 and 4
                    dup2(fd3[1], STDOUT_FILENO); // redirect stdout to write end of pipe that connects command 1 and 2
                    close(fd3[1]); // close write end of pipe that connects command 1 and 2 as stdout is now redirected to it
                    execvp(args[0][0], args[0]); // execute command with arguments
                    perror(args[0][0]); // if error, print error
                    exit(EXIT_FAILURE); // exit with failure if execvp failed
                }

                else {
                    
                    close(out[0]); // close read end of pipe for final output
                    // close(out[1]); // close write end of pipe for final output

                    int status;
                    wait(&status); // wait for child process to finish
                    if (WEXITSTATUS(status) == 1)
                    { // if child process exited with failure, exit with failure
                        printf("Error: Invalid combination\n");
                        exit(EXIT_FAILURE);
                    }

                    if (flag[1] != NULL) { // if flag is not NULL, print error message and exit with failure
                        dup2(out[1], STDOUT_FILENO); // redirect stdout to write end of pipe
                        dup2(out[1], STDERR_FILENO); // redirect stderr to write end of pipe
                        close(out[1]); // close write end of pipe
                        printf("Error: Invalid redirection\n");
                        exit(EXIT_FAILURE);
                    }
                    close(out[1]); // close write end of pipe for final output

                    if (redirHandle(flag[1], file[1]) == -1) { // handle redirection, if error, exit with failure
                        exit(EXIT_FAILURE);
                    }
                    close(fd3[1]); // close write end of pipe that connects command 1 and 2
                    close(fd2[0]); // close read end of pipe that connects command 2 and 3
                    close(fd1[1]); // close write end of pipe that connects command 3 and 4
                    close(fd1[0]); // close read end of pipe that connects command 3 and 4
                    
                    dup2(fd3[0], STDIN_FILENO); // redirect stdin to read end of pipe that connects command 1 and 2
                    dup2(fd2[1], STDOUT_FILENO); // redirect stdout to write end of pipe that connects command 2 and 3
                    close(fd3[0]); // close read end of pipe that connects command 1 and 2 as stdin is now redirected to it
                    close(fd2[1]); // close write end of pipe that connects command 2 and 3 as stdout is now redirected to it
                    execvp(args[1][0], args[1]); // execute command with arguments
                    perror(args[1][0]); // if error, print error
                    exit(EXIT_FAILURE); // exit with failure if execvp failed
                }
            }

            else {
                
                close(out[0]); // close read end of pipe for final output
                // close(out[1]); // close write end of pipe for final output

                int status;
                wait(&status); // wait for child process to finish
                if (WEXITSTATUS(status) == 1)
                { // if child process exited with failure, exit with failure
                    printf("Error: Invalid combination\n");
                    exit(EXIT_FAILURE);
                }

                if (flag[2] != NULL) { // if flag is not NULL, print error message and exit with failure
                    dup2(out[1], STDOUT_FILENO); // redirect stdout to write end of pipe
                    dup2(out[1], STDERR_FILENO); // redirect stderr to write end of pipe
                    close(out[1]); // close write end of pipe
                    printf("Error: Invalid redirection\n");
                    exit(EXIT_FAILURE);
                }
                close(out[1]); // close write end of pipe for final output

                if (redirHandle(flag[2], file[2]) == -1) { // handle redirection, if error, exit with failure
                    exit(EXIT_FAILURE);
                }
                close(fd2[1]); // close write end of pipe that connects command 2 and 3
                close(fd1[0]); // close read end of pipe that connects command 3 and 4

                dup2(fd2[0], STDIN_FILENO); // redirect stdin to read end of pipe that connects command 2 and 3
                dup2(fd1[1], STDOUT_FILENO); // redirect stdout to write end of pipe that connects command 3 and 4
                close(fd2[0]); // close read end of pipe that connects command 2 and 3 as stdin is now redirected to it
                close(fd1[1]); // close write end of pipe that connects command 3 and 4 as stdout is now redirected to it
                execvp(args[2][0], args[2]); // execute command with arguments
                perror(args[2][0]); // if error, print error
                exit(EXIT_FAILURE); // exit with failure if execvp failed
            }
            
        }
        else {
            close(out[0]); // close read end of final output pipe

            int status;
            wait(&status); // wait for child process to finish
            if (WEXITSTATUS(status) == 1)
            { // if child process exited with failure, exit with failure
                printf("Error: Invalid combination\n");
                exit(EXIT_FAILURE);
            }

            if (flag[3] != NULL && strcmp(flag[3], ">") != 0 && strcmp(flag[3], "2>") != 0) { // if flag is not NULL and flag is not output redirection, print error message and exit with failure
                dup2(out[1], STDOUT_FILENO); // redirect stdout to write end of pipe
                dup2(out[1], STDERR_FILENO); // redirect stderr to write end of pipe
                close(out[1]); // close write end of pipe
                printf("Error: Invalid redirection\n");
                exit(EXIT_FAILURE);
            }

            if (flag[3] == NULL) { // if no flag specified, then redirect output to pipe to send back to client
                dup2(out[1], STDOUT_FILENO); // redirect stdout to write end of pipe
                dup2(out[1], STDERR_FILENO); // redirect stderr to write end of pipe
            }
            else if (redirHandle(flag[3], file[3]) == -1) { // handle redirection, if error, exit with failure
                exit(EXIT_FAILURE);
            }
            close(out[1]); // close write end of final output pipe
            close(fd1[1]); // close write end of pipe that connects command 3 and 4
            dup2(fd1[0], STDIN_FILENO); // redirect stdin to read end of pipe that connects command 3 and 4
            close(fd1[0]); // close read end of pipe that connects command 3 and 4 as stdin is now redirected to it
            execvp(args[3][0], args[3]); // execute command with arguments
            perror(args[3][0]); // if error, print error
            exit(EXIT_FAILURE); // exit with failure if execvp failed
        }
    }
    else {
        close(out[1]); // close write end of pipe
        wait(NULL); // wait for child process to finish

        if (flag[3] == NULL || strcmp(flag[3], "<") == 0) { // if no flag specified or flag is incorrect, then redirect output to pipe to send back to client
            // Read the output from the pipe
            char buffer[4096]; // Adjust the buffer size as needed
            ssize_t bytesRead = read(out[0], buffer, sizeof(buffer));
            close(out[0]); // Close the read end of the pipe

            if (bytesRead == -1) {
                perror("read");
                exit(EXIT_FAILURE);
            }

            // Allocate memory for the result and copy the output
            char *output = malloc(bytesRead + 1);
            if (output == NULL) {
                perror("malloc");
                exit(EXIT_FAILURE);
            }

            strncpy(output, buffer, bytesRead);
            output[bytesRead] = '\0'; // Null-terminate the string

            return output;
        }

        close(out[0]); // close read end of pipe
    }
    return NULL;
}

