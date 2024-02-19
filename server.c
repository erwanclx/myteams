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
#include <curl/curl.h>
#define USERNAME_SIZE 20
#define MAX_CLIENTS 50

typedef struct
{
    int socket;
    char username[USERNAME_SIZE];
    int messages_sent;
    int is_admin;

} ClientInfo;

ClientInfo clients[MAX_CLIENTS];

char *get_username(int client_socket, char *username);

char *find_string(char *buffer, char *key, int size)
{
    char *start = strstr(buffer, key);
    start += strlen(key);
    char *end = start;
    while (*end != '\0' && *end != ',' && *end != '}')
    {
        end++;
    }
    char *result = (char *)malloc((end - start + 1) * sizeof(char));
    if (result != NULL)
    {
        strncpy(result, start, end - start);
        result[end - start] = '\0';
    }
    return result;
}

int is_client_registered(int client_socket)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].socket == client_socket)
        {
            return 1;
        }
    }
    return 0;
}

char write_data(void *buffer, size_t size, size_t nmemb, void *userp)
{

    char *admin_str = find_string(buffer, "\"Admin\":", 6);
    int is_admin = strcmp(admin_str, "true") == 0 ? 1 : 0;

    char *username = find_string(buffer, "\"Username\":\"", USERNAME_SIZE);
    username[strlen(username) - 1] = '\0';

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (strcmp(clients[i].username, username) == 0)
        {
            clients[i].is_admin = is_admin;
            printf("[DEBUG] Client %d is admin: %d\n", clients[i].socket, clients[i].is_admin);
            break;
        }
    }

    return size * nmemb;
}

int is_admin(int sender)
{
    CURL *curl;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_ALL);

    char *tmp = "";
    char *username = get_username(sender, tmp);

    curl = curl_easy_init();
    if (curl)
    {
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Authorization: Token q24qiZP0hexG2wlzkx3hl9THnS6MF6RD");

        const char *basis_url = "https://api.baserow.io/api/database/rows/table/256115/?user_field_names=true&filters=";
        char json_data[200];

        snprintf(json_data, sizeof(json_data), "{\"filter_type\":\"AND\",\"filters\":[{\"type\":\"equal\",\"field\":\"Username\",\"value\":\"%s\"}],\"groups\":[]}", username);
        char *url = malloc(strlen(basis_url) + strlen(json_data) + 1);
        strcpy(url, basis_url);
        strcat(url, json_data);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(res));
            printf("\033[1;31m");
            printf("$ Authentification failed, error in the request. Insure you only give valid characters.\n");
            printf("\033[0m");
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
}

void register_client(int client_socket, char *username)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].socket == 0)
        {
            clients[i].socket = client_socket;
            strcpy(clients[i].username, username);
            clients[i].messages_sent = 1;
            printf("[DEBUG] Client %d registered as %s with %d messages\n", clients[i].messages_sent, clients[i].username, clients[i].messages_sent);
            printf("[DEBUG] Client %d is admin: %d\n", client_socket, clients[i].is_admin);
            is_admin(client_socket);
            break;
        }
    }
}

void unregister_client(int client_socket)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].socket == client_socket)
        {
            clients[i].socket = 0;
            memset(clients[i].username, 0, sizeof(clients[i].username));
            clients[i].messages_sent = 0;
            clients[i].is_admin = 0;
            break;
        }
    }
}

char *get_username(int client_socket, char *username)
{
    // clients[client_socket].messages_sent++;
    // printf("[DEBUG] Client %d has %d messages, his username is \n", client_socket, clients[client_socket].messages_sent, clients[client_socket].username);
    // return clients[client_socket].username;
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].socket == client_socket)
        {
            return clients[i].username;
        }
    }
    return "Unknown";
}

