CC=gcc
CFLAGS=-std=gnu2x -g -O0 -march=x86-64 -msse4.2 -mavx2 -mf16c	\
	-DCGLM_FORCE_DEPTH_ZERO_TO_ONE	\
	-DCGLM_FORCE_LEFT_HANDED	\
	-ISDL/include	\
	-Iuthash/src	\
	-Idxvk-native/include/native/windows	\
	-Idxvk-native/include/native/directx \
	-Icglm/include	\
	-Wall				\
	-Wl,-rpath='libs',--disable-new-dtags	#so exe looks in libs for shared libs
#	-Xlinker --verbose	
SOURCES=$(wildcard *.c)
LIBS=-lvulkan -lUtilityLib -lMaterialLib -lMeshLib -lTerrainLib -lInputLib# -ldxvk_d3d11 -ldxvk_dxgi
LDFLAGS=-LUtilityLib -LMaterialLib -LMeshLib -LTerrainLib -LInputLib# -Ldxvk-native/build/dxvk-native-master/lib/x86_64-linux-gnu -LSDL/build

all: TestStuff

TestStuff: $(SOURCES)
	$(CC) $(CFLAGS) $(LDFLAGS) $(SOURCES) -o TestStuff -lm $(LIBS)	\
	SDL/build/libSDL3.so	\
	libpng/build/libpng.so	\
	dxvk-native/build/dxvk-native-master/lib/x86_64-linux-gnu/libdxvk_dxgi.so	\
	dxvk-native/build/dxvk-native-master/lib/x86_64-linux-gnu/libdxvk_d3d11.so