CC=gcc
CFLAGS=-std=c17 -g -march=x86-64		\
	-Iuthash/include	\
	-Idxvk-native/include/native/windows	\
	-Idxvk-native/include/native/directx \
	-Wl,-rpath='libs',--disable-new-dtags	#so exe looks in libs for shared libs
#	-Xlinker --verbose	
SOURCES=$(wildcard *.c)
LIBS=-lvulkan -lUtilityLib -lMaterialLib# -ldxvk_d3d11 -ldxvk_dxgi
LDFLAGS=-LUtilityLib -LMaterialLib# -Ldxvk-native/build/dxvk-native-master/lib/x86_64-linux-gnu -LSDL/build

all: TestStuff

TestStuff: $(SOURCES)
	$(CC) $(CFLAGS) $(LDFLAGS) $(SOURCES) -o TestStuff $(LIBS)	\
	SDL/build/libSDL3.so -lm	\
	dxvk-native/build/dxvk-native-master/lib/x86_64-linux-gnu/libdxvk_dxgi.so	\
	dxvk-native/build/dxvk-native-master/lib/x86_64-linux-gnu/libdxvk_d3d11.so
