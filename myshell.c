#include <stdio.h> // for printf
#include <unistd.h> // for fork
#include <stdlib.h> // for exit
#include <sys/wait.h> // for wait
#include <sys/types.h> // for pid_t
#include <fcntl.h> // for flags
#include <sys/stat.h> // for permission flags
#include <string.h> // for strtok
#include "execute.h"
#include "parse.h"
#include <netdb.h> 
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>
#include <semaphore.h>
#include "list.h" // contains the waiting queue

#define MAX 1024
#define PORT 5454

void* client_thread(void * socket);
void* scheduler_thread();

Node* waitingQueue = NULL;
int quantum = 3; // quantum for round robin scheduling
Node* running = NULL; // currently running process
int curr_time = 0; // current time
int** gantt = NULL; // gantt chart array, has format [[curr_time, client_id], ...]
int ganttIdx = 0; // index for gantt chart
int printGantt = 0; // flag to print gantt chart

sem_t sem_waitingQueue; // semaphore for waitingQueue
sem_t *mutexRunning; // semaphore for running

int preempted = 0; // flag to check if running was preempted
int hasPreempted = 0; // flag to check if running preempted ever in the current round

int main() {

    sem_init(&sem_waitingQueue, 1, 1); // initialize semaphore for waitingQueue
    mutexRunning = sem_open("mutexRunning", O_CREAT, 0644, 0); // named semaphore for controlling scheduling based on running threads

    // intializing gantt chart
    gantt = (int **)malloc(20 * sizeof(int *));

    // create scheduler thread
    pthread_t scheduler;
    pthread_create(&scheduler, NULL, scheduler_thread, NULL);

    int server_socket, client_socket;
    socklen_t len;
    struct sockaddr_in servaddr, cli;
    int addrlen = sizeof(servaddr);

    // socket create and verification
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1)
    {
        printf("socket creation failed...\n");
        exit(0);
    }

    // socket reuse
    int value  = 1;
    setsockopt(server_socket,SOL_SOCKET,SO_REUSEADDR,&value,sizeof(value)); //&(int){1},sizeof(int)

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);

    // binding newly created socket to given IP and verification
    if ((bind(server_socket, (struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
    {
        printf("socket bind failed...\n");
        exit(0);
    }

    printAll(); // print welcome message and list of all commands

    while(1){  // server will keep listening for new clients
        // listen for clients
        if (listen(server_socket, 3) < 0) 
        { 
                perror("listen"); 
                exit(EXIT_FAILURE); 
        }
        // accept client
        if ((client_socket = accept(server_socket, (struct sockaddr *)&servaddr, 
                                        (socklen_t*)&addrlen))<0) 
        { 
                perror("accept");
                exit(EXIT_FAILURE); 
        } 
        pthread_t th;
        printf("[%d]<<< connected\n", client_socket); // print client connected
        pthread_create(&th,NULL,client_thread,&client_socket); // create thread for client
	}
    close(server_socket); // close the socket
    return 0; 
}


void printGanttChart(int curr_time) {
    printf("\nGantt Chart\n");
    printf("CID\tTime\n");
    
    // starting time of 0
    printf("-\t0\n");

    // printing gantt chart
    for (int i = 0; i < ganttIdx; i++) {
        printf("P%d\t%d\n", gantt[i][0], gantt[i][1]); // format is [client_id, finish_time]
    }
    
    // finishing time
    printf("0\t%d\n", curr_time);
    printf("\n");

    // free gantt chart
    for (int i = 0; i < ganttIdx; i++) {
        free(gantt[i]);
    }
    free(gantt);

    ganttIdx = 0;

    // reassign gantt
    gantt = (int **)malloc(20 * sizeof(int *));
}


void* scheduler_thread() {
    while(1){

        // if waitingQueue is empty, continue
        if (waitingQueue == NULL || running == NULL) {
            continue;
        }

        if (preempted == 1) {
            preempted = 0;
            hasPreempted = 1;
   
            quantum = 7;
            // set all remaining times in waitingQueue to quantum
            Node* current = waitingQueue;
            while (current != NULL) {
                current->remaining_time = quantum;
                current = current->next;
            }
            sem_wait(&sem_waitingQueue);
            printf("[%d]--- waiting (%d)\n", running->client_id, running->burst_time);

            // add entry to gantt chart
            gantt[ganttIdx] = (int *)malloc(2 * sizeof(int));
            gantt[ganttIdx][0] = running->client_id;
            gantt[ganttIdx][1] = curr_time;
            ganttIdx++;

            running = searchLeast(waitingQueue);
            if (running->startFlag == 0) {
                running->startFlag = 1;
                printf("[%d]--- started (%d)\n", running->client_id, running->burst_time);
            }
            else {
                printf("[%d]--- running (%d)\n", running->client_id, running->burst_time);
            }
            sem_post(&sem_waitingQueue);
        }

        if (running->startFlag == 0) {
            running->startFlag = 1;
            printf("[%d]--- started (%d)\n", running->client_id, running->burst_time);
        }

        if (running->type == 0) { // if demo command
            sem_post(running->sem_running);
            sem_wait(mutexRunning);
        }
        // sleep(1); // sleep for 1 second to simulate 1 second of execution
        

        // printf("CID %d, Arrival: %d, Burst: %d, Q: %d, Curr: %d, Rem: %d\n",
        //        running->client_id, running->arrival_time, running->burst_time, quantum, curr_time, running->remaining_time);


        running->burst_time--; // decrement burst time
        running->remaining_time--; // decrement remaining time
        curr_time++; // increment time elapsed

        // if running has finished executing, set it to the next least burst time in waitingQueue
        if (running->burst_time <= 0) {
            sem_wait(&sem_waitingQueue);
            sem_post(running->completed);
            printf("[%d]--- ended (%d)\n", running->client_id, running->burst_time);

            // add entry to gantt chart
            gantt[ganttIdx] = (int *)malloc(2 * sizeof(int));
            gantt[ganttIdx][0] = running->client_id;
            gantt[ganttIdx][1] = curr_time;
            ganttIdx++;

            waitingQueue = deleteNode(waitingQueue, running->client_id);
            running = searchLeast(waitingQueue);
            if (running != NULL) {
                if (running->startFlag == 0) {
                    running->startFlag = 1;
                    printf("[%d]--- started (%d)\n", running->client_id, running->burst_time);
                }
                else {
                    printf("[%d]--- running (%d)\n", running->client_id, running->burst_time);
                }
            }
            else {
                printGantt = 1;
            }
            sem_post(&sem_waitingQueue);
            continue;
        }
        
        // if quantum has finished, set running to the next least burst time in waitingQueue
        if (running->remaining_time == 0 && running != NULL) {
            sem_wait(&sem_waitingQueue);
            printf("[%d]--- waiting (%d)\n", running->client_id, running->burst_time);

            // add entry to gantt chart
            gantt[ganttIdx] = (int *)malloc(2 * sizeof(int));
            gantt[ganttIdx][0] = running->client_id;
            gantt[ganttIdx][1] = curr_time;
            ganttIdx++;

            if (running->next == NULL && hasPreempted == 0) { // if running is the last node in waitingQueue and no preemption, update quantum
                quantum = 7;
                // set all remaining times in waitingQueue to quantum
                Node* current = waitingQueue;
                while (current != NULL) {
                    current->remaining_time = quantum;
                    current = current->next;
                }
            }

            running->remaining_time = quantum;
            running = searchNextLeast(waitingQueue, running);
            if (running->startFlag != 0) {
                printf("[%d]--- running (%d)\n", running->client_id, running->burst_time);
            }
            // printf("[%d]--- running (%d)\n", running->client_id, running->burst_time);
            sem_post(&sem_waitingQueue);
        }
    }
}


void* client_thread (void * socket) {
    int *sock = (int *)socket;
    int s = *sock;

    char* input = NULL; // user input
    char buff[MAX]; // buffer to recv from client
    char** commands; // commands separated by pipes
    char** flags; // flags for each redirection
    char** files; // files for each redirection
    int numPipes; // number of pipes in input
    int numCommands; // number of commands in input
    char*** arguments; // arguments for each command
    int dealoc_flag = 0; // flag to deallocate memory, 0 = don't deallocate, 1 = deallocate, based on if user exits right away
    int shell_flag = 0; // flag to check if shell command is being executed
    int demo_flag = 0; // flag to check if demo command is being executed
    int burst = 0; // burst time for demo command

    while(1) { // loop until user enters "exit"

        // reinitialize buff before recv-ing
        memset(buff, 0, sizeof(buff));
        
        recv(s, buff, sizeof(buff), 0);

        input = buff; // to maintain consistency across our previous functions

        if (strcmp(input, "exit") == 0) { // exit if user enters "exit"
            break;
        }
        if (strcmp(input, "") == 0) { // if user enters nothing, continue
            continue;
        }
        if (strcmp(input, "clear") == 0) { // if user enters "clear", clear screen
            system("clear");
            continue;
        }

        numPipes = countPipes(input); // count number of pipes in input

        if (numPipes > 3) { // if more than 3 pipes, print error message and continue
            printf("Error: More than 3 pipes\n");
            continue;
        }
        if (numPipes == -1) { // if error counting pipes, print error message and continue
            printf("Error: Invalid redirection\n");
            continue;
        }

        commands = parseCMD(input); // parse input into commands separated by pipes
        arguments = (char ***)malloc((numPipes + 1) * sizeof(char **)); // allocate memory for arguments
        numCommands = numPipes + 1; // number of commands is number of pipes + 1
        for (int i = 0; i < numCommands; i++) { // parse each command into arguments
            arguments[i] = parseARGS(commands[i]); 
        }
        parseRedir(arguments, &numPipes, &flags, &files); // parse redirections into flags and files, if any


        // check if command starts with "./"
        if (strstr(arguments[0][0], "./") != NULL) {
            // check if first argument is "./demo"
            if (strcmp(arguments[0][0], "./demo") == 0) {
                // printf("Demo command detected\n");
                demo_flag = 1; // set demo_flag to 1
                burst = atoi(arguments[0][1]); // set demo_burst to second argument
                // printf("Demo burst time: %d\n", demo_burst);
                // add s to the arguments list at index [0][2]
                arguments[0][2] = (char *)malloc(10);
                if (arguments[0][2] == NULL) {
                    perror("malloc");
                    exit(EXIT_FAILURE);
                }
                sprintf(arguments[0][2], "%d", s);                
            }
            else {
                // printf("Program command detected\n");
                demo_flag = 0; // set demo_flag to 0
                burst = 1; // default burst time chosen to be 1
            }
        }
        else {
            // printf("Shell command detected\n");
            shell_flag = 1; // set shell_flag to 1
            burst = -1; // top priority for shell commands
        }

        printf("[%d]>>> %s\n", s, input); // print user input
        printf("[%d]--- created (%d)\n", s, burst); // print user input
        
        int type = 0; // type of client job, 0 = demo, 1 = program, 2 = shell
        if (demo_flag == 1) {
            type = 0;
        }
        else if (shell_flag == 1) {
            type = 2;
        }
        else {
            type = 1;
        }

        Node* newNode = createNode(s, curr_time, burst, quantum, type); // create a new node

        if (burst != -1) { // if burst is -1, we have a shell command (top priority)
            // allow execution till completion
            sem_wait(&sem_waitingQueue); // wait for waitingQueue to be available

            // insert the new node into the waiting queue
            // waitingQueue = insertNode(waitingQueue, s, curr_time, burst, quantum);
            waitingQueue = insertNode(waitingQueue, newNode);

            sem_post(&sem_waitingQueue); // signal that waitingQueue is available

            if (running == NULL) { // if running is NULL, set it to the least burst time in waitingQueue
                // printf("[%d]--- started (%d)\n", running->client_id, running->burst_time);
                running = searchLeast(waitingQueue);
            }
            
            if (running->burst_time > burst) { // if running has a greater burst time than the new node, preempt
                preempted = 1;
            }
            
        }

        // newNode has a checkStart semaphore, check if it's value is 1 otherwise wait
        if (burst == -1) {
        //     sem_wait(&newNode->checkStart);
        // }
        // else{
            printf("[%d]--- started (%d)\n", s, burst);
        }
        // sem_wait(&newNode->checkStart);

        dealoc_flag = 1; // set dealoc_flag to 1 since user did not exit right away and memory will need to be deallocated

        char *res = NULL; // to hold result of execution

        if (numPipes == 0) { // execute single command if no pipes
            res = singleCMDArgs(arguments[0], flags, files); // testing purposes
        }
        else if (numPipes == 1) { // execute if one pipe
            res = onePipe(arguments, flags, files);
        }
        else if (numPipes == 2) { // execute if two pipes
            res = twoPipe(arguments, flags, files);
        }
        else if (numPipes == 3) { // execute if three pipes
            res = threePipe(arguments, flags, files);
        }        
        
        // printf("sending\n");

        if (res == NULL) { // if res is NULL, set it to "skip"
            res = (char *)malloc(5);
            if (res == NULL) {
                perror("malloc");
                exit(EXIT_FAILURE);
            }
            strcpy(res, "skip"); 
        }
        
        // sleep(1); // ADDRESS -1 CASE

        if (burst != -1) {
            sem_wait(newNode->completed); // signal that node has completed execution
        }
        else {
            printf("[%d]--- ended (%d)\n", s, burst);
        }
        
        send(s, res, strlen(res), 0); // send result to client

        printf("[%d]<<< [%d] bytes sent\n", s, (int)strlen(res)); // print number of bytes sent'
        int terminate = 0;
        if (burst == -1) {
            terminate = -1;
        }
        // printf("[%d]--- ended (%d)\n", s, terminate); // print user input

        if (printGantt == 1) {
            printGantt = 0;
            printGanttChart(curr_time);
        }

    }

    printf("[%d]--- disconnected\n", s); // print client disconnected

    close(s); // close the socket

    // deallocate memory if dealoc_flag is 1
    if (dealoc_flag == 1) {
        // free(input); // free input

        for (int i = 0; i < numCommands; i++) { // free commands and arguments
            char **curr_args = arguments[i]; 
            int j = 0;
            while (curr_args[j] != NULL) { // free each argument in arguments
                free(curr_args[j]); 
                j++;
            }
            free(curr_args); // free arguments
            free(commands[i]); // free command
        }
        free(commands); // free overall commands
        free(arguments); // free overall arguments

        for (int i = 0; i <= numCommands; i++) { // free flags and files if any
            if (flags[i]) free(flags[i]);
            if (files[i]) free(files[i]);
        }

        free(flags); // free overall flags
        free(files); // free overall files
    }

    return NULL;
}
