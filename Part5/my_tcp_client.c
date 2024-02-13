#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h> // Type for IP addresses
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

struct arguments
{
    char *targetIP;
    int targetPORT;
    char *message;
};

// struct sockaddr_in
// {
//     short sin_family;
//     unsigned short sin_port;
//     struct in_addr sin_addr;
// };

// struct in_addr
// {
//     unsigned long s_addr;
// };

// struct sockaddr
// {
//     unsigned short sa_family;
//     char sa_data[14];
// };

int createSocket(char *targetIP, int targetPORT)
{
    int network_socket;
    struct sockaddr_in server;

    network_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (network_socket == -1)
    {
        printf("Error creating socket");
        return 1;
    };

    printf("[DEBUG] Socket created \n");

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(targetIP);
    server.sin_port = htons(targetPORT);

    if (connect(network_socket, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        printf("Error connecting socket. \n");
        exit(1);
    }

    printf("Connected to %s on port %d. \n", targetIP, targetPORT);

    return (network_socket);
};

void sendMessage(int network_socket, char *message)
{
    if (send(network_socket, message, strlen(message), 0) < 0)
    {
        printf("Send failed\n");
        exit(1);
    }

    printf("Data Send\n");
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        printf("Please only provide IP, port and message to send.");
        return 1;
    };

    struct arguments args;
    args.targetIP = argv[1];
    args.targetPORT = atoi(argv[2]);
    args.message = argv[3];

    int socketToConnect;
    socketToConnect = createSocket(args.targetIP, args.targetPORT);

    sendMessage(socketToConnect, args.message);

    // pause();
};