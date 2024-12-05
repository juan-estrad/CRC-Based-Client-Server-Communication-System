#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/select.h>

#include<string.h>

#define MAX_CLIENTS 10
#define MAX_BUFFER_SIZE 256

int pid;
int clientSelect;
int numMessages;
int msgLength;
char *useMsg;
int readfd;
int writefd;
char clientFifo[256];  
char storedValue[256];
int storedValueAvailable = 0;
char *NA = "NA";
int client_writefds[MAX_CLIENTS];
int counter = 1;
int counterPID;
int msgType;

// length of the generator polynomial  
#define GEN_POLY_LENGTH strlen(gen_poly)  
// data to be transmitted and received  
char dataSent[255];  
// CRC value  
char check_value[255];  
// generator polynomial  
char gen_poly[255] = "1010";  
// variables   
int data_length,i,j; 

//CRC Server Process  
void serverCrcGeneration(){  
    //Get Sender's Message Length
    data_length = strlen(useMsg);
    printf("\nOriginal Data: %s\n", useMsg);
    //Append n-1 zeros to the data  
    for(i=data_length;i<data_length+GEN_POLY_LENGTH-1;i++) {
        useMsg[i]='0';  
    }
    printf("\nData with Appended Zeros: %s\n", useMsg);

    //Perform CRC generation
    printf("\nUsing Generator Polynomial: %s\n", gen_poly);
    //Initializing check_value  
    for(i=0;i<GEN_POLY_LENGTH;i++)  
        check_value[i]=useMsg[i];  
    do{  
        //Check if the first bit is 1 and Perform XOR Operation  
        if(check_value[0]=='1')
            //If both bits are the same, the output is 0  
            //If the bits are different the output is 1  
            for(j = 1;j < GEN_POLY_LENGTH; j++)  
            check_value[j] = (( check_value[j] == gen_poly[j])?'0':'1');  
        //Move the bits by 1 position for the next computation  
        for(j=0;j<GEN_POLY_LENGTH-1;j++)  
            check_value[j]=check_value[j+1];  
        // appending a bit from data  
        check_value[j]=useMsg[i++];  
    }while(i<=data_length+GEN_POLY_LENGTH-1);  

    // Append data with check_value(CRC)    
    for(i=data_length;i<data_length+GEN_POLY_LENGTH-1;i++) {
        useMsg[i]=check_value[i-data_length];
    }
    printf("\nData with CRC Checksum: %s\n", useMsg);
}

//MESSAGE QUEUE

//Structure to represent a message
typedef struct {
    int type;
    int length;
    char *data;
    char crc[10]; //CRC checksum
} Message;

//Structure to represent a node in the message queue
typedef struct Node {
    Message message;
    struct Node *next;
} Node;

//Structure to represent the message queue
typedef struct {
    Node *front;
    Node *rear;
} Queue;

// Global for message queue
Queue messageQueue; 

//Initialize the message queue
void initQueue(Queue *q) {
    q->front = q->rear = NULL;
}

