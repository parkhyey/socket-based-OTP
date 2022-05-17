#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>  // ssize_t
#include <sys/socket.h> // send(),recv()
#include <netdb.h>      // gethostbyname()
#include <netinet/in.h>

/*
* Encryption client code:
* 1. Create a socket and connect to the server specified in the command arugments.
* 2. Provide the server with a plain text file and a key file.
* 3. Print the encrypted text received from the server to stdout and exit the program.
*/

// Maximum read size
#define MAX_SIZE 1000
// End of file indentifier
#define EOF_CHAR "@"
// enc_client indentifier
#define ENC_PREFIX "E!"

// Set up the address struct
void setupAddressStruct(struct sockaddr_in* address,
    int portNumber,
    char* hostname)
{
    // Clear out the address struct
    memset((char*)address, '\0', sizeof(*address));

    // The address should be network capable
    address->sin_family = AF_INET;
    // Store the port number
    address->sin_port = htons(portNumber);

    // Get the DNS entry for this host name
    struct hostent* hostInfo = gethostbyname(hostname);
    if (hostInfo == NULL)
    {
        fprintf(stderr, "CLIENT: ERROR, no such host\n");
        exit(0);
    }
    // Copy the first IP address from the DNS entry to sin_addr.s_addr
    memcpy((char*)&address->sin_addr.s_addr,
        hostInfo->h_addr_list[0],
        hostInfo->h_length);
}

int main(int argc, char* argv[])
{
    // Open the input file for read only
    FILE* fp = fopen(argv[1], "r");
    char* text;
    long file_length;
    if (fp == NULL)
    {
        fprintf(stderr, "CLIENT: ERROR opening text file %s\n", argv[1]);
        exit(1);
    }
    else
    {
        // Find file length
        fseek(fp, 0, SEEK_END);
        file_length = ftell(fp);
        // Move file pointer position to the beginning of the file
        fseek(fp, 0, SEEK_SET);
        if (file_length)
        {
            text = malloc(sizeof * text * (file_length + 1));
            // Read line until a new line character
            fgets(text, sizeof * text * (file_length + 1), fp);
        }
        fclose(fp);
    }
    // Strip off the new line character
    if (text)
    {
        text[file_length - 1] = '\0';
        file_length -= 1;
    }
    // Check for bad characters
    for (int i = 0; i < file_length; i++)
    {
        // Get integer representation of each character
        int char_int = text[i];
        // Check for allowed characters (26 alphabets + space)
        // Per ASCII table, A~Z(65~90), space(32)
        if ((char_int < 65 && char_int != 32) || char_int > 90)
        {
            fprintf(stderr, "%s error, input contains bad characters\n", argv[0]);
            exit(1);
        }
    }

    // Open the key file for read only
    FILE* fp_key = fopen(argv[2], "r");
    char* key;
    long key_length;
    if (fp_key == NULL)
    {
        fprintf(stderr, "CLIENT: ERROR opening key file %s\n", argv[2]);
        exit(1);
    }
    else
    {
        // Find file length
        fseek(fp_key, 0, SEEK_END);
        key_length = ftell(fp_key);
        // Move file pointer position to the beginning of the file
        fseek(fp_key, 0, SEEK_SET);
        if (key_length)
        {
            key = malloc(sizeof * key * (key_length + 1));
            fgets(key, sizeof * key * (key_length + 1), fp_key);
        }
        fclose(fp_key);
    }
    // Strip off the new line character
    if (key)
    {
        key[key_length - 1] = '\0';
        key_length -= 1;
    }
    // Check key length
    if (key_length < file_length)
    {
        fprintf(stderr, "ERROR, key '%s' is too short\n", argv[2]);
        exit(1);
    }

    // Set buffer to be 2 time length of input file
    // Add 2(PREFIX) + 1(EOF_CHAR) + 1('\0')
    long output_length = (file_length * 2) + 4;
    char* output_buffer = malloc(output_length);

    // Add ENC_PREFIX, plaintext, key, EOF_CHAR in order
    strcpy(output_buffer, ENC_PREFIX);
    strcat(output_buffer, text);
    // Only add file length of key characters
    strncat(output_buffer, key, file_length);
    strcat(output_buffer, EOF_CHAR);

    int socketFD, charsWritten, charsRead;
    struct sockaddr_in serverAddress;

    // Check usage & args
    if (argc < 4)
    {
        fprintf(stderr, "USAGE: %s plaintext key enc_port\n", argv[0]);
        exit(0);
    }

    // Create a socket
    socketFD = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFD < 0)
    {
        fprintf(stderr, "CLIENT: ERROR opening socket\n");
    }

    // Set up the server address struct
    // Set localhost as the target IP address/host
    setupAddressStruct(&serverAddress, atoi(argv[3]), "localhost");

    // Connect to server
    if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0)
    {
        fprintf(stderr, "CLIENT: ERROR connecting socket\n");
    }

    // Send string to server
    // Write to the server
    charsWritten = send(socketFD, output_buffer, strlen(output_buffer), 0);
    if (charsWritten < 0)
    {
        fprintf(stderr, "CLIENT: ERROR writing to socket\n");
    }
    if (charsWritten < strlen(output_buffer))
    {
        fprintf(stderr, "CLIENT: WARNING: Not all data written to socket!\n");
    }

    // Reset buffer
    // Create a read buffer to read in MAX_SIZE characters
    memset(output_buffer, '\0', output_length);
    char read_buffer[MAX_SIZE];

    // Read the server's response from the socket
    // Until it reads EOF_CHAR
    while (strstr(read_buffer, EOF_CHAR) == NULL)
    {
        memset(read_buffer, '\0', MAX_SIZE);
        // Read from the socket
        charsRead = recv(socketFD, read_buffer, MAX_SIZE - 1, 0);
        if (charsRead < 0)
        {
            fprintf(stderr, "CLIENT: ERROR reading from socket\n");
        }
        // Check for the bad connection signal "0"
        if (strstr(read_buffer, "0"))
        {
            fprintf(stderr, "Error: could not contact enc_server on port %s\n", argv[3]);
            exit(2);
        }
        // Add to the main buffer message
        strcat(output_buffer, read_buffer);
    }

    // Replace EOF_CHAR with a newline char
    output_buffer[strlen(output_buffer) - 1] = '\n';
    // Print output it to stdout
    fprintf(stdout, "%s", output_buffer);

    // Close the socket
    close(socketFD);
    return 0;
}