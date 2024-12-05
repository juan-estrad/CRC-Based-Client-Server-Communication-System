#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

int readfd;             
int writefd;    
char clientNfifo[255]; 
int receiverSelect;    
int counterPID; 
int ErrorIntro = 0;

char buffer[256];
// length of the generator polynomial  
#define GEN_POLY_LENGTH strlen(gen_poly)  
// CRC value  
char check_value[255];  
// generator polynomial  
char gen_poly[255] = "1010";  
// variables   
int data_length,i,j;  

//Flip a bit
void flip_bit(char *buffer, int bit_index) {
    // Ensure bit_index is valid
    if (bit_index >= 0 && bit_index < strlen(buffer)) {
        // Toggle the bit at the specified index
        buffer[bit_index] = (buffer[bit_index] == '0') ? '1' : '0';
    }
}

//ERROR CORRECTION
void errorCorrection(char *buffer, char *msgWithError) {
    //SYNDROME Table
    printf("\nSYNDROME TABLE");
    data_length = strlen(buffer);
    char flippedMsg[data_length][256];
    for (i=0; i < data_length; i++) {
        //Initialize copy of buffer
        strcpy(flippedMsg[i], buffer);

        //Generate Syndrome table
        flip_bit(flippedMsg[i], i);
        printf("\nError Bit %d Message is: %s\n", i, flippedMsg[i]);
    }
    //Find match for Error Message in the Syndrome Table 
    for (int i = 0; i < data_length; i++) {
        if (strcmp(msgWithError, flippedMsg[i]) == 0) { // Compare Error Message with Syndrome Table entries
            flip_bit(flippedMsg[i], i); // Flip the bit again to correct the error
            printf("\nError Found at Error Bit %d.\n", i);
            printf("Corrected Message: %s\n", flippedMsg[i]);
            break;
        }
    }
}

//ERROR DETECTION
void errorDetection(char *buffer){
    data_length = strlen(buffer);

    //Perform CRC on Message Received
    // initializing check_value  
    for(i=0;i<GEN_POLY_LENGTH;i++)  
        check_value[i]=buffer[i];  
    do{  
    // check if the first bit is 1 and Perform XOR Operation  
        if(check_value[0]=='1') 
            // if both bits are the same, the output is 0  
            // if the bits are different the output is 1  
            for(j = 1;j < GEN_POLY_LENGTH; j++)  
            check_value[j] = (( check_value[j] == gen_poly[j])?'0':'1'); 
        // Move the bits by 1 position for the next computation  
        for(j=0;j<GEN_POLY_LENGTH-1;j++)  
            check_value[j]=check_value[j+1];  
        // appending a bit from data  
        check_value[j]=buffer[i++];  
    }while(i<=data_length+GEN_POLY_LENGTH-1);  
    // Check if the remainder is zero to find the error  
    for (i = 0; (i < GEN_POLY_LENGTH - 1) && (check_value[i] != '1'); i++);
    if (i < GEN_POLY_LENGTH - 1) {
        printf("\nError detected\n\n");
    } else {
        printf("\nNo error detected\n\n");
    } 
}

