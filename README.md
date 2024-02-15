# MyTeams - Dev

[![MyTeams Make CI](https://github.com/erwanclx/myteams/actions/workflows/action.yml/badge.svg?branch=erwan)](https://github.com/erwanclx/myteams/actions/workflows/action.yml)

## Description
MyTeams is a simple chat application that allows users to exchange messages in real time in C.
GUI application using the ncurses library.

## How to use
- First, you need to compile the server and the client.
- Then, you can run the server and the client.

### How to compile
Before compiling the server and the client, you need to install the ncurses library:
```bash
sudo apt-get install libncurses5-dev libncursesw5-dev
```

To compile all the files, you can use the Makefile:
```bash
make
```
### How to run
To run the server, you can use the following command:
```bash
./server 4242
```
**Help:** ./server PORT

To run the client, you can use the following command:
```bash
./client 127.0.0.1 4242 Erwan
```
**Help:** ./client IP PORT USERNAME
