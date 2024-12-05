# CRC-Based Client-Server Communication System

## Abstract
In this project, we investigated error correction and detection using **Cyclic Redundancy Check (CRC)** and explored how it can be used in a client-server data transmission environment. To investigate this, we created three programs: **server**, **sender**, and **receiver**, capable of message passing. These programs implement the CRC process for error detection and utilize a syndrome table for error correction. The expected outcome was a working client-server network where messages are transmitted with the CRC checksum appended, allowing error detection. If an error is detected, error correction is performed using the syndrome table.

---

## Features
1. **CRC Error Detection**:
   - The server and receiver detect errors in transmitted data using CRC.
   - The receiver checks for errors in received messages and, if necessary, performs error correction.

2. **Error Correction with Syndrome Table**:
   - The receiver uses a **syndrome table** to identify and correct errors by flipping specific bits in the received message.

3. **Message Passing**:
   - **FIFOs (Named Pipes)** are used for communication between the server, sender, and receiver.
   - The sender sends messages, the server processes them, and the receiver detects and corrects any errors in the messages.

4. **Client-Server Interaction**:
   - The sender program sends messages to the server with a CRC checksum.
   - The receiver program detects and corrects errors in the messages before printing the corrected message.

---

## Files
- **`server.c`**: Implements the server logic that handles incoming messages, performs CRC error detection, and manages message queues.
- **`sender.c`**: Implements the client (sender) logic that sends messages to the server, including the CRC checksum for error detection.
- **`receiver.c`**: Implements the receiver logic, responsible for error detection, correction using the syndrome table, and displaying the final message.

---

## Setup

### Prerequisites
- **C** compiler
- **UNIX/Linux-based system** (for FIFO and file handling)

### Compilation
To compile the three programs:
```bash
gcc -o server server.c
gcc -o sender sender.c
gcc -o receiver receiver.c
```

### Execution
1. Start the server:
```bash
./server
```
2. Start the sender (client):
```bash
./sender
```
3. Start the receiver (in a separate terminal):
```bash
./receiver
```

---

# Usage

## Sender Program
The sender sends messages to the server with an appended CRC checksum for error detection. The user can choose to send a message, request a message, or exit the program.

- **1**: Send a request to the server (send a message or request a message).
- **0**: Exit the sender program.
- **3**: Shutdown the server.

## Receiver Program
The receiver handles received messages, detecting errors using CRC and correcting any errors using a syndrome table.

- **2**: Send a message to the server.
- **3**: Receive a message from the server, detect errors, and correct them if needed.
- **0**: Exit the receiver program.

---

## Example Workflow

### Sender (Client) Workflow
1. **Send a Message**:
   - The sender program allows the user to input a message, which is then sent to the server. The CRC checksum is appended to the message for error detection.

2. **Receive a Message**:
   - The sender can request a message from the server. The message will be received along with CRC validation, ensuring the integrity of the data.

### Server Workflow
1. The server receives messages, performs CRC error detection, and responds to the sender.

2. If an error is detected in the message, the server can either request a retransmission or pass the message to the receiver for correction.

### Receiver Workflow
1. **Error Detection**:
   - The receiver uses CRC to detect any errors in the received message.
   - If no error is detected, the receiver prints the message as is.

2. **Error Correction**:
   - If an error is detected, the receiver uses a syndrome table to identify and correct the erroneous bit by flipping it.
   - After correction, the message is printed as the corrected message.

---

## Limitations
- The system assumes unique client identifiers for communication.
- Only basic CRC error detection and correction is implemented with a single generator polynomial.

---
## License

[MIT](https://choosealicense.com/licenses/mit/)