int get_client_index(int client_socket)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].socket == client_socket)
        {
            return i;
        }
    }
    return -1;
}

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

    printf("DEBUG ---- %s\n", msg);

    if (strcmp(msg, "/list") == 0)
    {
        char user_list[BUFSIZ];
        memset(user_list, 0, BUFSIZ);

        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if (clients[i].socket != 0)
            {
                strcat(user_list, clients[i].username);
                strcat(user_list, " | ");
            }
        }

        char final_list[BUFSIZ + 8];
        snprintf(final_list, sizeof(final_list), "$ %s\n", user_list);
        status = send(sender, final_list, strlen(final_list), 0);
    }
    else if (strcmp(msg, "/kick") == 0)
    {
        char kick_msg[50] = "$ Please provide a username to kick.";
        status = send(sender, kick_msg, strlen(kick_msg), 0);
    }
    // If kick + username
    else if (strncmp(msg, "/kick", 5) == 0)
    {

        // is_admin(sender);

        int sender_index = get_client_index(sender);
        int admin_status = clients[sender_index].is_admin;

        char *username = msg + 6;
        int found = 0;

        if (admin_status == 1)
        {
            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                if (strcmp(clients[i].username, username) == 0)
                {
                    send(clients[i].socket, "$ You have been kicked.", 22, 0);
                    send(clients[i].socket, "\n", 22, 0);
                    close(clients[i].socket);
                    FD_CLR(clients[i].socket, all_sockets);
                    unregister_client(clients[i].socket);
                    found = 1;
                    status = send(sender, "$ User kicked.", 14, 0);
                    break;
                }
            }
            if (found == 0)
            {
                char kick_msg[50] = "$ User not found.";
                status = send(sender, kick_msg, strlen(kick_msg), 0);
            }
        }
        else
        {
            char kick_msg[50] = "$ You are not an admin.";
            status = send(sender, kick_msg, strlen(kick_msg), 0);
        }
    }
    else
    {

        char *username = get_username(sender, msg);

        for (int i = 0; i <= FD_SETSIZE; i++)
        {
            if (FD_ISSET(i, all_sockets) && i != server_socket && i != sender)
            {
                char final_msg[BUFSIZ + 8];
                sprintf(final_msg, "# %s >> %s", username, msg);
                status = send(i, final_msg, strlen(final_msg), 0);
                if (status == -1)
                    printf("\033[1;31m$ Send error to client %d: %s\n", i, strerror(errno));
            }

            else if (i == sender)
            {
                char sender_msg[BUFSIZ + 8];
                // sprintf(sender_msg, "(You) %s", msg);
                sprintf(sender_msg, "# (You) %s >> %s", username, msg);
                status = send(i, sender_msg, strlen(sender_msg), 0);
                if (status == -1)
                    printf("\033[1;31m$ Send error to client %d: %s\n", i, strerror(errno));
            }
        }
    }
}

void get_ip_address(int socket)
{
    struct sockaddr_in addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);
    getpeername(socket, (struct sockaddr *)&addr, &addr_size);
    char *clientip = inet_ntoa(addr.sin_addr);
    int clientport = ntohs(addr.sin_port);
    printf("Received from => %s:%d\n", clientip, clientport);
}

void read_data_from_socket(int socket, fd_set *all_sockets, int fd_max, int server_socket)
{
    char buffer[BUFSIZ];
    // char msg_to_send[BUFSIZ];
    int bytes_read;
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
        unregister_client(socket);
    }
    else
    {
        printf("[%d] Got message:\n %s\n", socket, buffer);

        if (!is_client_registered(socket))
        {
            register_client(socket, buffer);
            return;
        }

        get_ip_address(socket);
        // memset(&msg_to_send, '\0', sizeof msg_to_send);

        // char *username = get_username(socket, buffer);

        // snprintf(msg_to_send, sizeof(msg_to_send) + sizeof(socket) + sizeof(username) + sizeof(buffer), "# %s >> %s", username, buffer);
        // send_to_all_clients(all_sockets, server_socket, socket, msg_to_send);
        send_to_all_clients(all_sockets, server_socket, socket, buffer);
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
            // Print all clients
            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                if (clients[i].socket != 0)
                {
                    printf("Client %d: %s\n", clients[i].socket, clients[i].username);
                }
            }
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

            if (i == server_socket)
                accept_new_connection(server_socket, &all_sockets, &fd_max);
            else
                read_data_from_socket(i, &all_sockets, fd_max, server_socket);
            i++;
        }
    }
    return (0);
}