#ifndef LIST_H
#define LIST_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>

// structure of a node in the waiting queue
typedef struct Node {
    int client_id; // unique client id
    int arrival_time; // time at which the client arrives
    int burst_time; // remaining time till completion
    int remaining_time; // time allowed in the current quantum
    struct Node* next; // pointer to the next node
    sem_t *sem_running; // semaphore for the client
    sem_t *completed; // semaphore for the client
    int startFlag; // flag to check if the client has started
    int type; // type of client job
} Node;

Node* createNode(int client_id, int arrival_time, int burst_time, int remaining_time, int type); // to create a new node
// Node* insertNode(Node* head, int client_id, int arrival_time, int burst_time, int remaining_time); // to insert a node at the end of the list
Node* insertNode(Node* head, Node* newNode); // to insert a node at the end of the list
void printNode(Node* head); // to print the list
Node* searchNode(Node* head, int client_id); // to search for a node in the list
Node* searchLeast(Node* head); // to specifically search for the node with burst time equal to -1, else return the first node
Node* searchNextLeast(Node* head, Node* least); // to search for the node with the next least burst time in the list
Node* deleteNode(Node* head, int client_id); // to delete a node from the list
int numNodes(Node* head); // to count the number of nodes in the list
void freeQ(Node* head); // to free the memory allocated for the list

#endif
