CC=gcc
CFLAGS=-std=c17 -g -march=x86-64 -msse2 -mf16c -fpic -shared	\
	-I../dxvk-native/include/native/directx \
	-I../dxvk-native/include/native/windows	\
	-I../uthash/include	\
	-I../cglm/include	\
	-Wl,-rpath='$$ORIGIN',--disable-new-dtags	#so exe looks in . for shared libs
SOURCES=$(wildcard *.c)
#LIBS=-lvulkan

all: MeshLib

MeshLib: $(SOURCES)
	$(CC) $(CFLAGS) $(LDFLAGS) $(SOURCES) -o libMeshLib.so -lm $(LIBS) \
	../dxvk-native/build/dxvk-native-master/lib/x86_64-linux-gnu/libdxvk_dxgi.so	\
	../dxvk-native/build/dxvk-native-master/lib/x86_64-linux-gnu/libdxvk_d3d11.so