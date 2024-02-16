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

int isIntro = 1;

WINDOW *createWin(int height, int width, int starty, int startx)
{
    WINDOW *local_win;

    local_win = newwin(height, width, starty, startx);
    box(local_win, 0, 0);
    wrefresh(local_win);

    return local_win;
}

void displayIntro(WINDOW *intro_win)
{

    int row, col;
    getmaxyx(intro_win, row, col);
    mvwprintw(intro_win, row / 2 - 2, (col - 30) / 2, "Welcome to MyTeams !");
    mvwprintw(intro_win, row / 2 - 1, (col - 30) / 2, "Type /help to see the list of commands.");
    mvwprintw(intro_win, row / 2, (col - 30) / 2, "Type /quit to exit the chatroom.");
    mvwprintw(intro_win, row / 2 + 1, (col - 30) / 2, "Press any key to continue.");

    wrefresh(intro_win);

    // getch();
}

void displayHelp()
{

    int row, col;
    getmaxyx(stdscr, row, col);
    WINDOW *helpWindow = newwin(5, 50, (row - 5) / 2, (col - 50) / 2);
    box(helpWindow, 0, 0);
    wrefresh(helpWindow);

    mvwprintw(helpWindow, 1, 1, "/help : Display the list of commands");
    mvwprintw(helpWindow, 2, 1, "/quit : Quit the chatroom");
    mvwprintw(helpWindow, 3, 1, "/list : List all the users in the chatroom");
    // Any key to leave
    mvwprintw(helpWindow, 4, 1, "Any key to continue");
    wrefresh(helpWindow);

    wgetch(helpWindow);
    wclear(helpWindow);
    wrefresh(helpWindow);
}

void clearMessage(WINDOW *input_win)
{
    wclear(input_win);
    box(input_win, 0, 0);
    mvwprintw(input_win, 1, 1, "Enter message (/help) : ");
    wrefresh(input_win);
}

char *writeMessage(WINDOW *input_win)
{
    static char message[1000];
    mvwgetstr(input_win, 2, 1, message);
    wrefresh(input_win);
    clearMessage(input_win);
    return message;
}

// void printMessage(WINDOW *chat_win, char *message)
// {
//     mvwprintw(chat_win, messageCount, 1, message);
//     wrefresh(chat_win);
//     messageCount++;
// }

void printMessage(WINDOW *chatWindow, char *message)
{
    int row, col;
    getmaxyx(chatWindow, row, col);

    int lines = 0;
    int message_length = strlen(message);
    int message_index = 0;

    while (message_index < message_length)
    {
        int remaining_space = col - 1;
        while (remaining_space > 0 && message[message_index] != '\0')
        {
            message_index++;
            remaining_space--;
        }
        lines++;
    }

    if (messageCount + lines > row - 1)
    {
        scroll(chatWindow);
        wclear(chatWindow);
        box(chatWindow, 0, 0);
        messageCount = 1;
    }

    mvwprintw(chatWindow, messageCount, 1, "%s", message);
    wrefresh(chatWindow);

    messageCount += lines;
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

void sendMessage(int network_socket, char *message, WINDOW *chatWindow)
{
    if (strcmp(message, "/help") == 0)
    {

        displayHelp();

        box(chatWindow, 0, 0);
        wrefresh(chatWindow);
    }
    else
    {
        if (send(network_socket, message, strlen(message), 0) < 0)
        {
            printf("Send failed\n");
            exit(1);
        }
    }
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
    // cbreak();  // Disable line buffering

    int row, col;
    getmaxyx(stdscr, row, col);

    WINDOW *chatWindow = createWin(row - 5, col, 0, 0);
    WINDOW *inputWindow = createWin(5, col, row - 5, 0);
    WINDOW *usernameWindow = createWin(1, col, row - 6, 0);

    displayIntro(chatWindow);

    // mvwprintw(inputWindow, 1, 1, "Enter message (/help) : ");
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

    // pause();
    wgetch(chatWindow);
    wclear(chatWindow);
    box(chatWindow, 0, 0);
    mvwprintw(inputWindow, 1, 1, "Enter message (/help) : ");
    wrefresh(inputWindow);

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
                printMessage(chatWindow, buffer);
            }
        }

        if (FD_ISSET(STDIN_FILENO, &tmpfds))
        {
            char *message = writeMessage(inputWindow);
            sendMessage(socketToConnect, message, chatWindow);
            wrefresh(inputWindow);
        }
    }

    close(socketToConnect);
    endwin();
    return 0;
}