//Enqueue a message into the message queue
void enqueue(Queue *q, Message msg) {
    Node *newNode = (Node *)malloc(sizeof(Node));
    if (newNode == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    newNode->message = msg;
    newNode->next = NULL;

    if (q->rear == NULL) {
        q->front = q->rear = newNode;
    } else {
        q->rear->next = newNode;
        q->rear = newNode;
    }
}

// Modify the enqueue function to accept a char* parameter
void enqueueMsg(Queue *q, char *useMsg) {
    // Allocate memory for the new node
    Node *newNode = (Node *)malloc(sizeof(Node));
    if (newNode == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    // Set the message data for the new node
    newNode->message.data = strdup(useMsg);
    // Calculate the length of the message
    newNode->message.length = strlen(useMsg);
    // Set the type of the message (you need to define a type or use a default value)
    newNode->message.type = msgType;
    // Set the next pointer to NULL
    newNode->next = NULL;

    // If the queue is empty, set both front and rear to the new node
    if (q->rear == NULL) {
        q->front = q->rear = newNode;
    } else {
        // Otherwise, add the new node to the end of the queue
        q->rear->next = newNode;
        q->rear = newNode;
    }
}

//Print the contents of the message queue
void printMessageQueue(Queue* queue) {
    Node* current = queue->front;

    // Check if the message queue is empty
    if (current == NULL) {
        printf("Message queue is empty.\n");
        return;
    }
    printf("\nMessage Queue:\n");
    while (current != NULL) {
        printf("Type: %d, Length: %d, Data: %s\n", current->message.type, current->message.length, current->message.data);
        current = current->next;
    }
}


//PENDING REQUESTS QUEUE

//Structure to represent a client request
typedef struct {
    int clientPID;
    int messageType;
    int counterPID;
}Request;

//Structure to represent a node in the pending request queue
typedef struct RequestNode {
    int clientPID;  // Process ID of the client
    int messageType; // Requested message type
    int counterPID;
    struct RequestNode *next;
} RequestNode;

//Structure to represent the pending request queue
typedef struct {
    RequestNode *front;
    RequestNode *rear;
} RequestQueue;

// Global for pending requests queue
RequestQueue pendingRequestsQueue; 

//Initialize the pending request queue
void initRequestQueue(RequestQueue *rq) {
    rq->front = rq->rear = NULL;
}

//Enqueue a pending request into the pending request queue
void enqueueRequest(RequestQueue *rq, int pid, int messageType, int counterPID) {
    RequestNode *newNode = (RequestNode *)malloc(sizeof(RequestNode));
    if (newNode == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    newNode->clientPID = pid;
    newNode->messageType = messageType;
    newNode->counterPID = counterPID;
    newNode->next = NULL;

    if (rq->rear == NULL) {
        rq->front = rq->rear = newNode;
    } else {
        rq->rear->next = newNode;
        rq->rear = newNode;
    }
}

//Dequeue a pending request from the pending request queue
RequestNode dequeueRequest(RequestQueue *rq) {
    if (rq->front == NULL) {
        fprintf(stderr, "Pending request queue is empty\n");
        exit(EXIT_FAILURE);
    }
    RequestNode *temp = rq->front;
    RequestNode request = *temp;
    rq->front = rq->front->next;
    if (rq->front == NULL) {
        rq->rear = NULL;
    }
    free(temp);
    return request;
}

//Process client's pending requests for messages of specific types
void processPendingRequests(int messageType) {
    while (pendingRequestsQueue.front != NULL) {
        // Dequeue the next pending request
        RequestNode request = dequeueRequest(&pendingRequestsQueue);

        // Check if the messageType of the dequeued request matches the specified messageType
        if (request.messageType == messageType) {
            // Send the requested message to the client
            int clientPID = request.clientPID;
            int counterPID = request.counterPID;
            Node *current = messageQueue.front;
            Node *prev = NULL;
            while (current != NULL) {
                if (current->message.type == messageType) {
                    // Send the message to the pending client's fd
                    write(client_writefds[counterPID], (const void *)current->message.data, strlen(current->message.data));
                    printf("\nFulfilled Pending Request\n");
                    printf("Sent message of type %d to client %d\n", messageType, clientPID);
                    
                    // Remove the message from the message queue
                    if (prev == NULL) {
                        messageQueue.front = current->next;
                        if (messageQueue.front == NULL) {
                            messageQueue.rear = NULL;
                        }
                    } else {
                        prev->next = current->next;
                        if (current->next == NULL) {
                            messageQueue.rear = prev;
                        }
                    }
                    free(current);

                    // Exit the loop after sending the message
                    break;
                }
                prev = current;
                current = current->next;
            }
        } else {
            // If the request type doesn't match the current message type,
            // enqueue it back to the pending requests queue for later processing
            enqueueRequest(&pendingRequestsQueue, request.clientPID, request.messageType, request.counterPID);
        }
    }
}

//Print the contents of the Pending Requests queue
void printPendingRequestsQueue(RequestQueue* queue) {
    RequestNode* current = queue->front;

    // Check if the pending requests queue is empty
    if (current == NULL) {
        printf("Pending Requests queue is empty.\n");
        return;
    }

    printf("\nPending Requests Queue:\n");
    while (current != NULL) {
        printf("ClientPID: %d, Requested Type: %d\n", current->clientPID, current->messageType);
        current = current->next;
    }
}

//Process Client's Request
void processClientRequest(const char *data) {
    char *token;
    char *dataCopy = strdup(data);

    token = strtok(dataCopy, ",");
    pid = atoi(token);

    token = strtok(NULL, ",");
    counterPID = atoi(token);

    token = strtok(NULL, ",");
    clientSelect = atoi(token);
    printf("\nRequest Recevied from Client %d:\n", pid);
    if (clientSelect == 1) {
        printf("Connect Call\n\n");
    }
    else if(clientSelect == 2) {
        printf("Message Send\n\n");
    }
    else if (clientSelect == 3) {
        printf("Message Receive\n\n");
    }
    else if (clientSelect == 0) {
        printf("Exit\n\n");
    }
    else if (clientSelect == -1) {
        printf("Shutdown\n\n");
    }
    else {
        printf(" \n");
    }

    printf("Client ID: %d\n", pid);

    token = strtok(NULL, ",");
    numMessages = atoi(token);

    //Message Type
    token = strtok(NULL, ",");
    msgType = atoi(token);
    printf("Message Type: %d\n", msgType);

    //Message Length
    token = strtok(NULL, ",");
    msgLength = atoi(token);

    //Message Data
    if (numMessages > 0) {
        for (int i = 0; i < numMessages; i++) {
            token = strtok(NULL, ",");
            if (token == NULL) {
                break;
            }

            // Dynamically allocate memory for msgValue
            char *msgValue = strdup(token);

                printf("Msg: %s\n", msgValue);
                strncpy(clientFifo, msgValue, sizeof(clientFifo));  
                useMsg = msgValue;
                //Msg Object
                Message msg = {msgType, msgLength, strdup(useMsg)};

        //Open client specific fifo and decide return value based on Client Select Number
        if (clientSelect == 1) {
                writefd = open(clientFifo, O_WRONLY);
                if (writefd == -1) {
                    printf("Error Opening client FIFO");
                }
                else{
                printf("\nOpened %s in WRITE mode\n",clientFifo);
                }
                char counterMsg[255];
                sprintf(counterMsg, "%d", counter);
                //Store client's writefd  into client_writefds ARRAY
                client_writefds[counter] = writefd;
                write(client_writefds[counter], counterMsg, strlen(counterMsg));   
                counter++;
            }
            if (clientSelect == 2) {
                //PERFORM CRC GENERATION and APPEND to MSG
                serverCrcGeneration();

                //Message Send
                write(client_writefds[counterPID], useMsg, strlen(useMsg));  
                printf("\nCheck Value is: %s\n", check_value); 
                printf("\nMessage Sent is: %s\n", useMsg); 

                //Enqueue Msg
                enqueueMsg(&messageQueue, useMsg);

                //Print Message queue Contents
                if (messageQueue.front == NULL && messageQueue.rear == NULL) {
                    printf("Message Queue is Empty\n");
                }else{
                    printMessageQueue(&messageQueue);
                }
                
                //Check if Pending requests can be satisfied
                //Fulfill if possible
                processPendingRequests(msgType);
            }
            if (clientSelect == 3) {
                write(client_writefds[counterPID], useMsg, strlen(useMsg));     
                printf("Message Received is: %s\n", useMsg);          
            }

      free(msgValue); 
        }
    } 
    else {
            //Message Receive
             if (clientSelect == 3) {
                int requestedType = msgType;
                Message msgToSend;
                //Dequeue Message if message with requested Type is found in messageQueue
                if (messageQueue.front != NULL) {
                    Node *current = messageQueue.front;
                    Node *prev = NULL;
                    while (current != NULL) {
                        if (current->message.type == requestedType) {
                            msgToSend = current->message;
                            if (prev == NULL) {
                                messageQueue.front = current->next;
                                if (messageQueue.front == NULL) {
                                    messageQueue.rear = NULL;
                                }
                            } else {
                                prev->next = current->next;
                                if (current->next == NULL) {
                                    messageQueue.rear = prev;
                                }
                            }
                            free(current);

                            //Send dequeued message of correct Type requested
                            write(client_writefds[counterPID], msgToSend.data, strlen(msgToSend.data));     
                            printf("Message Received is: %s\n", msgToSend.data);
                            break;
                        }
                        prev = current;
                        current = current->next;
                    }
                    if (current == NULL) {
                        printf("No message of this type in queue.\n");
                        //Request Object
                        Request request;
                        request.clientPID = pid;
                        request.messageType = msgType;
                        request.counterPID = counterPID;
                        //Enqueue client waiting for message of specific Type into pendingRequestsQueue
                        enqueueRequest(&pendingRequestsQueue, request.clientPID, request.messageType, request.counterPID);
                        //Print pendingRequestsQueue
                        printPendingRequestsQueue(&pendingRequestsQueue);
                    }
                }
 
            }
    }

    free(dataCopy);

}

int main(int argc, char* argv[]) {

    //Initialize Message Queue
    initQueue(&messageQueue);

    //Initialize Pending Requests Queue
    initRequestQueue(&pendingRequestsQueue);

    //Create well-known server FIFO
    if (mkfifo("serverFifo", 0777) == -1) {
        if (errno != EEXIST) {
            printf("Fifo file creation was unsuccessful\n");
            return 1;
        }
    }
    //Open server FIFO in Read/Write mode
    readfd = open("serverFifo", O_RDWR);
    if (readfd == -1) {
        return 1;
    }

    //Read Client's data from buffer
    char buffer[256];
    while(1) {
    ssize_t bytesRead = read(readfd, buffer, sizeof(buffer));
    if (bytesRead == -1) {
        return 1;
    }
    if (bytesRead > 0) {
        buffer[bytesRead] = '\0';
        processClientRequest(buffer);
        printf("\n");
    }
    //Client sends Exit
    if (clientSelect == 0) {
        writefd ++;
        close(writefd);
        unlink(clientFifo);
        printf("\nClient %d has Exited.\n",pid);
    }
    //Client sends Shutdown
    if (clientSelect == -1) {
        close(readfd);
        unlink("serverFifo");
        printf("\nServer Shutdown\n");
        break;
    }
        usleep(100000);
    }

    return 0;

}

/*
gcc -o server server.c
gcc -o client client.c
*/ 