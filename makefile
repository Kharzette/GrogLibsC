CC=gcc
CFLAGS=-std=gnu23 -g -O0 -march=native	\
	-DCGLM_FORCE_DEPTH_ZERO_TO_ONE	\
	-DCGLM_FORCE_LEFT_HANDED	\
	-Iuthash/src	\
	-Idxvk-native/include/native/windows	\
	-Idxvk-native/include/native/directx \
	-Icglm/include	\
	-Ijoltc/include	\
	-Wall				\
	-Wl,-rpath='libs',--disable-new-dtags	#so exe looks in libs for shared libs
#	-Xlinker --verbose	
SOURCES=$(wildcard *.c)
LIBS=-lm -lvulkan -lUtilityLib -lMaterialLib -lMeshLib -lTerrainLib -lInputLib -lAudioLib -lFAudio -lSDL2 -lpng
LDFLAGS=-Llibs

all: TestStuff

TestStuff: $(SOURCES)
	$(CC) $(CFLAGS) $(LDFLAGS) $(SOURCES) -o TestStuff $(LIBS)	\
	dxvk-native/build/src/dxgi/libdxvk_dxgi.so	\
	dxvk-native/build/src/d3d11/libdxvk_d3d11.so	\
	joltc/build/lib/libjoltc.so
