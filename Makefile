all: chat
chat: chat.o
	cc chat.o -ochat
chat.o: chat.c
	cc -c chat.c
