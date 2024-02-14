#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>

// int create_server_socket(int targetPORT);
// void accept_new_connection(int listener_socket, fd_set *all_sockets, int *fd_max);
// void read_data_from_socket(int socket, fd_set *all_sockets, int fd_max, int server_socket);

void banner(int targetPORT)
{
    printf("\033[1;32m        ┌────────────────┐ \n");
    printf("        │    Welcome     │\n");
    printf("        │  to My Teams   │\n");
    printf("        └────────────────┘\n");
    printf("        ╔════════════════╗\n");
    printf("        ║  Port : %d   ║\n", targetPORT);
    printf("        ╚════════════════╝ \033[0m\n");
    printf("Now that the server is listening, chill with this banger: https://youtu.be/qetz38gTfBg\n");
}

int create_server_socket(int targetPORT)
{
    struct sockaddr_in sa;
    int network_socket;
    int status;

    memset(&sa, 0, sizeof sa);
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = INADDR_ANY;
    sa.sin_port = htons(targetPORT);

    network_socket = socket(sa.sin_family, SOCK_STREAM, 0);
    if (network_socket == -1)
    {
        printf("\033[1;31m$ Error creating socket: %s\n", strerror(errno));
        return (-1);
    }
    printf("$ Starting on port %d\n", targetPORT);

    status = bind(network_socket, (struct sockaddr *)&sa, sizeof sa);
    if (status != 0)
    {
        printf("\033[1;31m$ Error binding server: %s \033[0m \n", strerror(errno));
        return (-1);
    }
    printf("$ Binding OK\n");

    return (network_socket);
}

void accept_new_connection(int server_socket, fd_set *all_sockets, int *fd_max)
{
    int client_fd;
    char msg_to_send[BUFSIZ];
    int status;
    client_fd = accept(server_socket, NULL, NULL);
    if (client_fd == -1)
    {
        printf("\033[1;31m$ Accept error: %s \033[0m \n", strerror(errno));
        return;
    }
    FD_SET(client_fd, all_sockets);
    if (client_fd > *fd_max)
        *fd_max = client_fd;

    printf("$ Accepted new connection on client socket %d.\n", client_fd);
    memset(&msg_to_send, '\0', sizeof msg_to_send);
    sprintf(msg_to_send, "$ Welcome. You are client [%d]\n", client_fd);
    status = send(client_fd, msg_to_send, strlen(msg_to_send), 0);
    if (status == -1)
        printf("\033[1;31m$ Send error to client %d: %s \033[0m \n", client_fd, strerror(errno));
}

void send_to_all_clients(fd_set *all_sockets, int server_socket, int sender, char *msg)
{
    int status;
    for (int i = 0; i <= FD_SETSIZE; i++)
    {
        if (FD_ISSET(i, all_sockets) && i != server_socket && i != sender)
        {
            status = send(i, msg, strlen(msg), 0);
            if (status == -1)
                printf("\033[1;31m$ Send error to client %d: %s\n", i, strerror(errno));
        }
    }
}

void get_ip_address(int socket)
{
    struct sockaddr_in addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);
    int res = getpeername(socket, (struct sockaddr *)&addr, &addr_size);
    char *clientip = inet_ntoa(addr.sin_addr);
    int clientport = ntohs(addr.sin_port);
    printf("Received from => %s:%d\n", clientip, clientport);
}

void read_data_from_socket(int socket, fd_set *all_sockets, int fd_max, int server_socket)
{
    char buffer[BUFSIZ];
    char msg_to_send[BUFSIZ];
    int bytes_read;
    int status;
    memset(&buffer, '\0', sizeof buffer);
    bytes_read = recv(socket, buffer, BUFSIZ, 0);
    if (bytes_read <= 0)
    {
        if (bytes_read == 0)
            printf("[%d] Client socket closed connection.\n", socket);
        else
            printf("[Server] Recv error: %s\n", strerror(errno));
        close(socket);
        FD_CLR(socket, all_sockets);
    }
    else
    {
        printf("[%d] Got message:\n %s\n", socket, buffer);
        get_ip_address(socket);
        memset(&msg_to_send, '\0', sizeof msg_to_send);
        // sprintf(msg_to_send, "Client %d: %s", socket, buffer);
        // snprintf(msg_to_send, sizeof(msg_to_send) + 10, "Client %d: %s", socket, buffer);
        snprintf(msg_to_send, sizeof(msg_to_send) + sizeof(socket) + sizeof(buffer), "Client %d: %s", socket, buffer);
        send_to_all_clients(all_sockets, server_socket, socket, msg_to_send);
    }
}

int main(int argc, char *argv[])
{

    if (argc != 2)
    {
        printf("\033[1;31m$ Please only port for listen new messages.");
        return 1;
    };

    int port;
    port = atoi(argv[1]);

    int server_socket, status, i, fd_max;
    fd_set all_sockets, read_fds;
    struct timeval timer;

    server_socket = create_server_socket(port);
    if (server_socket == -1)
    {
        printf("\033[1;31m$ Error creating server socket\n");
        exit(-1);
    }

    printf("$ Listening on port %d\n", port);
    status = listen(server_socket, 10);
    if (status != 0)
    {
        printf("\033[1;31m$ Listen error: %s\n", strerror(errno));
        exit(-1);
    }

    FD_ZERO(&all_sockets);
    FD_ZERO(&read_fds);
    FD_SET(server_socket, &all_sockets);
    fd_max = server_socket;

    banner(port);

    while (1)
    {
        read_fds = all_sockets;

        timer.tv_sec = 2;
        timer.tv_usec = 0;

        status = select(fd_max + 1, &read_fds, NULL, NULL, &timer);
        if (status == -1)
        {
            printf("\033[1;31m$ Select error: %s\n", strerror(errno));
            exit(-1);
        }
        else if (status == 0)
        {
            printf("$ Waiting...\n");
            continue;
        }

        i = 0;
        while (i <= fd_max)
        {
            if (FD_ISSET(i, &read_fds) != 1)
            {
                i++;
                continue;
            }
            // printf("[%d] Ready for I/O operation\n", i);

            if (i == server_socket)
                accept_new_connection(server_socket, &all_sockets, &fd_max);
            else
                read_data_from_socket(i, &all_sockets, fd_max, server_socket);
            i++;
        }
    }
    return (0);
}