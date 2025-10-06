CC=g++
CFLAGS=-std=gnu++23 -g -O0 -fPIC -shared -march=native 	\
	-DCGLM_FORCE_DEPTH_ZERO_TO_ONE	\
	-DCGLM_FORCE_LEFT_HANDED	\
	-DJPH_PROFILE_ENABLED	\
	-DJPH_DEBUG_RENDERER	\
	-DJPH_OBJECT_STREAM	\
	-I../JoltPhysics \
	-I../uthash/src		\
	-I../cglm/include	\
	-Wall				\
	-Wl,-rpath='$$ORIGIN',--disable-new-dtags	#so exe looks in . for shared libs

#my only cpp file
SOURCES=$(wildcard *.cpp)
OBJS=$(SOURCES:.c=.o)

all: PhysicsLib

PhysicsLib: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o ../libs/libPhysicsLib.so \
	../JoltPhysics/Build/Linux_Debug/libJolt.a

.PHONY: all clean

clean:
	rm *.o ../libs/libPhysicsLib.so