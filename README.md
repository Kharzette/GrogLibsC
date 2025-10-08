# GrogLibsC
Game libraries using C language, often ported from my C# libraries of the same name.  This all sits atop dxvk-native and is only really working on linux64 that I know of.  I'll get around to testing a windows build one of these days.

![alt text](https://github.com/Kharzette/MiscMedia/blob/master/GrogLibsC.png?raw=true "Groggy")

# Working Stuff
- Animated character style meshes
- Really basic input
- Very simple terrain generation / rendering
- UI Module using [Clay](https://github.com/nicbarker/clay)

# TODO
- Better and more focused smaller test programs
- Integration of a physics library of some sort (Jolt in progress)
- Static meshes with various collision shapes
- Better collision / physics on characters
- Windows

# Building
For awhile I had every dependency as a submodule, but that made it annoying for people.  These packages should be installed:
- SDL3
- FAudio
- pnglib

The dxvk-native submodule needs building:
```bash
meson --buildtype "debug" build
cd build
ninja
```

Build JoltPhysics
```bash
cd JoltPhysics/Build
cmake -S . -B Linux_Debug -G "Unix Makefiles" -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=g++
cd Linux_Debug
make -j3
```

I've recently ditched makefiles for a build that uses a c program to build.  It is quite fast and easy to use.  The repo for the h file is [here](https://github.com/tsoding/nob.h).  Just run ./BuildIt.  If it isn't there do:
```bash
gcc BuildIt.c -o BuildIt
```

For my own testing with vscode, I have a variable in settings.json called CurrentGame that defines where my game directory is located, that contains all the game assets and such.  CurrentGameLibs is where all of the various libraries get copied to for the run or debug.

# Coordinate System
I've fairly recently switched from right to left handed.  Since early directX used left, my brain is just wired to work in left handed.  Right made it much easier to get data in and out of blender, but when doing gameplay and cameras and such I was having a rough time of it.

Doing a Ludum Dare on the old original XBox kind of brought me back to the left hand way of doing things.

Z is forward, X is right and Y is up.