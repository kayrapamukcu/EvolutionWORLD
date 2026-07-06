#pragma once

#include <vector>
#include "raylib.h"
#include "Helper.h"

class Creature;
class World;

struct SpeciesCount {
	int nodeCount = 0;
	int muscleCount = 0;
	int count = 0;
};

class SpeciesGraph {
public:
	World* world = nullptr;
	int x, y, width, height;

	int maxDataPoints = 500;
	int firstGeneration = 0;
	std::vector<int> storedGenerations;
	std::vector<std::vector<SpeciesCount>> data;
	Color backgroundColor = { 240, 188, 120, 255 };

	void draw() const;
	void addGeneration(int generation, const Creature* creatures, int creatureCount);
	void compressGraph();

	SpeciesGraph(int xPos, int yPos, int w, int h) : x(xPos), y(yPos), width(w), height(h) {
		data.reserve(maxDataPoints * 2);
	}
};
