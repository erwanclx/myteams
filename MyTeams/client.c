#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <errno.h>
#include <ncurses.h>

struct arguments
{
    char *targetIP;
    int targetPORT;
    char *username;
};

int messageCount = 1;

WINDOW *create_newwin(int height, int width, int starty, int startx)
{
    WINDOW *local_win;

    local_win = newwin(height, width, starty, startx);
    box(local_win, 0, 0);
    wrefresh(local_win);

    return local_win;
}

void destroy_win(WINDOW *local_win)
{
    wborder(local_win, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
    wrefresh(local_win);
    delwin(local_win);
}

char *writeMessage(WINDOW *input_win)
{
    static char message[1000];
    mvwgetstr(input_win, 1, 1, message);
    wrefresh(input_win);
    return message;
}

int createSocket(char *targetIP, int targetPORT)
{
    int network_socket;
    struct sockaddr_in server;

    network_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (network_socket == -1)
    {
        printf("Error creating socket\n");
        return -1;
    }

    printf("[DEBUG] Socket created\n");

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(targetIP);
    server.sin_port = htons(targetPORT);

    if (connect(network_socket, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        printf("Error connecting socket.\n");
        return -1;
    }

    printf("Connected to %s on port %d.\n", targetIP, targetPORT);

    return network_socket;
}

void sendMessage(int network_socket, char *message)
{
    if (send(network_socket, message, strlen(message), 0) < 0)
    {
        printf("Send failed\n");
        exit(1);
    }

    // printf("Data Sent\n");
}

void sendUsername(int network_socket, char *username)
{
    if (send(network_socket, username, strlen(username), 0) < 0)
    {
        printf("Send failed\n");
        exit(1);
    }

    printf("Username sent\n");
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        printf("Please provide IP, port of the server, and your username.\n");
        return 1;
    }

    struct arguments args;
    args.targetIP = argv[1];
    args.targetPORT = atoi(argv[2]);
    args.username = argv[3];

    int socketToConnect;
    socketToConnect = createSocket(args.targetIP, args.targetPORT);

    sendUsername(socketToConnect, args.username);

    initscr(); // Start ncurses
    cbreak();  // Disable line buffering
    // noecho();  // Don't echo while we do getch

    int row, col;
    getmaxyx(stdscr, row, col);

    WINDOW *chatWindow = create_newwin(row - 5, col, 0, 0);
    WINDOW *inputWindow = create_newwin(5, col, row - 5, 0);
    // WINDOW *usernameWindow = create_newwin(1, col, row - 1, 0);
    // On top of the input window centered
    WINDOW *usernameWindow = create_newwin(1, col, row - 6, 0);

    mvwprintw(inputWindow, 1, 1, "Enter message: ");
    wrefresh(inputWindow);

    fd_set readfds;
    struct timeval timeout;
    int status;

    FD_ZERO(&readfds);
    FD_SET(socketToConnect, &readfds);
    FD_SET(STDIN_FILENO, &readfds);

    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    mvwprintw(usernameWindow, 0, 1, "Username: %s", args.username);
    wrefresh(usernameWindow);

    while (1)
    {
        fd_set tmpfds = readfds;
        status = select(socketToConnect + 1, &tmpfds, NULL, NULL, NULL);

        if (status < 0)
        {
            printf("Error in select\n");
            break;
        }
        else if (status == 0)
        {
            continue;
        }

        if (FD_ISSET(socketToConnect, &tmpfds))
        {
            char buffer[BUFSIZ];
            int bytes_read = recv(socketToConnect, buffer, BUFSIZ, 0);
            if (bytes_read <= 0)
            {
                if (bytes_read == 0)
                    printf("Server closed connection.\n");
                else
                    printf("Recv error: %s\n", strerror(errno));
                close(socketToConnect);
                exit(1);
            }
            else
            {
                buffer[bytes_read] = '\0';
                // Print received message in chat window
                mvwprintw(chatWindow, messageCount, 1, "%s", buffer);
                wrefresh(chatWindow);
                messageCount += 1;
            }
        }

        if (FD_ISSET(STDIN_FILENO, &tmpfds))
        {
            char *message = writeMessage(inputWindow);
            sendMessage(socketToConnect, message);
            wrefresh(inputWindow);
        }
    }

    close(socketToConnect);
    endwin();
    return 0;
}
