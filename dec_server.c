#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>

/*
* Decryption server code:
* 1. Accept connection from dec_client
* 2. Create a socket to listen to client requests
* 3. Spawn a child process and check for correct connection
* 4. Read messages from client and do decryption
* 5. Send the plain text to the dec_client
*/

// Maximum read size
#define MAX_SIZE 1000
// End of file indentifier
#define EOF_CHAR "@"
// dec_client indentifier
#define DEC_PREFIX "D!"

// Set up the address struct for the server socket
void setupAddressStruct(struct sockaddr_in* address, int portNumber)
{
    // Clear out the address struct
    memset((char*)address, '\0', sizeof(*address));
    // The address should be network capable
    address->sin_family = AF_INET;
    // Store the port number
    address->sin_port = htons(portNumber);
    // Allow a client at any address to connect to this server
    address->sin_addr.s_addr = INADDR_ANY;
}

int main(int argc, char* argv[])
{
    int connectionSocket, charsRead, charsWritten, childStatus;
    char input_buffer[200000];
    struct sockaddr_in serverAddress, clientAddress;
    socklen_t sizeOfClientInfo = sizeof(clientAddress);
    // Check for correct connection
    int connected = 1;

    // Check usage & args
    if (argc < 2)
    {
        fprintf(stderr, "USAGE: %s port\n", argv[0]);
        exit(1);
    }

    // Create the socket that will listen for connections
    int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket < 0)
    {
        fprintf(stderr, "ERROR opening socket\n");
    }

    // Set up the address struct for the server socket
    setupAddressStruct(&serverAddress, atoi(argv[1]));

    // Associate the socket to the port
    if (bind(listenSocket,
        (struct sockaddr*)&serverAddress,
        sizeof(serverAddress)) < 0)
    {
        fprintf(stderr, "ERROR on binding\n");
        close(listenSocket);
        exit(1);
    }

    // Start listening for connetions. Allow up to 5 connections to queue up
    listen(listenSocket, 5);

    // Accept a connection, blocking if one is not available until one connects
    while (1)
    {
        // Accept the connection request which creates a connection socket
        connectionSocket = accept(listenSocket,
            (struct sockaddr*)&clientAddress, &sizeOfClientInfo);
        if (connectionSocket < 0)
        {
            fprintf(stderr, "ERROR on accept\n");
        }

        // Spawn a child process
        pid_t childPid = fork();
        switch(childPid)
        {
        case -1:
            fprintf(stderr, "ERROR failed fork()\n");
            break;

        case 0:
            // Child process
            memset(input_buffer, '\0', 200000);
            // Create a socket buffer to read in MAX_READ characters
            char read_buffer[MAX_SIZE];
            // Set a flag if prefix needs to be checked
            int check_prefix = 1;

            // Read the client's message from the socket
            // Until it reads EOF_CHAR
            while (strstr(read_buffer, EOF_CHAR) == NULL)
            {
                // Read from the socket
                memset(read_buffer, '\0', MAX_SIZE);
                charsRead = recv(connectionSocket, read_buffer, MAX_SIZE - 1, 0);
                if (charsRead < 0)
                {
                    fprintf(stderr, "ERROR reading from socket\n");
                }
                // Check for prefix match to make sure it is communicating with dec_client
                if (check_prefix)
                {
                    // Get first 2 characters
                    char prefix[2];
                    strncpy(prefix, read_buffer, 2);
                    // Check for bad connection
                    if (strcmp(prefix, DEC_PREFIX) != 0)
                    {
                        // Send "0" for bad connection
                        charsWritten = send(connectionSocket, "0", 1, 0);
                        if (charsWritten < 0)
                        {
                            fprintf(stderr, "ERROR reading from socket\n");
                        }
                        // Set flag 0 for bad connection
                        connected = 0;
                        break;
                    }
                    // Good connection
                    else
                    {
                        check_prefix = 0;
                    }
                }
                // Add read buffer to the main buffer
                strcat(input_buffer, read_buffer);
            }

            // If Connection is good, do decryption
            if (connected)
            {
                // Remove EOF_CHAR & first 2 chars
                input_buffer[strlen(input_buffer) - 1] = '\0';
                char* temp = input_buffer + 2;
                memmove(input_buffer, temp, strlen(input_buffer));

                // Find output text length which is half of the input buffer length
                long text_length = strlen(input_buffer) / 2;

                // Arrays for cipher text, key, plain text
                char cipher_text[text_length + 1];
                char key[text_length + 1];
                // Add 2 for null char and EOF_CHAR
                char plain_text[text_length + 2];

                // Set all byte to NULL terminator
                memset(cipher_text, '\0', text_length + 1);
                memset(key, '\0', text_length + 1);
                memset(plain_text, '\0', text_length + 2);

                // Copy the first half into cipher text and second half into key
                strncpy(cipher_text, input_buffer, text_length);
                strcpy(key, &input_buffer[text_length]);

                // Decryption
                for (int i = 0; i < text_length; i++)
                {
                    // Convert capital alphabet to integer 0-25
                    int text_i = cipher_text[i] - 65;;
                    int key_i = key[i] - 65;
                    // Convert space to int 26
                    if (text_i == -33)
                    {
                        text_i = 26;
                    }
                    if (key_i == -33)
                    {
                        key_i = 26;
                    }
                    // ciphertext – key
                    int temp = text_i - key_i;
                    // If subtraction result is negative
                    // Add 27 to make it zero or higher
                    if (temp < 0)
                    {
                        temp += 27;
                    }
                    int modulo_result = temp % 27;
                    // Convert modulo result 26 to space
                    if (modulo_result == 26)
                    {
                        plain_text[i] = 32;
                    }
                    // Convert modulo result to capital alphabet
                    else
                    {
                        plain_text[i] = modulo_result + 65;
                    }
                }
                // Add EOF_CHAR at the end
                strcat(plain_text, EOF_CHAR);

                // Send the plain text back to the client
                charsWritten = send(connectionSocket, plain_text, strlen(plain_text), 0);
                if (charsWritten < 0)
                {
                    fprintf(stderr, "ERROR writing to socket\n");
                }
                if (charsWritten < strlen(plain_text))
                {
                    fprintf(stderr, "SERVER: WARNING: Not all data written to socket!\n");
                }
            }
            // Close the connection socket for this client
            close(connectionSocket);
            // Exit the child process
            exit(0);
            break;

        default:
            // Non-blocking wait
            childPid = waitpid(childPid, &childStatus, WNOHANG);
        }
    }
    // Close the listening socket
    close(listenSocket);
    return 0;
}