# MandelCL Makefile
CC=gcc

# Flags!
SDLFLAGS=$(shell sdl-config --cflags)

# Comment this line and uncomment the next to get debug symbols
CFLAGS=-c -Wall -O2 $(SDLFLAGS)
# CFLAGS=-c -Wall -ggdb $(SDLFLAGS)

# Libs!
SDLLIBS=$(shell sdl-config --libs) -lSDL_ttf
OPENCLLIBS=-lOpenCL

LIBS=-lm -lpthread $(SDLLIBS)

# Includes!

INCLUDE=-I/usr/include/SDL -I./

all: mandelclassic clfract test

mandelclassic: mandel_classic.o
	$(CC) $(INCLUDE) mandel_classic.o $(LIBS) -o  mandelclassic

mandelclassic.o: mandel_classic.c
	$(CC) $(CFLAGS) $(INCLUDE) $(LIBS) mandel_classic.c -o mandel_classic.o

clfract: clfract.o
	$(CC) $(INCLUDE) clfract.o $(LIBS) $(OPENCLLIBS) -o clfract

clfract.o: main.c
	$(CC) $(CFLAGS) $(INCLUDE) $(LIBS) $(OPENCLLIBS) main.c -o clfract.o

test: test.o
	$(CC) $(INCLUDE) test.o $(LIBS) -o test

test.o: test.c
	$(CC) $(CFLAGS) $(INCLUDE) $(LIBS) test.c -o test.o

.PHONY: clean

clean:
	@rm *.o mandelclassic test
