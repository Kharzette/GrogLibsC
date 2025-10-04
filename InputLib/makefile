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
LDFLAGS=#-LUtilityLib -LMaterialLib -LMeshLib -LTerrainLib -Ldxvk-native/build/dxvk-native-master/lib/x86_64-linux-gnu -LSDL/build

all: InputLib

InputLib:  $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o ../libs/libInputLib.so $(LIBS)
