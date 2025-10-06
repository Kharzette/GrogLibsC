CC=gcc
CFLAGS=-std=gnu23 -g -O0 -march=native -fpic -shared	\
	-DCGLM_FORCE_DEPTH_ZERO_TO_ONE	\
	-DCGLM_FORCE_LEFT_HANDED	\
	-I../uthash/src		\
	-I../cglm/include	\
	-Wall				\
	-Wl,-rpath='libs',--disable-new-dtags	#so exe looks in libs for shared libs
#	-Xlinker --verbose	

SOURCES=$(wildcard *.c)
OBJS=$(SOURCES:.c=.o)
LIBS=-lSDL3 -lm

all: InputLib

InputLib:  $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o ../libs/libInputLib.so $(LIBS)

.PHONY: all clean

clean:
	rm *.o ../libs/libInputLib.so