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

SOURCES=$(wildcard tinywav/tinywav.c) $(wildcard *.c)
OBJS = $(SOURCES:.c=.o)

LIBS=-lSDL3 -lFAudio -lm

all: AudioLib Audio

AudioLib: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o ../libs/libAudioLib.so $(LIBS)

Audio: $(OBJS)
	$(CC) $(CFLAGS2) $(OBJS) $(LDFLAGS) $(LIBS)

.PHONY: all clean

clean:
	rm *.o a.out ../libs/libAudioLib.so