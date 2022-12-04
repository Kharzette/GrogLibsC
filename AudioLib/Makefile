CC=gcc
#CFLAGS=-std=c17 -g -I../vcpkg/packages/xaudio2redist_x64-mingw-static/include
CFLAGS=-std=c17 -g -march=x86-64 -IFAudio/include#-Xlinker --verbose
#LDFLAGS=-L../vcpkg/packages/xaudio2redist_x64-mingw-static/lib
#LDFLAGS=-LFAudio/build/
#LDFLAGS=-L../SDL/build
#LDFLAGS=-lm
#LIBS=-lxaudio2_9redist -lole32
#LIBS=-lSDL3
SOURCES=$(wildcard tinywav/tinywav.c) $(wildcard *.c)

all: Timey

Timey: $(SOURCES)
	$(CC) $(CFLAGS) $(LDFLAGS) $(SOURCES) -o AudioLib $(LIBS) FAudio/build/libFAudio.a ../SDL/build/libSDL3.a -lm