CC=gcc
CFLAGS=-std=gnu2x -g -O0 -march=x86-64 -msse4.2 -mavx2 -mf16c -fpic -shared	\
	-DCGLM_FORCE_DEPTH_ZERO_TO_ONE	\
	-DCGLM_FORCE_LEFT_HANDED	\
	-I../SDL/include	\
	-I../uthash/src		\
	-I../dxvk-native/include/native/directx \
	-I../dxvk-native/include/native/windows	\
	-I../dxvk-native/include/native	\
	-I../cglm/include	\
	-Wall				\
	-Wl,-rpath='$$ORIGIN',--disable-new-dtags	#so exe looks in . for shared libs
SOURCES=$(wildcard *.c)
LIBS=-lvulkan

all: UtilityLib

UtilityLib: $(SOURCES)
	$(CC) -o libUtilityLib.so $(SOURCES) $(CFLAGS) $(LDFLAGS) $(LIBS) \
	../SDL/build/libSDL3.so -lm	\
	../dxvk-native/build/dxvk-native-master/lib/x86_64-linux-gnu/libdxvk_dxgi.so	\
	../dxvk-native/build/dxvk-native-master/lib/x86_64-linux-gnu/libdxvk_d3d11.so