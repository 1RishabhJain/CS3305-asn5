#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<pthread.h>

/*
 * CS3305A - Assignment 5
 * November 29th, 2021
 * Rishabh Jain
 */

void *transaction(void *thread_id);

typedef struct Accounts Account;
typedef struct Clients Client;
pthread_mutex_t lock;

Account *head;
Account *accountPrevious = NULL;

Client *clientHead;
Client *clientPrevious = NULL;

// Using struct to hold account information
struct Accounts {
    char accountName[10];
    int balance;
    Account *next;
};

// Using struct to hold account information
struct Clients {
    char clientName[10];
    char actions[256];
    long int threadID;
    Client *next;
};

// Thread for transaction
void *transaction(void *thread_id) {
    char *saveptr;
    long int thread_name = *((long int *) thread_id);
    Client *clientTemp = clientHead;
    while (clientTemp != NULL) {
        if (clientTemp->threadID == thread_name) {
            break;
        }
        else {
            clientTemp = clientTemp->next;
        }
    }
    int action;
    // First is the client name
    char *ptr = strtok_r(clientTemp->actions, " ", &saveptr);
    // Action (d or w)
    ptr = strtok_r(NULL, " ", &saveptr);
    while (ptr != NULL) {
        ptr[strcspn(ptr, "\r\n")] = 0;
        // If action is deposit (d)
        if (strcmp(ptr, "d") == 0) {
            action = 1;
        }
            // If action is withdrawal (w)
        else if (strcmp(ptr, "w") == 0) {
            action = 0;
        }
        // Account name
        ptr = strtok_r(NULL, " ", &saveptr);
        ptr[strcspn(ptr, "\r\n")] = 0;
        // Finds the balance of the account to be changed
        Account *accountTemp = head;
        while (accountTemp != NULL) {
            if (strcmp(accountTemp->accountName, ptr) == 0) {
                break;
            }
            else {
                accountTemp = accountTemp->next;
            }
        }
        // Transaction amount
        ptr = strtok_r(NULL, " ", &saveptr);
        ptr[strcspn(ptr, "\r\n")] = 0;
        int amount = atoi(ptr);
        // ENTRY
        pthread_mutex_lock(&lock);
        if (action == 1) {
            accountTemp->balance = accountTemp->balance + amount;
        }
        else if (action == 0 && accountTemp->balance >= amount) {
            accountTemp->balance = accountTemp->balance - amount;
        }
        // EXIT
        pthread_mutex_unlock(&lock);
        // Next Action
        ptr = strtok_r(NULL, " ", &saveptr);
    }
}

// Function to print the account balances
void printList() {
    // Array for output using sprintf
    char output[256];
    Account *temp = head;
    while (temp != NULL) {
        sprintf(output,"%s b %d", temp->accountName, temp->balance);
        puts(output);
        temp = temp->next;
    }
}

Account *createAccount(char *line) {
    char accountArray[3][15];
    int count = 0;
    // Splitting line around " " until NULL
    char *ptr = strtok(line, " ");
    while (ptr != NULL) {
        ptr[strcspn(ptr, "\r\n")] = 0;
        strcpy(&accountArray[count][0], ptr);
        ptr = strtok(NULL, " ");
        count++;
    }
    Account *account = malloc(sizeof(Account));
    strcpy(account->accountName, accountArray[0]);
    if (strncmp(accountArray[1], "b", 1) == 0) {
        account->balance = atoi(accountArray[2]);
        // No previous, then it is the first account. Set that to head
        if (!accountPrevious) {
            head = account;
        }
            // There is a previous then it is not the first account
        else if (accountPrevious) {
            accountPrevious->next = account;
        }
        accountPrevious = account;
    }
    return 0;
}

Client *createClient(char *line) {
    Client *client = malloc(sizeof(Client));

    // Copy line into the actions
    strcpy(client->actions, line);
    char *ptr = strtok(line, " ");
    ptr[strcspn(ptr, "\r\n")] = 0;
    strcpy(client->clientName, ptr);
    // No previous, then it is the first account. Set that to head
    if (!clientPrevious) {
        clientHead = client;
    }
    // There is a previous then it is not the first account
    if (clientPrevious) {
        clientPrevious->next = client;
    }
    clientPrevious = client;
    return 0;
}

int main() {
    // array to store original file line and the one which will be modified
    char fileLine[256];
    char modLine[256];

    // Array for output using sprintf
    char output[256];

    // Array to hold client line values
    char clientValues[30][100];

    // Total number of clients
    int clientCount = 0;

    // File "assignment_5_input.txt" opened in read mode
    FILE *file = fopen("assignment_5_input.txt", "r");

    // Check if the file pointer is NULL
    if (file == NULL) {
        sprintf(output, "Unable to open the file");
        puts(output);
        exit(1);
    }

    // Total number of lines
    int lineCount = 0;

    // Continue to read lines until no more lines are left
    while (fgets(fileLine, 256, file)) {
        strcpy(modLine, fileLine);
        // Continue splitting line around " " until NULL
        char *ptr = strtok(modLine, " ");
        // If first letter is a then it is bank account
        if (*ptr == 'a') {
            createAccount(fileLine);
        } else {
            createClient(fileLine);
            strcpy(&clientValues[clientCount][0], fileLine);
            clientCount++;
        }
        lineCount++;
    }

    // Threads
    int i, err_thread;
    pthread_t threads[clientCount];

    if (pthread_mutex_init(&lock, NULL) != 0) {
        printf("\n mutex init failed\n");
        return 1;
    }

    Client *clientTemp = clientHead;
    for (i = 0; i < clientCount; i++) {
        err_thread = pthread_create(&threads[i], NULL, &transaction, &threads[i]);
        if (err_thread != 0)
            printf("\n Error creating thread %d", i);
        else {
            long int threadID = *((long int *) &threads[i]);
            clientTemp->threadID = threadID;
        }
        clientTemp = clientTemp->next;
    }

    for (i = 0; i < clientCount; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&lock);

    // Calls function to print each account balance
    printList();

    return 0;
}