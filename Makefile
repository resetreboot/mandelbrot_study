# MandelCL Makefile
CC=gcc

# Flags!
SDLFLAGS=$(shell sdl2-config --cflags)

# Comment this line and uncomment the next to get debug symbols
CFLAGS=-c -Wall -O2 $(SDLFLAGS)
# CFLAGS=-c -Wall -ggdb $(SDLFLAGS)

# Libs!
SDLLIBS=$(shell sdl2-config --libs) -lSDL2_ttf
OPENCLLIBS=-lOpenCL

LIBS=-lm -lpthread $(SDLLIBS)

# Includes!

INCLUDE=-I/usr/include/SDL2 -I./

all: mandelclassic clfract test clfractinteractive

mandelclassic: mandel_classic.o
	$(CC) $(INCLUDE) mandel_classic.o $(LIBS) -o  mandelclassic

mandelclassic.o: mandel_classic.c
	$(CC) $(CFLAGS) $(INCLUDE) $(LIBS) mandel_classic.c -o mandel_classic.o

clfract: clfract.o
	$(CC) $(INCLUDE) clfract.o $(LIBS) $(OPENCLLIBS) -o clfract

clfract.o: main.c
	$(CC) $(CFLAGS) $(INCLUDE) $(LIBS) $(OPENCLLIBS) main.c -o clfract.o

clfractinteractive: clfractinteractive.o
	$(CC) $(INCLUDE) clfractinteractive.o $(LIBS) $(OPENCLLIBS) -o clfractinteractive

clfractinteractive.o: interactive.c
	$(CC) $(CFLAGS) $(INCLUDE) $(LIBS) $(OPENCLLIBS) interactive.c -o clfractinteractive.o

test: test.o
	$(CC) $(INCLUDE) test.o $(LIBS) -o test

test.o: test.c
	$(CC) $(CFLAGS) $(INCLUDE) $(LIBS) test.c -o test.o

.PHONY: clean

clean:
	@rm *.o mandelclassic test
