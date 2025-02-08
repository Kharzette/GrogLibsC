# GrogLibsC
Game libraries using C language, often ported from my C# libraries of the same name.  This all sits atop dxvk-native and is only really working on linux64 that I know of.  I'll get around to testing a windows build one of these days.

![alt text](https://github.com/Kharzette/MiscMedia/blob/master/GrogLibsC.png?raw=true "Groggy")

# Working Stuff
- Animated character style meshes
- Really basic input
- Very simple terrain generation / rendering
- UI Module using [Clay](https://github.com/nicbarker/clay)

# Not Working Stuff
- Terrain raycasts

# TODO
- BSP Maps
- Integration of a physics library of some sort
- Better collision / physics on characters
- Static meshes with various collision shapes
- Windows

# Building
For awhile I had every dependency as a submodule, but that made it annoying for people.  These packages should be installed:
- SDL2
- FAudio
- pnglib

The dxvk-native submodule needs building:
```bash
meson --buildtype "debug" build
cd build
ninja
```
Grog's makefiles will look for the libs there in dxvk-native/build/etc

So something really annoying I do that I should probably fix is using .vscode launch.json and tasks.json to copy the shared libraries around.  I should either commit those or stop doing that and have the makefiles do something more reasonable.

# Coordinate System
I've fairly recently switched from right to left handed.  Since early directX used left, my brain is just wired to work in left handed.  Right made it much easier to get data in and out of blender, but when doing gameplay and cameras and such I was having a rough time of it.

Doing a Ludum Dare on the old original XBox kind of brought me back to the left hand way of doing things.

Z is forward, X is right and Y is up.