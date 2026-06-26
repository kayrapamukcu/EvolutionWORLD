# AGENTS.md

## Project Overview

EvolutionWORLD is a C++20 raylib creature simulation game. Creatures are made from physics nodes and muscles, simulated over generations, sorted by distance traveled, and reproduced with mutation.

The main architecture is:

- `src/main.cpp`: application entry point, raylib setup, menus, UI state machine, world creation/loading, and game views.
- `World`: population storage, generation simulation, worker threads, save/load, drawing helpers, percentile history.
- `Creature`: node/muscle physics, mutation, reproduction, and creature drawing.
- UI controls: `Button`, `InputField`, `Slider`, all derived from `UIElement`.
- `Helper`: shared drawing helpers, scaling constants, global app state, notices, and global `world`.

## Build

Primary build setup is Visual Studio:

- Open `EvolutionWORLD.sln`.
- Build the `EvolutionWORLD` project.
- The solution also builds vendored raylib from `build/external/raylib-master/`.

Important generated/config files:

- `build/build_files/EvolutionWORLD.vcxproj`
- `build/build_files/raylib.vcxproj`
- `CppProperties.json`

The app expects runtime assets under `resources/`, especially:

- `resources/mus_menu.ogg`
- `resources/romulus.png`

## Coding Guidelines

- Prefer small, local changes. This codebase is tightly coupled through globals and UI element indices.
- Keep C++ style consistent with the existing files.
- Use `rg` for search.
- Do not edit vendored raylib files under `build/external/raylib-master/` unless the task is specifically about raylib.
- Treat `bin/` as build output unless a task explicitly asks about release artifacts.
- Be careful with existing uncommitted changes. Do not revert user changes.

## Risk Areas

- `World` owns worker threads. Be careful with lifecycle, condition variables, `activeWorkers`, `generation`, and `terminate`.
- `World::Save()` / `World::Load()` use a custom binary format. Changes to serialized fields need version handling.
- `Creature` uses `unique_ptr<Node[]>` and `unique_ptr<Muscle[]>` with custom copy behavior.
- `Creature` and `World` depend on global `world` and static mutable settings.
- UI behavior in `main.cpp` often depends on hard-coded vector indices and `elementID` values.
- `Slider::draw()` assumes `maxVal != minVal`.
- `PercentileGraph` index math is delicate and assumes valid `world` and populated data.
- `UIElement` currently lacks a virtual destructor even though derived controls are stored through `unique_ptr<UIElement>`.

## Testing / Verification

There is no automated test suite in the repository.
Testing will be done manually for now.

## Notes For Future Agents

- `main.cpp` currently defines `std::unique_ptr<World> world` and `versionString`.
- `Helper.h` declares many inline globals used across the app.
- `World.h` contains serialization helpers and `MiniWorld`, so it is more than just declarations.