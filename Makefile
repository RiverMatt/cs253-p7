# Makefile for smash.c 

EXE = smash
OBJS = smash.o command.o history.o

CC = gcc
CFLAGS = -Wall -std=c99

all: rules.d $(EXE)

rules.d: Makefile $(wildcard *.c) $(wildcard *.h)
	gcc -MM $(wildcard *.c) >rules.d

-include rules.d

$(EXE): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

debug: CFLAGS += -g -O0
debug: all

clean:
	rm -f $(OBJS) $(EXE) *.d
