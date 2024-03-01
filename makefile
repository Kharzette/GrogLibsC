CC=gcc
CFLAGS=-std=gnu2x -g -O0 -march=x86-64 -msse2 -mf16c	\
	-ISDL/include	\
	-Iuthash/include	\
	-Idxvk-native/include/native/windows	\
	-Idxvk-native/include/native/directx \
	-Icglm/include	\
	-Wl,-rpath='libs',--disable-new-dtags	#so exe looks in libs for shared libs
#	-Xlinker --verbose	
SOURCES=$(wildcard *.c)
LIBS=-lvulkan -lUtilityLib -lMaterialLib -lMeshLib -lTerrainLib# -ldxvk_d3d11 -ldxvk_dxgi
LDFLAGS=-LUtilityLib -LMaterialLib -LMeshLib -LTerrainLib# -Ldxvk-native/build/dxvk-native-master/lib/x86_64-linux-gnu -LSDL/build

all: TestStuff

TestStuff: $(SOURCES)
	$(CC) $(CFLAGS) $(LDFLAGS) $(SOURCES) -o TestStuff $(LIBS)	\
	SDL/build/libSDL3.so -lm	\
	dxvk-native/build/dxvk-native-master/lib/x86_64-linux-gnu/libdxvk_dxgi.so	\
	dxvk-native/build/dxvk-native-master/lib/x86_64-linux-gnu/libdxvk_d3d11.so	\
	cglm/build/libcglm.a