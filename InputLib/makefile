CC=gcc
CFLAGS=-std=gnu2x -g -O0 -march=native -fpic -shared	\
	-DCGLM_FORCE_DEPTH_ZERO_TO_ONE	\
	-DCGLM_FORCE_LEFT_HANDED	\
	-I../SDL/include	\
	-I../uthash/src		\
	-I../cglm/include	\
	-Wall				\
	-Wl,-rpath='libs',--disable-new-dtags	#so exe looks in libs for shared libs
#	-Xlinker --verbose	
SOURCES=$(wildcard *.c)
LIBS=#-lvulkan -lUtilityLib -lMaterialLib -lMeshLib -lTerrainLib -ldxvk_d3d11 -ldxvk_dxgi
LDFLAGS=#-LUtilityLib -LMaterialLib -LMeshLib -LTerrainLib -Ldxvk-native/build/dxvk-native-master/lib/x86_64-linux-gnu -LSDL/build

all: InputLib

InputLib: $(SOURCES)
	$(CC) $(CFLAGS) $(LDFLAGS) $(SOURCES) -o ../libs/libInputLib.so $(LIBS)	\
	../SDL/build/libSDL3.so -lm