CC=gcc
CFLAGS=-std=gnu2x -g -O0 -march=x86-64 -msse4.2 -mavx2 -fpic -shared	\
	-DCGLM_FORCE_DEPTH_ZERO_TO_ONE	\
	-DCGLM_FORCE_LEFT_HANDED	\
	-I../dxvk-native/include/native/directx \
	-I../SDL/include	\
	-I../libpng/build/include	\
	-I../dxvk-native/include/native/windows	\
	-I../dxvk-native/include/native	\
	-I../uthash/src		\
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