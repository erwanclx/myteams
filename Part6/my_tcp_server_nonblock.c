// #include <sys/types.h>
// #include <sys/socket.h>
// #include <arpa/inet.h> // Type for IP addresses
// #include <stdio.h>
// #include <stdlib.h>
// #include <unistd.h>
// #include <string.h>
// #include <netdb.h>
// #include <fcntl.h>

// void banner(int targetPORT)
// {
//     printf("\033[1;32m        ┌────────────────┐ \n");
//     printf("        │    Welcome     │\n");
//     printf("        │  to My Teams   │\n");
//     printf("        └────────────────┘\n");
//     printf("        ╔════════════════╗\n");
//     printf("        ║  Port : %d   ║\n", targetPORT);
//     printf("        ╚════════════════╝ \033[0m\n");
//     printf("Now that the server is listening, chill with this banger: https://youtu.be/qetz38gTfBg\n");
// }

// char resolveHost(char *host)
// {
//     char ip[16];
//     struct hostent *he;
//     struct in_addr **addr_list;

//     if ((he = gethostbyname(host)) == NULL)
//     {
//         printf("\033[1;31m$ Error during resolving host\033[0m\n");
//         return 1;
//     }

//     addr_list = (struct in_addr **)he->h_addr_list;

//     strcpy(ip, inet_ntoa(*addr_list[0]));

//     printf("%s resolved to : %s\n", host, ip);

//     return 0;
// }

// int createServerSocket(int targetPORT)
// {
//     int network_socket, new_socket, c;
//     struct sockaddr_in server, client;

//     network_socket = socket(AF_INET, SOCK_STREAM, 0);
//     // fcntl(network_socket, F_SETFL, O_NONBLOCK);

//     if (network_socket == -1)
//     {
//         printf("\033[1;31m$ Error creating socket\033[0m\n");
//         return 1;
//     };

//     printf("$ Starting on port %d\n", targetPORT);

//     server.sin_family = AF_INET;
//     server.sin_addr.s_addr = INADDR_ANY;
//     server.sin_port = htons(targetPORT);

//     if (bind(network_socket, (struct sockaddr *)&server, sizeof(server)) < 0)
//     {
//         printf("\033[1;31m$ Error binding server\033[0m\n");
//     }

//     printf("$ Binding OK\n");

//     listen(network_socket, 3);
//     banner(targetPORT);

//     while (1)
//     {
//         c = sizeof(struct sockaddr_in);
//         new_socket = accept(network_socket, (struct sockaddr *)&client, (socklen_t *)&c);

//         if (new_socket < 0)
//         {
//             printf("$ Error during connection\n");
//         }

//         char *client_ip = inet_ntoa(client.sin_addr);
//         int client_port = ntohs(client.sin_port);

//         char client_message[2000];

//         printf("Receveid from : %s:%d =>\n", client_ip, client_port);

//         if (recv(new_socket, client_message, 2000, 0) < 0)
//         {
//             printf("\033[1;31m$ Error during message reception\033[0m\n");
//         }

//         printf(">> %s\n", client_message);

//         close(new_socket);
//     }

//     return 0;
// }

// int main(int argc, char *argv[])
// {
//     if (argc != 2)
//     {
//         printf("Please only port for listen new messages.");
//         return 1;
//     };

//     int port;
//     port = atoi(argv[1]);

//     createServerSocket(port);
// }

