CC=gcc
CFLAGS=-std=c17 -g -march=x86-64	\
	-I../dxvk-native/include/native/directx \
	-I../SDL/include	\
	-I../dxvk-native/include/native/windows	\
	-I../dxvk-native/include/native	\
	-Wl,-rpath='$$ORIGIN',--disable-new-dtags	#so exe looks in . for shared libs
SOURCES=$(wildcard *.c)
LIBS=-lvulkan

all: UtilityLib

UtilityLib: $(SOURCES)
	$(CC) $(CFLAGS) $(LDFLAGS) $(SOURCES) -o UtilityLib $(LIBS) \
	../SDL/build/libSDL3.so -lm	\
	../dxvk-native/build/dxvk-native-master/lib/x86_64-linux-gnu/libdxvk_dxgi.so	\
	../dxvk-native/build/dxvk-native-master/lib/x86_64-linux-gnu/libdxvk_d3d11.so
