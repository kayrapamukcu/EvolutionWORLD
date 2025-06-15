#pragma once

#include "Helper.h"

class World; // forward declaration

struct Node {
	float x, y;
	float xSpeed, ySpeed;
	float mass;
	float friction;
};
struct Muscle {
	int node1;
	int node2;
	float length1;
	float length2;
	float strength;
};

class Creature {
public:
	static World* world;

	static int idCounter;
	int id;
	bool muscleStage = 0;
	Node nodes[3];
	Muscle muscles[3];

	void reset();
	void draw();
	void initialize();
	void tick();
	float getCenterX();
	Creature reproduce();
private:
	int tickCounter = ticksToSwitchMuscleStage;
};