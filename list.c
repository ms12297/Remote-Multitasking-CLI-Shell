#include "list.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>

// // structure of a node in the waiting queue
// typedef struct Node {
//     int client_id;
//     int arrival_time;
//     int burst_time;
//     int remaining_time; // time allowed in the current quantum
//     struct Node* next;
// } Node;


// to create a new node
Node* createNode(int client_id, int arrival_time, int burst_time, int remaining_time, int type) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    if (newNode == NULL) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }
    newNode->client_id = client_id;
    newNode->arrival_time = arrival_time;
    newNode->burst_time = burst_time;
    newNode->remaining_time = remaining_time; // remaining time is initially equal to the current quantum
    newNode->next = NULL;
    // assigning the semaphore with name as client_id
    char sem_name[10];
    sprintf(sem_name, "%d", client_id);
    newNode->sem_running = sem_open(sem_name, O_CREAT, 0644, 0);

    // assigning the semaphore with name as client_id+"c"
    char completed_name[10];
    sprintf(completed_name, "%dc", client_id);
    newNode->completed = sem_open(completed_name, O_CREAT, 0644, 0);

    // initializing the flag to check if the client has started
    newNode->startFlag = 0;

    // assigning the type of client job
    newNode->type = type;

    return newNode;
}

// to insert a node at the end of the list
Node* insertNode(Node* head, Node* newNode) {
    if (head == NULL) {
        return newNode;
    }
    Node* current = head;
    while (current->next != NULL) {
        current = current->next;
    }
    current->next = newNode;
    return head;
}

// to print the list
void printNode(Node* head) {
    while (head != NULL) {
        printf("Client ID: %d, Arrival Time: %d, Burst Time: %d\n",
               head->client_id, head->arrival_time, head->burst_time);
        head = head->next;
    }
}

// to search for a node in the list
Node* searchNode(Node* head, int client_id) {
    while (head != NULL) {
        if (head->client_id == client_id) {
            return head; // found
        }
        head = head->next;
    }
    return NULL; // not found
}

// to search for the node with the least burst time in the list
Node* searchLeast(Node* head) {
    Node* leastBurstTimeNode = head;
    while (head != NULL) {
        if (head->burst_time < leastBurstTimeNode->burst_time) {
            leastBurstTimeNode = head;
        }
        head = head->next;
    }
    return leastBurstTimeNode;
}

Node* searchNextLeast(Node* head, Node* least) {
    // if there is only one element or all elements have the same burst time, return the head
    if (head->next == NULL || (least->next == NULL && least->burst_time == head->burst_time)) {
        return head;
    }
    // now we know that there is one other element with a different burst time
    Node* nextLeastNode = NULL;
    Node* current = head;
    while (current != NULL) {
        if (current != least) {
            if (nextLeastNode == NULL || current->burst_time < nextLeastNode->burst_time) {
                nextLeastNode = current;
            }
        }
        current = current->next;
    }
    return nextLeastNode;
}

// to delete a node from the list
Node* deleteNode(Node* head, int client_id) {
    Node *current = head;
    Node *prev = NULL;

    while (current != NULL && current->client_id != client_id) {
        prev = current;
        current = current->next;
    }

    if (current == NULL) {
        printf("Client %d not found in the list.\n", client_id);
        return head;
    }

    if (prev == NULL) {
        head = current->next; // client to be deleted is the first node
    } else {
        prev->next = current->next; // client to be deleted is not the first node
        // // close and unlink the semaphore
        // sem_close(current->sem_running);
    }

    free(current); // freeing the memory allocated for the node
    return head;
}


int numNodes(Node* head) { // to count the number of nodes in the list
    int count = 0;
    while (head != NULL) {
        count++;
        head = head->next;
    }
    return count;
}

// to free the memory allocated for the list
void freeQ(Node* head) {
    Node* current = head;
    Node* next;
    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }
}