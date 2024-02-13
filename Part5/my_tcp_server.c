#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h> // Type for IP addresses
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>

char resolveHost(char *host)
{
    char ip[16];
    struct hostent *he;
    struct in_addr **addr_list;

    if ((he = gethostbyname(host)) == NULL)
    {
        printf("Error during resolving host\n");
        return 1;
    }

    addr_list = (struct in_addr **)he->h_addr_list;

    strcpy(ip, inet_ntoa(*addr_list[0]));

    printf("%s resolved to : %s\n", host, ip);

    return 0;
}

int createServerSocket(int targetPORT)
{
    int network_socket, new_socket, c;
    struct sockaddr_in server, client;

    network_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (network_socket == -1)
    {
        printf("Error creating socket");
        return 1;
    };

    printf("Starting on port %d\n", targetPORT);

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(targetPORT);

    if (bind(network_socket, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        printf("Error binding server\n");
    }
    printf("Binding OK\n");

    listen(network_socket, 3);

    printf("Waiting message from client\n");

    while (1)
    {
        c = sizeof(struct sockaddr_in);
        new_socket = accept(network_socket, (struct sockaddr *)&client, (socklen_t *)&c);

        if (new_socket < 0)
        {
            printf("Error during connection\n");
        }

        char *client_ip = inet_ntoa(client.sin_addr);
        int client_port = ntohs(client.sin_port);

        char client_message[2000];

        printf("Receveid from : %s:%d =>\n", client_ip, client_port);

        if (recv(new_socket, client_message, 2000, 0) < 0)
        {
            printf("Error during message receveid\n");
        }

        printf(">> %s\n", client_message);

        close(new_socket);
    }

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

    createServerSocket(port);

    // printf("Starting on port %d.\n", port);
}