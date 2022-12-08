CC=g++	#linking cpp stuff causes mega link errors
CFLAGS=-std=c++17 -g -march=x86-64 -I../dxvk-native/include/native/directx \
	-I../SDL/include	\
	-I../dxvk-native/include/native/windows	#-Xlinker --verbose
SOURCES=$(wildcard *.c)
LIBS=-lvulkan

all: UtilityLib

UtilityLib: $(SOURCES)
	$(CC) $(CFLAGS) $(LDFLAGS) $(SOURCES) -o UtilityLib $(LIBS) \
	../SDL/build/libSDL3.a -lm	\
	../dxvk-native/build/dxvk-native-master/lib/x86_64-linux-gnu/libdxvk_d3d11.a	\
	../dxvk-native/build/dxvk-native-master/lib/x86_64-linux-gnu/libdxvk_dxgi.a
