CC = gcc
all: server client clean

server: server.o
	${CC} server.o -o server

server.o: server.c
	${CC} server.c -c

client: client.o
	${CC} client.o -o client

client.o: client.c
	${CC} client.c -c

clean:
	@rm -rf *.o

