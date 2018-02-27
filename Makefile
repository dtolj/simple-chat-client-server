all: chat
chat: chat.o
	cc chat.o -o chat
chat.o: chat.c
	cc -c chat.c
