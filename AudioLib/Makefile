CC=gcc
#CFLAGS=-std=c17 -g -I../vcpkg/packages/xaudio2redist_x64-mingw-static/include
CFLAGS=-std=gnu2x -g -O0 -march=native -fpic -shared	\
	-DCGLM_FORCE_DEPTH_ZERO_TO_ONE	\
	-DCGLM_FORCE_LEFT_HANDED	\
	-I../cglm/include	\
	-Wall				\
	-Wl,-rpath='libs',--disable-new-dtags	#so exe looks in libs for shared libs
#	-IFAudio/include	\
#LDFLAGS=-L../vcpkg/packages/xaudio2redist_x64-mingw-static/lib
#LDFLAGS=-LFAudio/build/
#LDFLAGS=-L../SDL/build
#LDFLAGS=-lm
#LIBS=-lxaudio2_9redist -lole32
LIBS=-lSDL2 -lFAudio
SOURCES=$(wildcard tinywav/tinywav.c) $(wildcard *.c)

all: AudioLib

AudioLib: $(SOURCES)
	$(CC) $(CFLAGS) $(LDFLAGS) $(SOURCES) -o ../libs/libAudioLib.so $(LIBS) -lm