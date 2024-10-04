#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

void ClientProcess(int []);

int main(int argc, char *argv[])
{
    int ShmID;
    int *ShmPTR;
    pid_t pid;
    int status;

    // Create shared memory for two integers: BankAccount and Turn
    ShmID = shmget(IPC_PRIVATE, 2 * sizeof(int), IPC_CREAT | 0666);
    if (ShmID < 0) {
        printf("*** shmget error (server) ***\n");
        exit(1);
    }
    printf("Server has received a shared memory for BankAccount and Turn...\n");

    // Attach shared memory
    ShmPTR = (int *) shmat(ShmID, NULL, 0);
    if (ShmPTR == (int *) -1) {
        printf("*** shmat error (server) ***\n");
        exit(1);
    }
    printf("Server has attached the shared memory...\n");

    // Initialize shared variables
    ShmPTR[0] = 0; // BankAccount
    ShmPTR[1] = 0; // Turn
    printf("Server has initialized BankAccount = %d, Turn = %d\n", ShmPTR[0], ShmPTR[1]);

    // Fork a child process
    printf("Server is about to fork a child process...\n");
    pid = fork();
    if (pid < 0) {
        printf("*** fork error (server) ***\n");
        exit(1);
    } else if (pid == 0) {
        ClientProcess(ShmPTR);
        exit(0);
    }

    // Parent process
    srand(time(NULL)); // Seed random number generator
    int i;
    for (i = 0; i < 25; i++) {
        sleep(rand() % 6); // Sleep for 0 to 5 seconds
        int account = ShmPTR[0]; // Copy current BankAccount value
        
        while (ShmPTR[1] != 0) // Busy wait until it's the parent's turn
            ;

        if (account <= 100) {
            int balance = rand() % 101; // Random amount to deposit between 0 and 100
            if (balance % 2 == 0) { // Even number: deposit
                ShmPTR[0] += balance; // Deposit into BankAccount
                printf("Dear old Dad: Deposits $%d / Balance = $%d\n", balance, ShmPTR[0]);
            } else { // Odd number: no money to give
                printf("Dear old Dad: Doesn't have any money to give\n");
            }
        } else {
            printf("Dear old Dad: Thinks Student has enough Cash ($%d)\n", account);
        }
        
        ShmPTR[0] = account; // Update the shared BankAccount value
        ShmPTR[1] = 1; // Set turn to child
    }

    wait(&status); // Wait for the child to finish
    printf("Server has detected the completion of its child...\n");
    shmdt((void *) ShmPTR); // Detach shared memory
    printf("Server has detached its shared memory...\n");
    shmctl(ShmID, IPC_RMID, NULL); // Remove shared memory
    printf("Server has removed its shared memory...\n");
    printf("Server exits...\n");
    exit(0);
}

void ClientProcess(int SharedMem[])
{
    printf("   Client process started\n");
    srand(time(NULL) + 1); // Seed random number generator
    int i;
    for (i = 0; i < 25; i++) {
        sleep(rand() % 6); // Sleep for 0 to 5 seconds
        int account = SharedMem[0]; // Copy current BankAccount value
        
        while (SharedMem[1] != 1) // Busy wait until it's the child's turn
            ;

        int balance = rand() % 51; // Random amount needed between 0 and 50
        printf("Poor Student needs $%d\n", balance);
        
        if (balance <= account) {
            SharedMem[0] -= balance; // Withdraw from BankAccount
            printf("Poor Student: Withdraws $%d / Balance = $%d\n", balance, SharedMem[0]);
        } else {
            printf("Poor Student: Not Enough Cash ($%d)\n", account);
        }

        SharedMem[0] = account; // Update the shared BankAccount value
        SharedMem[1] = 0; // Set turn to parent
    }

    printf("   Client process exiting...\n");
}
