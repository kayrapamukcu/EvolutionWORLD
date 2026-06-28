# EvolutionWORLD 

EvolutionWORLD is a creature simulation game built using C++ and Raylib.  
Creatures are made up of physics-based nodes and muscles that evolve over time through simple generational mechanics.
The player is free to play with the world's parameters at the worlds' creation. This can lead to creatures finding different ways/strategies to walk. 


**This project was heavily inspired by carykh's Evolution Simulator from 2015. You can take a look at his YouTube videos on it [here](https://www.youtube.com/watch?v=GOFws_hhZs8&list=PLrUdxfaFpuuK0rj55Rhc187Tn9vvxck7t).** The primary motivation for making this project was to make something similar to carykh's Evo.Sim., but make it faster by coding it in C++ and multithreading it. A secondary motivation was to make the world more customizable, and adding a save and load feature.

Only Windows executables are provided, but it should be possible to compile for Linux and Mac.

## Features

- **Custom World Generation**  
  Create unique worlds with different seeds, creature counts, tick rates, and more.

- **Physics-Based Creatures**  
  Each creature is composed of nodes and muscles governed by simple physics rules (mass, spring forces, friction, damping).

- **Procedural Evolution**  
  Creatures are evaluated and sorted every generation based on performance (distance traveled to the right), with the top half selected to reproduce.

- **History Tracking**  
  Best, average, and worst creatures from each generation are stored for later comparison and visualization.

## License

This project is licensed under the MIT License. See the LICENSE file for details.
