CC=gcc
CFLAGS=-std=c17 -g -march=x86-64 -I../dxvk-native/include/native/directx \
	-I../dxvk-native/include/native/windows -v	#-Xlinker --verbose
SOURCES=$(wildcard *.c)

all: UtilityLib

UtilityLib: $(SOURCES)
	$(CC) $(CFLAGS) $(LDFLAGS) $(SOURCES) -o UtilityLib $(LIBS) \
	../dxvk-native/build/dxvk-native-master/lib/x86_64-linux-gnu/libdxvk_d3d11.so \
	../dxvk-native/build/dxvk-native-master/lib/x86_64-linux-gnu/libdxvk_dxgi.so \
	../SDL/build/libSDL3.a -lm