#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#define PORT 4242 // le port de notre serveur
int create_server_socket(void);
void accept_new_connection(int listener_socket, fd_set *all_sockets, int *fd_max);
void read_data_from_socket(int socket, fd_set *all_sockets, int fd_max, int server_socket);
int main(void)
{
    int server_socket;
    int status;
    int i;              // Pour notre bloucle de vérification des sockets
    fd_set all_sockets; // Ensemble de toutes les sockets du serveur
    fd_set read_fds;    // Ensemble temporaire pour select()
    int fd_max;         // Descripteur de la plus grande socket
    struct timeval timer;
    // Création de la socket du serveur
    server_socket = create_server_socket();
    if (server_socket == -1)
        exit(-1);
    // Écoute du port via la socket
    printf("[Server] Listening on port %d\n", PORT);
    status = listen(server_socket, 10);
    if (status != 0)
    {
        printf("[Server] Listen error: %s\n", strerror(errno));
        exit(-1);
    }
    // Préparation des ensembles de sockets pour select()
    FD_ZERO(&all_sockets);
    FD_ZERO(&read_fds);
    FD_SET(server_socket, &all_sockets);
    fd_max = server_socket;
    printf("[Server] Set up select fd sets\n");
    while (1) // Boucle principale
    {
        // Copie l'ensemble des sockets puisque select() modifie l'ensemble surveillé
        read_fds = all_sockets;
        // Timeout de 2 secondes pour select()
        timer.tv_sec = 2;
        timer.tv_usec = 0;
        // Surveille les sockets prêtes à être lues
        status = select(fd_max + 1, &read_fds, NULL, NULL, &timer);
        if (status == -1)
        {
            printf("[Server] Select error: %s\n", strerror(errno));
            exit(-1);
        }
        else if (status == 0)
        {
            // Aucun descipteur de fichier de socket n'est prêt pour la lecture
            printf("[Server] Waiting...\n");
            continue;
        }
        // Boucle sur nos sockets
        i = 0;
        while (i <= fd_max)
        {
            if (FD_ISSET(i, &read_fds) != 1)
            {
                i++;
                continue;
            }
            printf("[%d] Ready for I/O operation\n", i);

            if (i == server_socket)
                // La socket est notre socket serveur qui écoute le port
                accept_new_connection(server_socket, &all_sockets, &fd_max);
            else
                // La socket est une socket client, on va la lire
                read_data_from_socket(i, &all_sockets, fd_max, server_socket);
            i++;
        }
    }
    return (0);
}
int create_server_socket(void)
{
    struct sockaddr_in sa;
    int socket_fd;
    int status;
    // Préparaton de l'adresse et du port pour la socket de notre serveur
    memset(&sa, 0, sizeof sa);
    sa.sin_family = AF_INET;                     // IPv4
    sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // 127.0.0.1, localhost
    sa.sin_port = htons(PORT);
    // Création de la socket
    socket_fd = socket(sa.sin_family, SOCK_STREAM, 0);
    if (socket_fd == -1)
    {
        printf("[Server] Socket error: %s\n", strerror(errno));
        return (-1);
    }
    printf("[Server] Created server socket fd: %d\n", socket_fd);
    // Liaison de la socket à l'adresse et au port
    status = bind(socket_fd, (struct sockaddr *)&sa, sizeof sa);
    if (status != 0)
    {
        printf("[Server] Bind error: %s\n", strerror(errno));
        return (-1);
    }
    printf("[Server] Bound socket to localhost port %d\n", PORT);
    return (socket_fd);
}
// Accepte une nouvelle connexion et ajoute la nouvelle socket à l'ensemble des sockets
void accept_new_connection(int server_socket, fd_set *all_sockets, int *fd_max)
{
    int client_fd;
    char msg_to_send[BUFSIZ];
    int status;
    client_fd = accept(server_socket, NULL, NULL);
    if (client_fd == -1)
    {
        printf("[Server] Accept error: %s\n", strerror(errno));
        return;
    }
    FD_SET(client_fd, all_sockets); // Ajoute la socket client à l'ensemble
    if (client_fd > *fd_max)
        *fd_max = client_fd; // Met à jour la plus grande socket
    printf("[Server] Accepted new connection on client socket %d.\n", client_fd);
    memset(&msg_to_send, '\0', sizeof msg_to_send);
    sprintf(msg_to_send, "Welcome. You are client fd [%d]\n", client_fd);
    status = send(client_fd, msg_to_send, strlen(msg_to_send), 0);
    if (status == -1)
        printf("[Server] Send error to client %d: %s\n", client_fd, strerror(errno));
}
// Lit le message d'une socket et relaie le message à toutes les autres
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
        close(socket);               // Ferme la socket
        FD_CLR(socket, all_sockets); // Enlève la socket de l'ensemble
    }
    else
        printf("[%d] Got message: %s", socket, buffer);
}