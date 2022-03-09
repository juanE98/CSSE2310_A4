CC = gcc 
CFLAGS = -pedantic -Wall -std=gnu99 -pthread -I/local/courses/csse2310/include  
DEBUG = -g 
TARGETS = intclient intserver 

LIB = -L/local/courses/csse2310/lib -ltinyexpr -lm -lcsse2310a4 -lcsse2310a3
 
.DEFAULT := all
.PHONY: all debug cleano clean 

debug: CFLAGS += $(DEBUG)
debug: clean $(TARGETS)

all: $(TARGETS) 

intserver: intserver.c shared.o 
	$(CC) $(CFLAGS) $(LIB) shared.o intserver.c -o intserver

intclient: intclient.c shared.o
	$(CC) $(CFLAGS) $(LIB) shared.o intclient.c -o intclient

shared.o: shared.c shared.h 
	$(CC) $(CFLAGS) $(LIB) -c shared.c -o shared.o 

cleano: 
	rm -f *.o 

clean: cleano 
	rm -f $(TARGETS) 
	rm -rf *.dSYM 
