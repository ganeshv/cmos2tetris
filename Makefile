TARGETS = hackcpu hackio
CFLAGS = -g -O2 -Wall
INCLUDES = $(shell sdl2-config --cflags)
LIBS = $(shell sdl2-config --libs)

CC = gcc

all: $(TARGETS)

hackcpu: hackmain.c hackcpu.o hackcpu.h
	$(CC) $(CFLAGS) -o hackcpu hackmain.c hackcpu.o

hackcpu.o: hackcpu.c hackcpu.h
	$(CC) $(CFLAGS) -c hackcpu.c

hackio: hackio.c hackcpu.o hackcpu.h
	$(CC) $(CLAGS) $(INCLUDES) -o hackio hackio.c hackcpu.o $(LIBS)

clean:
	rm -f $(TARGETS) hackio hackio.o hackcpu.o hackmain.o
