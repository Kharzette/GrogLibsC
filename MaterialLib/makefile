CC=gcc
CFLAGS=-std=gnu23 -g -O0 -march=native -fpic -shared	\
	-DCGLM_FORCE_DEPTH_ZERO_TO_ONE	\
	-DCGLM_FORCE_LEFT_HANDED	\
	-I../dxvk-native/include/native/directx \
	-I../dxvk-native/include/native/windows	\
	-I../dxvk-native/include/native	\
	-I../uthash/src		\
	-I../cglm/include	\
	-Wall				\
	-Wl,-rpath='$$ORIGIN',--disable-new-dtags	#so exe looks in . for shared libs

SOURCES=$(wildcard *.c)
OBJS=$(SOURCES:.c=.o)
LIBS=-lvulkan -lpng16 -lSDL3

all: MaterialLib

MaterialLib: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o ../libs/libMaterialLib.so $(LIBS) \
	../dxvk-native/build/src/dxgi/libdxvk_dxgi.so	\
	../dxvk-native/build/src/d3d11/libdxvk_d3d11.so
