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

int clientCount = 0;
char delim[] = " ";

// Array to hold client line values
char clientValues[10][100];

Account *head;
Account *accountPrevious = NULL;

Client *clientHead;
Client *clientPrevious = NULL;

int balance = 0;

// Array for output using sprintf
char output[256];

// Using struct to hold account information
struct Accounts {
    char accountName[10];
    int balance;
    Account *next;
};

typedef struct Clients {
    char clientName[10];
    char actions[256];
    int lineComplete;
    Client *next;
} Clients;


void *transaction(void *thread_id) {

    pthread_mutex_lock(&lock);  // ENTRY
    long int *temp = (long int *) thread_id;
    long int thread_name = *temp;
    int transactionComplete = 0;
    sprintf(output, "\n\n****thread ID %ld****", thread_name);
    puts(output);

    Client *clientTemp = clientHead;
    while (clientTemp != NULL && transactionComplete == 0) {
        if (clientTemp->lineComplete == 0) {
            sprintf(output, "%s", clientTemp->actions);
            puts(output);
            int action;
            char *ptr = strtok(clientTemp->actions, delim);
            ptr = strtok(NULL, delim);
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
                ptr = strtok(NULL, delim);
                ptr[strcspn(ptr, "\r\n")] = 0;

                // Finds the balance of the account to be changed
                Account *aTemp = head;
                while (aTemp != NULL) {
                    if (strcmp(aTemp->accountName, ptr) == 0) {
                        balance = aTemp->balance;
                        break;
                    } else {
                        aTemp = aTemp->next;
                    }
                }

                sprintf(output, "Starting Balance %s %d", ptr, balance);
                puts(output);
                ptr = strtok(NULL, delim);
                ptr[strcspn(ptr, "\r\n")] = 0;
                int value = atoi(ptr);
                if (action == 1) {
                    balance = balance + value;
                    aTemp->balance = balance;
                }
                else if (action == 0 && balance >= value) {
                    balance = balance - value;
                    aTemp->balance = balance;
                }
                sprintf(output, "Ending Balance %s %d\n", aTemp->accountName, balance);
                puts(output);

                // Next value in the line
                ptr = strtok(NULL, delim);
            }

            clientTemp->lineComplete = 1;
            transactionComplete = 1;
        }
        clientTemp = clientTemp->next;
    }
    pthread_mutex_unlock(&lock); // EXIT
}

void printList() {
    Account *temp = head;

    while (temp != NULL) {
        sprintf(output,"%s b %d", temp->accountName, temp->balance);
        puts(output);
        temp = temp->next;
    }
}

char accountArray[3][15];
int count = 0;

Account *createAccount(char *line) {
    count = 0;
    // Splitting line around " " until NULL
    char *ptr = strtok(line, delim);
    while (ptr != NULL) {
        ptr[strcspn(ptr, "\r\n")] = 0;
        strcpy(&accountArray[count][0], ptr);
        ptr = strtok(NULL, delim);
        count++;
    }
    Account *account = malloc(sizeof(Account));
    //account->accountID = i;
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
    //account->accountID = i;
    // Copy line into the actions
    strcpy(client->actions, line);
    char *ptr = strtok(line, delim);
    ptr[strcspn(ptr, "\r\n")] = 0;
    strcpy(client->clientName, ptr);
    // Line complete 0 means the transaction has not been complete
    client->lineComplete = 0;
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

    // File "assignment_5_input.txt" opened in read mode
    FILE *file = fopen("/Users/rishabhjain/Documents/GitHub/asn5/assignment_5_input.txt", "r");

    // Check if the file pointer is NULL
    if (file == NULL) {
        sprintf(output, "Unable to open the file");
        puts(output);
        exit(1);
    }

    //int accountCount = 0;
    int lineCount = 0;
    // Continue to read lines until no more lines are left
    while (fgets(fileLine, sizeof(fileLine), file)) {
        strcpy(modLine, fileLine);
        // Continue splitting line around " " until NULL
        char *ptr = strtok(modLine, delim);
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



    // Number of clients will be total lines minus the number of bank accounts
    int accountCount = lineCount - clientCount;

    // Threads
    int i, err_thread;

    pthread_t threads[clientCount];

    if (pthread_mutex_init(&lock, NULL) != 0) {
        printf("\n mutex init failed\n");
        return 1;
    }

    for (i = 0; i < clientCount; i++) {
        err_thread = pthread_create(&threads[i], NULL, &transaction, &threads[i]);

        if (err_thread != 0)
            printf("\n Error creating thread %d", i);
    }

    for (i = 0; i < clientCount; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&lock);

    printList();

    return 0;
}