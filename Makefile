# Makefile for smash.c 

EXE = smash
OBJS = command.o history.o

CC = gcc
CFLAGS = -Wall -std=c99

%.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $<

all: $(EXE)

smashLib.a : $(OBJS)
	ar r $@ $?

$(EXE): smash.o smashLib.a
	$(CC) -o $@ $^

debug: CFLAGS += -g -O0
debug: $(EXE)

clean:
	rm -f *.o *.a $(EXE) *~


#all: rules.d $(EXE)

#rules.d: Makefile $(wildcard *.c) $(wildcard *.h)
#	gcc -MM $(wildcard *.c) >rules.d
#
#-include rules.d
#
#$(EXE): $(OBJS)
#	$(CC) $(CFLAGS) $^ -o $@
#
#debug: CFLAGS += -g -O0
#debug: all
#
#clean:
#	rm -f $(OBJS) $(EXE) *.d
