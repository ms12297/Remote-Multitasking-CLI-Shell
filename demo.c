#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>

int main(int argc, char *argv[]) {
    // verifying the number of arguments
    if (argc != 3) {
        printf("Usage: %s <N>\n", argv[0]);
        return 1;
    }

    // converting the argument to an integer
    int N = atoi(argv[1]);
    int cid = atoi(argv[2]);

    // checking if the argument is valid
    if (N <= 0) {
        printf("Please provide a positive integer for N.\n");
        return 1;
    }

    // making named semaphores with cid as name
    sem_t *sem_running = sem_open(argv[2], 0);
    sem_t *mutexRunning = sem_open("mutexRunning", 0);

    // name is cid+"c"
    char completed_name[10];
    sprintf(completed_name, "%dc", cid);
    sem_t *completed = sem_open(completed_name, 0);
    // sem_t *completed = sem_open(argv[2], 0);
    // sem_t *sem_running = sem_open(argv[2], O_CREAT, 0644, 0);


    // printing progress
    for (int i = 1; i <= N; ++i) {
        sem_wait(sem_running); // wait for running to be available
        printf("%d/%d\n", i, N);
        sleep(1);
        sem_post(mutexRunning); // signal running
        // sem_post(sem_running); // signal running
    }

    // closing the semaphores
    sem_close(sem_running);

    // unlinking the semaphores
    sem_unlink(argv[2]);

    // // completing the process
    // sem_post(completed);
    
    return 0;
}
