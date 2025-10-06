CC=gcc
CFLAGS=-std=gnu23 -g -O0 -march=native	\
	-DCGLM_FORCE_DEPTH_ZERO_TO_ONE	\
	-DCGLM_FORCE_LEFT_HANDED	\
	-I../			\
	-I../uthash/src	\
	-I../dxvk-native/include/native/windows	\
	-I../dxvk-native/include/native/directx \
	-I../cglm/include	\
	-Wall				\
	-Wl,-rpath='libs',--disable-new-dtags	#so exe looks in . for shared libs
#	-Xlinker --verbose	

SRCSTUFF = TestStuff.c
SRCUI = TestUI.c
OBJSTUFF = $(SRCSTUFF:.c=.o)
OBJUI = $(SRCUI:.c=.o)

LIBS=-lm -lvulkan -lUtilityLib -lPhysicsLib -lMaterialLib -lUILib -lMeshLib -lTerrainLib -lInputLib -lAudioLib -lFAudio -lSDL3 -lpng
LDFLAGS=-L../libs

all: TestStuff TestUI

TestStuff: $(OBJSTUFF)
	$(CC) $(CFLAGS) $(OBJSTUFF) $(LDFLAGS) -o TestStuff $(LIBS)	\
	../dxvk-native/build/src/dxgi/libdxvk_dxgi.so	\
	../dxvk-native/build/src/d3d11/libdxvk_d3d11.so

TestUI: $(OBJUI)
	$(CC) $(CFLAGS) $(OBJUI) $(LDFLAGS) -o TestUI $(LIBS)	\
	../dxvk-native/build/src/dxgi/libdxvk_dxgi.so	\
	../dxvk-native/build/src/d3d11/libdxvk_d3d11.so

.PHONY: all clean

clean:
	rm TestStuff TestUI *.o