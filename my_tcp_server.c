#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h> // Type for IP addresses
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int createServerSocket(int targetPORT)
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
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(targetPORT);

    if (bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        printf("Error binding server\n");
    }
    printf("[DEBUG] Binding done\n");

    listen(socket_desc, 3);

    printf("Waiting for incoming connections..\n");

    c = sizeof(struct sockaddr_in);
    new_socket = accept(socket_desc, (struct sockaddr *)&client, (socklen_t *)&c);

    if (new_socket < 0)
    {
        printf("Error during connection\n");
    }

    printf("Connection established\n");

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Please only port for listen new messages.");
        return 1;
    };

    int port;
    port = atoi(argv[1]);

    // printf("Starting on port %d.\n", port);
}