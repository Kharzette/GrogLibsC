CC=gcc
CFLAGS=-std=gnu23 -g -O0 -march=native -fpic -shared	\
	-DCGLM_FORCE_DEPTH_ZERO_TO_ONE	\
	-DCGLM_FORCE_LEFT_HANDED	\
	-I../cglm/include	\
	-Wall				\
	-Wl,-rpath='libs',--disable-new-dtags	#so exe looks in libs for shared libs
CFLAGS2=-std=gnu23 -g -O0 -march=native -fpic	\
	-DCGLM_FORCE_DEPTH_ZERO_TO_ONE	\
	-DCGLM_FORCE_LEFT_HANDED	\
	-I../cglm/include	\
	-Wall				\
	-Wl,-rpath='libs',--disable-new-dtags	#so exe looks in libs for shared libs
LIBS=-lSDL3 -lFAudio
SOURCES=$(wildcard tinywav/tinywav.c) $(wildcard *.c)

all: AudioLib Audio

AudioLib: $(SOURCES)
	$(CC) $(CFLAGS) $(LDFLAGS) $(SOURCES) -o ../libs/libAudioLib.so $(LIBS) -lm

Audio: $(SOURCES)
	$(CC) $(CFLAGS2) $(LDFLAGS) $(SOURCES) $(LIBS) -lm