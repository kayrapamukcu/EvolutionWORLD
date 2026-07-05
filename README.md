# EvolutionWORLD 

EvolutionWORLD is a creature simulation game built using C++ and Raylib.  
Creatures are made up of physics-based nodes and muscles that evolve over time through simple generational mechanics.
The player is free to play with the world's parameters at the worlds' creation. This can lead to creatures finding different ways/strategies to walk. 


**This project was heavily inspired by carykh's Evolution Simulator from 2015. You can take a look at his YouTube videos on it [here](https://www.youtube.com/watch?v=GOFws_hhZs8&list=PLrUdxfaFpuuK0rj55Rhc187Tn9vvxck7t).** The primary motivation for making this project was to make something similar to carykh's Evo.Sim., but make it faster by coding it in C++ and multithreading it. A secondary motivation was to make the world more customizable, and adding a save and load feature.

Windows executables may be provided for releases, but the project can also be built from source on Windows, macOS, and Linux with CMake.

## Features

- **Custom World Generation**  
  Create unique worlds with different seeds, creature counts, tick rates, and more.

- **Physics-Based Creatures**  
  Each creature is composed of nodes and muscles governed by simple physics rules (mass, spring forces, friction, damping).

- **Procedural Evolution**  
  Creatures are evaluated and sorted every generation based on performance (distance traveled to the right), with the top half selected to reproduce.

- **History Tracking**  
  Best, average, and worst creatures from each generation are stored for later comparison and visualization.

## Building From Source

The recommended cross-platform build path is CMake. The repository vendors raylib under `build/external/raylib-master/`, so a separate raylib install is not required.

### Requirements

- CMake 3.16 or newer
- A C++20 compiler
  - Windows: Visual Studio 2022 or another CMake-supported compiler
  - macOS: Xcode command line tools or Apple Clang
  - Linux: GCC or Clang

On Linux, raylib's desktop backend may require development packages for OpenGL/X11/audio depending on the distro. For example, Debian/Ubuntu users may need packages such as `build-essential`, `cmake`, `libasound2-dev`, `libx11-dev`, `libxrandr-dev`, `libxi-dev`, `libgl1-mesa-dev`, `libglu1-mesa-dev`, `libxcursor-dev`, and `libxinerama-dev`.

### Configure And Build

```sh
cmake -S . -B build/cmake
cmake --build build/cmake --config Release
```

The executable is written under `build/cmake/bin/` for single-config generators. For Visual Studio and other multi-config generators, it is under `build/cmake/bin/<Config>/`.

The CMake build copies `resources/` next to the executable after each build. The game expects that folder to be available at runtime.

### Run

From the build output directory:

```sh
./EvolutionWORLD
```

On Windows, run `EvolutionWORLD.exe` instead.

### Visual Studio Solution

The existing Visual Studio solution is still available:

- Open `EvolutionWORLD.sln`.
- Build the `EvolutionWORLD` project.

## License

This project is licensed under the MIT License. See the LICENSE file for details.
