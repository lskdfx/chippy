CC = gcc
CFLAGS = -w -pedantic 
LFLAGS = -lSDL2
OBJS = src/chip8.c
HEADERS = src/chip8.h
OBJ_NAME = chippy

all: $(OBJS)
	$(CC) $(OBJS) $(HEADERS) $(CFLAGS) $(LFLAGS) -o $(OBJ_NAME)

