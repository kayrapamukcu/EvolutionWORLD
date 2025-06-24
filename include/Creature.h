#pragma once

#include "Helper.h"

class World; // forward declaration

struct Node {
	float initialX, initialY;
	float x, y;
	float xSpeed, ySpeed;
	float mass;
	float friction;
};
struct Muscle {
	uint8_t node1, node2;
	float length1, length2;
	float strength;
};

class Creature {
public:
	static World* world;
	static int idCounter;
	static int FLOOR_HEIGHT;
	int id;
	float fitness = 0.0f;
	bool muscleStage = false;
	int muscleCount = 3;
	int nodeCount = 3;
	Node nodes[3];
	Muscle muscles[3];

	void reset();
	void draw();
	void initialize();
	void initializeChild(Creature* parent); 
	void tick();
	const float getCenterX() const;
	Creature reproduce();

	static float minNodeFriction;
	static float maxNodeFriction;
	static int getMaxMuscleCountForNodeCount(int nodeCount);
private:
	int tickCounter = ticksToSwitchMuscleStage;
};