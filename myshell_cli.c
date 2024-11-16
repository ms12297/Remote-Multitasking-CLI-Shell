#include <stdio.h>     // for printf
#include <unistd.h>    // for fork
#include <stdlib.h>    // for exit
#include <fcntl.h>     // for flags
#include <sys/stat.h>  // for permission flags
#include <string.h>    // for strtok
#include <netdb.h>     // for socket
#include <sys/socket.h> // for socket
#include "parse.h"
#define PORT 5454


int main()
{
    int sock;
    struct sockaddr_in servaddr, cli;

    // socket create and verification
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        printf("socket creation failed...\n");
        exit(0);
    }

    // socket reuse
    int value  = 1;
    setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&value,sizeof(value)); //&(int){1},sizeof(int)

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);

    // connect the client socket to server socket
    if (connect(sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0)
    {
        printf("connection with the server failed...\n");
        exit(0);
    }

    char *buff = NULL; // to hold user input
    char *newBuff = NULL; // to hold server response
    while (1)
    {
        if (buff != NULL) // if buff is not null, free it
        {
            free(buff);
            buff = NULL;
        }
        buff = getUserInput(); // get user input

        send(sock, buff, strlen(buff), 0); // send user input to server

        if (strcmp(buff, "exit") == 0) { // if user enters exit, break
            break;
        }

        if (newBuff != NULL) // if newBuff is not null, free it
        {
            free(newBuff);
            newBuff = NULL;
        }
        newBuff = (char *)malloc(4096); // allocate memory for newBuff
        if (newBuff == NULL) // if malloc fails, exit
        {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        memset(newBuff, 0, 4096); // initialize newBuff to 0
        ssize_t bytes_received = recv(sock, newBuff, 4096, 0); // receive response from server
        // newBuff[bytes_received] = '\0';
        if (strstr(newBuff, "skip") == NULL) // if server response does not contain "skip", print it
        {
            printf("%s", newBuff);
        }
    }

    close(sock); // close the socket
    return 0;
}