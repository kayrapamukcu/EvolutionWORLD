#pragma once
#include <vector>
#include "Helper.h"
#include <memory>

class World; // forward declaration

struct Node {
	float initialX, initialY;
	float x, y;
	float xSpeed, ySpeed;
	float mass;
	float friction;
	Node() = default;
};
struct Muscle {
	uint8_t node1, node2;
	uint16_t state1Ticks, state2Ticks;
	bool currentMuscleStage;
	uint8_t muscleTickCounter;
	float length1, length2;
	float strength;
	Muscle() = default;
};

class Creature {
public:
	static World* world;
	static int idCounter;
	static int FLOOR_HEIGHT;
	int id;
	float fitness = 0.0f;
	int muscleCount = 3;
	int nodeCount = 3;
	int tickCounter = 0;
	std::unique_ptr<Node[]> nodes;
	std::unique_ptr<Muscle[]> muscles;

	void reset();
	void draw();
	void initialize();
	void initializeChild(Creature* parent); 
	void tick();
	const float getCenterX() const;
	Creature reproduce();

	Creature() = default;
	Creature(const Creature& other);
	Creature& operator=(const Creature& other)
	{
		if (this == &other)
			return *this;

		id = other.id;
		fitness = other.fitness;
		muscleCount = other.muscleCount;
		nodeCount = other.nodeCount;
		tickCounter = other.tickCounter;

		nodes = std::make_unique<Node[]>(nodeCount);
		for (int i = 0; i < nodeCount; i++)
			nodes[i] = other.nodes[i];

		muscles = std::make_unique<Muscle[]>(muscleCount);
		for (int i = 0; i < muscleCount; i++)
			muscles[i] = other.muscles[i];

		return *this;
	}

	static void stretchRange(float baseMin, float baseMax, float r, float& outMin, float& outMax);
	static void updateNodeAndMuscleRangesAndVariations();
private:
	static float minNodeFriction;
	static float maxNodeFriction;
	static float minNodeMass;
	static float maxNodeMass;
	static float minNodeInitialX;
	static float maxNodeInitialX;
	static float minNodeInitialY;
	static float maxNodeInitialY;
	static float minMuscleStrength;
	static float maxMuscleStrength;
	static float minMuscleLength;
	static float maxMuscleLength;
	static float minMuscleStateTicks;
	static float maxMuscleStateTicks;

	static float nodeFrictionVariation;
	static float nodeMassVariation;
	static float nodeInitialXVariation;
	static float nodeInitialYVariation;
	static float muscleStrengthVariation;
	static float muscleLengthVariation;
	static int muscleStateTicksVariation;

	static int drawRadius;
};