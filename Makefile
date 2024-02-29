OBJS	= server.o client.o
SOURCE	= server.c client.c
HEADER	= 
OUT	= server client
CC	 = gcc
FLAGS	 = -g -c
# -Wall: all warnings
LFLAGS	 = -lncurses -lcurl

# all: $(OBJS)
# 	$(CC) -g $(OBJS) -o $(OUT) $(LFLAGS)

all: $(OBJS)
	$(CC) -g server.o -o server $(LFLAGS)
	$(CC) -g client.o -o client $(LFLAGS)

server.o: server.c
	$(CC) $(FLAGS) server.c 

client.o: client.c
	$(CC) $(FLAGS) client.c 


clean:
	rm -f $(OBJS) $(OUT)

run: $(OUT)
	./$(OUT)