//Reads counterPID value given by server
int readCounter(int readfd) {
    char buffer[256];
    ssize_t bytes_read = read(readfd, buffer, sizeof(buffer) - 1);
    if (bytes_read == -1) {
        printf("Error reading server response from client specific FIFO");
        exit(EXIT_FAILURE);
    }
    buffer[bytes_read] = '\0';
    if (receiverSelect == 1) {
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
    if (receiverSelect == 1) {
        counterPID = atoi(buffer);
    }
    else if (receiverSelect == 2) {
        printf("\nMessage Sent: %s\n", buffer);
    }
    else if (receiverSelect == 3) {
        //INTRODUCE ERROR (Uncomment ErrorIntro = 1)
        ErrorIntro = 1;
        char msgWithError[255];        
        strcpy(msgWithError, buffer);
        srand(time(NULL));
        int bitToFlip = rand() % strlen(buffer);
        flip_bit(msgWithError, bitToFlip);

        if (ErrorIntro == 1) {
            printf("\nMessage Received: %s\n", msgWithError);
            //Error Detection
            errorDetection(msgWithError);
            //Error Correction
            errorCorrection(buffer, msgWithError);
        } else {
            printf("\nMessage Received: %s\n", buffer);
            //Error Detection
            errorDetection(buffer);
        }


    }
}

//Send Receiver Request to server
void sendReceiverRequest(int writefd, const char *receiverString) {
    write(writefd, receiverString, strlen(receiverString));
}

//Get user input for Receiver Request
void handleReceiverRequest(int writefd) {

    int client_ID = getpid();
    
    printf("Enter 2 to Send Message or 3 to Receive: ");
    scanf("%d", &receiverSelect);

    int numMessages;
    if (receiverSelect == 3) {
        numMessages = 0;
        printf("\nMessage Request by Type \n");
    } 
    else if (receiverSelect == 2) {
        numMessages = 1;
        printf("Please Enter your Message\n");
    }

    //Message Type
    printf("Enter the Message Type: ");
    int msgType;
    scanf("%d", &msgType);

    //Message Length
    int msgLength;
    msgLength = numMessages * 4;

    char receiverString[256];
    sprintf(receiverString, "%d, %d, %d, %d, %d, %d,", client_ID, counterPID, receiverSelect, numMessages, msgType, msgLength);

    //Get Message Data from User
    if (numMessages > 0) {
        for (int i = 0; i < numMessages; i++) {
            printf("Enter Message: ");

            char *msgValue = malloc(256);

            // Check if msgValue is an integer
            if (scanf("%d", (int *)msgValue) == 1) {
                sprintf(receiverString + strlen(receiverString), "%d,", *((int *)msgValue));
            } else {  // If not an integer, treat as string
                scanf("%s", msgValue);
                sprintf(receiverString + strlen(receiverString), "%s,", msgValue);
            }

            free(msgValue); 
        }
    } 

    sendReceiverRequest(writefd, receiverString);
    usleep(100000);
    printf("\nServer Response:");
    readResponse(readfd);
}


//Handles Exit Call
void handleExit(int writefd) {
    int client_ID = getpid();
    int receiverSelect = 0;
    int numMessages = 0;
    int msgType = 0;
    int msgLength = 0;
    
 
    char receiverString[256];
    sprintf(receiverString, "%d, %d, %d, %d, %d, %d,", client_ID, counterPID, receiverSelect, numMessages, msgType, msgLength);

    sendReceiverRequest(writefd, receiverString);
}

//Handls Shutdown Call
void handleShutdown(int writefd) {
    int client_ID = getpid();
    int receiverSelect = -1;
    int numMessages = 0;
    int msgType = 0;
    int msgLength = 0;
 
    char receiverString[256];
    sprintf(receiverString, "%d, %d, %d, %d, %d, %d,", client_ID, counterPID, receiverSelect, numMessages, msgType, msgLength);

    sendReceiverRequest(writefd, receiverString);
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
    int receiverSelect = 1;
    int numMessages = 1;
    int msgType = 0;
    int msgLength = 5;

    char receiverString[256];
    sprintf(receiverString, "%d, %d, %d, %d, %d, %d,", client_ID, counterPID, receiverSelect, numMessages, msgType, msgLength);
    sprintf(receiverString + strlen(receiverString), "%s,", clientNfifo);

    sendReceiverRequest(writefd, receiverString);
    
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
    if (receiverSelect = 1) {
        readCounter(readfd);
    }

    //Handle user decision to make call or exit
    int begin = 1;
    printf("Please Enter 1 to send request to server, 0(Exit), 3(SHUTDOWN): \n");
    int makeCall;
    scanf("%d", &makeCall);
    while(begin == 1) {

    if (makeCall == 1) {
        handleReceiverRequest(writefd);
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