CC=gcc
#CFLAGS=-std=c17 -g -I../vcpkg/packages/xaudio2redist_x64-mingw-static/include
CFLAGS=-std=c17 -g -march=x86-64 #-Xlinker --verbose
#LDFLAGS=-L../vcpkg/packages/xaudio2redist_x64-mingw-static/lib
#LIBS=-lxaudio2_9redist -lole32
LIBS=-lFAudio
SOURCES=$(wildcard tinywav/tinywav.c) $(wildcard *.c)

all: Timey

Timey: $(SOURCES)
	$(CC) $(CFLAGS) $(LDFLAGS) $(SOURCES) -o AudioLib $(LIBS)