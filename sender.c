#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

int readfd;             
int writefd;    
char clientNfifo[255]; 
int senderSelect;    
int counterPID; 

//Reads counterPID value given by server
int readCounter(int readfd) {
    char buffer[256];
    ssize_t bytes_read = read(readfd, buffer, sizeof(buffer) - 1);
    if (bytes_read == -1) {
        printf("Error reading server response from client specific FIFO");
        exit(EXIT_FAILURE);
    }
    buffer[bytes_read] = '\0';
    if (senderSelect == 1) {
        counterPID = atoi(buffer);
    }
}

//Read response from server
void readResponse(int readfd) {
    char buffer[256];
    ssize_t bytes_read = read(readfd, buffer, sizeof(buffer) - 1);
    if (bytes_read == -1) {
        printf("Error reading server response from client specific FIFO");
        exit(EXIT_FAILURE);
    }
    buffer[bytes_read] = '\0';
    if (senderSelect == 1) {
        counterPID = atoi(buffer);
    }
    else if (senderSelect == 2) {
        printf("\nMessage Sent: %s\n", buffer);
    }
    else if (senderSelect == 3) {
        printf("\nMessage Received: %s\n", buffer);
    }
}

//Send Request to server
void sendSenderRequest(int writefd, const char *senderString) {
    write(writefd, senderString, strlen(senderString));
}

//Get user input for Sender Request
void handleSenderRequest(int writefd) {

    int client_ID = getpid();
    
    printf("Enter 2 to Send Message or 3 to Receive: ");
    scanf("%d", &senderSelect);

    int numMessages;
    if (senderSelect == 3) {
        numMessages = 0;
        printf("\nMessage Request by Type \n");
    } 
    else if (senderSelect == 2) {
        numMessages = 1;
        printf("\nPlease Enter Message Information\n");
    }

    //Message Type
    printf("Enter the Message Type: ");
    int msgType;
    scanf("%d", &msgType);

    //Message Length
    int msgLength;
    msgLength = numMessages * 4;

    char senderString[256];
    sprintf(senderString, "%d, %d, %d, %d, %d, %d,", client_ID, counterPID, senderSelect, numMessages, msgType, msgLength);

    //Get Message Data from User
    if (numMessages > 0) {
        for (int i = 0; i < numMessages; i++) {
            printf("Enter Message: ");

            // Dynamically allocate memory for msgValue
            char *msgValue = malloc(256);

            // Check if msgValue is an integer
            if (scanf("%d", (int *)msgValue) == 1) {
                sprintf(senderString + strlen(senderString), "%d,", *((int *)msgValue));
            } else {  // If not an integer, treat as string
                scanf("%s", msgValue);
                sprintf(senderString + strlen(senderString), "%s,", msgValue);
            }

            free(msgValue); 
        }
    } 

    sendSenderRequest(writefd, senderString);
    usleep(100000);
    printf("\nServer Response:");
    readResponse(readfd);
}


//Handles Exit Call
void handleExit(int writefd) {
    int client_ID = getpid();
    int senderSelect = 0;
    int numMessages = 0;
    int msgType = 0;
    int msgLength = 0;
    
 
    char senderString[256];
    sprintf(senderString, "%d, %d, %d, %d, %d, %d,", client_ID, counterPID, senderSelect, numMessages, msgType, msgLength);

    sendSenderRequest(writefd, senderString);
}

//Handls Shutdown Call
void handleShutdown(int writefd) {
    int client_ID = getpid();
    int senderSelect = -1;
    int numMessages = 0;
    int msgType = 0;
    int msgLength = 0;
 
    char senderString[256];
    sprintf(senderString, "%d, %d, %d, %d, %d, %d,", client_ID, counterPID, senderSelect, numMessages, msgType, msgLength);

    sendSenderRequest(writefd, senderString);
}

//Generates dynamic FIFO name for each client
void generateFifoName() {
    pid_t pid = getpid();
    sprintf(clientNfifo, "Client%dfifo", (int)pid);
}

//Handles Connect Call
char* connectCall(int writefd) {
    generateFifoName();
    int client_ID = getpid();
    int senderSelect = 1;
    int numMessages = 1;
    int msgType = 0;
    int msgLength = 5;

    char senderString[256];
    sprintf(senderString, "%d, %d, %d, %d, %d, %d,", client_ID, counterPID, senderSelect, numMessages, msgType, msgLength);
    sprintf(senderString + strlen(senderString), "%s,", clientNfifo);

    sendSenderRequest(writefd, senderString);
    
}

//Create the client specific FIFO and open in Read mode
int createReadClientFifo() {
    //Create client specific FIFO
    if (mkfifo(clientNfifo, 0777) == -1) {
    if (errno != EEXIST) {
        printf("Fifo file creation was unsuccessful\n");
        exit(EXIT_FAILURE);
        }
    }

    //Open client specific FIFO for reading
    int readfd = open(clientNfifo, O_RDONLY);
        if (readfd == -1) {
            printf("Failed to open client specific fifo for reading");
            exit(EXIT_FAILURE);
        }
        return readfd;
    }


int main(int argc, char* argv[]) {

    //Open server FIFO in Write mode
    writefd = open("serverFifo", O_WRONLY);
    if (writefd == -1) {
        printf("\nError Opening server FIFO\n");
        return 1;
    }

    //Connect call
    connectCall(writefd);
    readfd = createReadClientFifo();

    //Read counterPID from server
    if (senderSelect = 1) {
        readCounter(readfd);
    }

    //Handle user decision to make call or exit
    int begin = 1;
    printf("Please Enter 1 to send request to server, 0(Exit), 3(SHUTDOWN): \n");
    int makeCall;
    scanf("%d", &makeCall);
    while(begin == 1) {

    if (makeCall == 1) {
        handleSenderRequest(writefd);
        printf("\nPlease Enter 1 to send request to server, 0(EXIT), 3(SHUTDOWN): \n");
        scanf("%d", &makeCall);
    }
    else if (makeCall == 0){
        handleExit(writefd);
        close(readfd);
        unlink(clientNfifo);
        printf("\nEXITED\n");
        begin = 0;
    }
    else if (makeCall == 3) {
        handleShutdown(writefd);
        close(readfd);
        unlink(clientNfifo);
        printf("\nSHUTDOWN\n");
        begin = 0;       
    }
    else {
        printf("Please enter 1(Begin), 0(Exit) or 3(Shutdown): \n");
        scanf("%d", &makeCall);
    }
 
}

    close(writefd);

    return 0;
}