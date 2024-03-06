CC=gcc
CFLAGS=-std=c17 -g -march=x86-64 -msse4.2 -fpic -shared	\
	-I../dxvk-native/include/native/directx \
	-I../SDL/include	\
	-I../libpng/build/include	\
	-I../dxvk-native/include/native/windows	\
	-I../dxvk-native/include/native	\
	-I../uthash/include	\
	-I../cglm/include	\
	-Wall				\
	-Wl,-rpath='$$ORIGIN',--disable-new-dtags	#so exe looks in . for shared libs
SOURCES=$(wildcard *.c)
LIBS=-lvulkan

all: MaterialLib

MaterialLib: $(SOURCES)
	$(CC) $(CFLAGS) $(LDFLAGS) $(SOURCES) -o libMaterialLib.so $(LIBS) \
	../SDL/build/libSDL3.so -lm	\
	../libpng/build/lib/libpng.so	\
	../dxvk-native/build/dxvk-native-master/lib/x86_64-linux-gnu/libdxvk_dxgi.so	\
	../dxvk-native/build/dxvk-native-master/lib/x86_64-linux-gnu/libdxvk_d3d11.so