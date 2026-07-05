#include "Creature.h"
#include "RNG.h"
#include "Raylib.h"
#include "World.h"

#include <iostream>
#include <cmath>
#include <algorithm>
#include <string>

int Creature::idCounter = 0;
int Creature::FLOOR_HEIGHT = 100; 

int Creature::drawRadius = 10;

float Creature::minNodeFriction = 1.0f;
float Creature::maxNodeFriction = 4.0f;
float Creature::minNodeMass = 0.3f;
float Creature::maxNodeMass = 2.0f;
float Creature::minNodeInitialX = -100.0f;
float Creature::maxNodeInitialX = 100.0f;
float Creature::minNodeInitialY = -100.0f;
float Creature::maxNodeInitialY = 100.0f;
float Creature::minMuscleStrength = 1.0f;
float Creature::maxMuscleStrength = 2.5f;
float Creature::minMuscleLength = 80.0f;
float Creature::maxMuscleLength = 160.0f;
float Creature::minMuscleStateTicks = 20.0f;
float Creature::maxMuscleStateTicks = 80.0f;

float Creature::nodeFrictionVariation = 0.05f;
float Creature::nodeMassVariation = 0.03f;
float Creature::nodeInitialXVariation = 1.0f;
float Creature::nodeInitialYVariation = 1.0f;
float Creature::muscleStrengthVariation = 0.03f;
float Creature::muscleLengthVariation = 5.0f;
int Creature::muscleStateTicksVariation = 2;

Creature::Creature(const Creature& other)
{
	id = other.id;
	fitness = other.fitness;
	muscleCount = other.muscleCount;
	nodeCount = other.nodeCount;

	nodes = std::make_unique<Node[]>(nodeCount);
	for (int i = 0; i < nodeCount; i++)
		nodes[i] = other.nodes[i];

	muscles = std::make_unique<Muscle[]>(muscleCount);
	for (int i = 0; i < muscleCount; i++)
		muscles[i] = other.muscles[i];
}

void Creature::stretchRange(float baseMin,float baseMax,float r,float& outMin,float& outMax){
	if (r >= 1.0f) {
		outMin = baseMin / r;
		outMax = baseMax * r;
	}
	else {
		const float halfSpan = 0.5f * (baseMax - baseMin);
		const float t = 1.0f - r;

		outMin = baseMin + t * halfSpan;
		outMax = baseMax - t * halfSpan;
	}
}

void Creature::updateNodeAndMuscleRangesAndVariations() {
	float r = world->mutabilityRange;
	stretchRange(1.0f, 4.0f, r, minNodeFriction, maxNodeFriction);
	stretchRange(0.3f, 2.0f, r, minNodeMass, maxNodeMass);
	stretchRange(-100.0f, 100.0f, r, minNodeInitialX, maxNodeInitialX);
	stretchRange(-100.0f, 100.0f, r, minNodeInitialY, maxNodeInitialY);
	stretchRange(1.0f, 2.5f, r, minMuscleStrength, maxMuscleStrength);
	stretchRange(80.0f, 160.0f, r, minMuscleLength, maxMuscleLength);
	stretchRange(20.0f, 80.0f, r, minMuscleStateTicks, maxMuscleStateTicks);

	nodeFrictionVariation = 0.01f * world->mutabilityFactor;
	nodeMassVariation = 0.01f * world->mutabilityFactor;
	nodeInitialXVariation = 1.0f * world->mutabilityFactor;
	nodeInitialYVariation = 1.0f * world->mutabilityFactor;
	muscleStrengthVariation = 0.01f * world->mutabilityFactor;
	muscleLengthVariation = 10.0f * world->mutabilityFactor;
	muscleStateTicksVariation = 0.5f * world->mutabilityFactor;
}

void Creature::initializeChild(Creature* parent) {
	id = idCounter++;

	nodeCount = parent->nodeCount;
	muscleCount = parent->muscleCount;

	nodes = std::make_unique<Node[]>(parent->nodeCount);
	muscles = std::make_unique<Muscle[]>(parent->muscleCount);

	for (int i = 0; i < muscleCount; i++) {
		muscles[i].node1 = parent->muscles[i].node1;
		muscles[i].node2 = parent->muscles[i].node2;
	}

	for (int i = 0; i < nodeCount; ++i) {
		nodes[i].initialX = std::clamp(parent->nodes[i].initialX + RNG::randomFloat(-nodeInitialXVariation, nodeInitialXVariation), minNodeInitialX, maxNodeInitialX);
		nodes[i].initialY = std::clamp(parent->nodes[i].initialY + RNG::randomFloat(-nodeInitialYVariation, nodeInitialYVariation), minNodeInitialY, maxNodeInitialY);
		nodes[i].x = nodes[i].initialX;
		nodes[i].y = nodes[i].initialY;
		nodes[i].xSpeed = 0.0f;
		nodes[i].ySpeed = 0.0f;
		nodes[i].mass = std::clamp(parent->nodes[i].mass + RNG::randomFloat(-nodeMassVariation, nodeMassVariation), minNodeMass, maxNodeMass);
		nodes[i].friction = std::clamp(parent->nodes[i].friction + RNG::randomFloat(-nodeFrictionVariation, nodeFrictionVariation), minNodeFriction, maxNodeFriction);
	}
	for (int i = 0; i < muscleCount; ++i) {
		muscles[i].state1Ticks = std::clamp(parent->muscles[i].state1Ticks + RNG::randomInt(-muscleStateTicksVariation, muscleStateTicksVariation), (int)minMuscleStateTicks, (int)maxMuscleStateTicks);
		muscles[i].state2Ticks = std::clamp(parent->muscles[i].state2Ticks + RNG::randomInt(-muscleStateTicksVariation, muscleStateTicksVariation), (int)minMuscleStateTicks, (int)maxMuscleStateTicks);
		muscles[i].length1 = std::clamp(parent->muscles[i].length1 + RNG::randomFloat(-muscleLengthVariation, muscleLengthVariation), minMuscleLength, maxMuscleLength);
		muscles[i].length2 = std::clamp(parent->muscles[i].length2 + RNG::randomFloat(-muscleLengthVariation, muscleLengthVariation), minMuscleLength, maxMuscleLength);
		muscles[i].strength = std::clamp(parent->muscles[i].strength + RNG::randomFloat(-muscleStrengthVariation, muscleStrengthVariation), minMuscleStrength, maxMuscleStrength);
		muscles[i].currentMuscleStage = 0;
		muscles[i].muscleTickCounter = 0;
	}
}

bool Creature::hasConnection(int a, int b) const
{
	if (a > b) std::swap(a, b);
	for (int i = 0; i < muscleCount; ++i) {
		int x = muscles[i].node1;
		int y = muscles[i].node2;

		if (x == a && y == b) {
			return true;
		}
	}

	return false;
}

void Creature::initialize()
{
	id = idCounter++;

	constexpr int minNodes = 3;
	constexpr int maxNodes = 32;

	nodeCount = RNG::biasedLowerInt(minNodes, maxNodes, 0.4f);

	int minMuscles = nodeCount;
	int maxMuscles = nodeCount * (nodeCount - 1) / 2;

	muscleCount = RNG::biasedLowerInt(minMuscles, maxMuscles, 0.25f);

	nodes = std::make_unique<Node[]>(nodeCount);
	muscles = std::make_unique<Muscle[]>(muscleCount);

	for (int i = 0; i < nodeCount; i++) {
		muscles[i].node1 = i;
		muscles[i].node2 = (i + 1) % nodeCount;
		if (muscles[i].node1 > muscles[i].node2) std::swap(muscles[i].node1, muscles[i].node2);
	}

	if (muscleCount > nodeCount) {
		for (int i = nodeCount; i < muscleCount; ++i) {
			int a, b;
			do {
				a = RNG::randomInt(0, nodeCount - 1);
				b = RNG::randomInt(0, nodeCount - 1);
			} while (a == b || hasConnection(a, b));

			if (a > b) std::swap(a, b);
			muscles[i].node1 = a;
			muscles[i].node2 = b;
		}
	}

	for (int i = 0; i < nodeCount; ++i) {
		nodes[i].initialX = RNG::randomFloat(minNodeInitialX, maxNodeInitialX);
		nodes[i].initialY = RNG::randomFloat(minNodeInitialY, maxNodeInitialY);
		nodes[i].x = nodes[i].initialX;
		nodes[i].y = nodes[i].initialY;
		nodes[i].mass = RNG::randomFloat(minNodeMass, maxNodeMass);
		nodes[i].xSpeed = 0.0f;
		nodes[i].ySpeed = 0.0f;
		nodes[i].friction = RNG::randomFloat(minNodeFriction, maxNodeFriction);
	}
	for (int i = 0; i < muscleCount; ++i) {
		muscles[i].state1Ticks = RNG::randomInt(minMuscleStateTicks, maxMuscleStateTicks);
		muscles[i].state2Ticks = RNG::randomInt(minMuscleStateTicks, maxMuscleStateTicks);
		muscles[i].length1 = RNG::randomFloat(minMuscleLength, maxMuscleLength);
		muscles[i].length2 = RNG::randomFloat(minMuscleLength, maxMuscleLength);
		muscles[i].strength = RNG::randomFloat(minMuscleStrength, maxMuscleStrength);
		muscles[i].currentMuscleStage = 0;
		muscles[i].muscleTickCounter = 0;
	}
}

void Creature::tick()
{
    constexpr float airFriction = 0.95f;
	constexpr float muscleDamping = 0.05f;
	constexpr float timestepScale = 1.0f / 250.f;
	constexpr float gravityMult = 1.0f / 3266.0f;
	constexpr float bounceDamping = 0.00f;

    for (int i = 0; i < muscleCount; ++i)
    {
        Muscle& m = muscles[i];
        Node& n1 = nodes[m.node1];
        Node& n2 = nodes[m.node2];

        // vector from n1 to n2
        float dx = n2.x - n1.x;
        float dy = n2.y - n1.y;
        float dist = std::sqrt(dx * dx + dy * dy);

		// update muscle stage
		if (m.muscleTickCounter >= (m.currentMuscleStage ? m.state1Ticks : m.state2Ticks )) {
			m.currentMuscleStage = !m.currentMuscleStage;
			m.muscleTickCounter = 0;
		}

		m.muscleTickCounter++;

        if (dist < 1e-5f) continue;

        // which rest length are we aiming for this tick?
		
        float rest = m.currentMuscleStage ? m.length1 : m.length2;

        float stretch = dist - rest;

        // unit vector along the muscle
        float ux = dx / dist;
        float uy = dy / dist;

		float rvx = n2.xSpeed - n1.xSpeed;
		float rvy = n2.ySpeed - n1.ySpeed;
		float relativeSpeed = rvx * ux + rvy * uy;

        float forceMag = m.strength * stretch + muscleDamping * relativeSpeed;

        // force components
        float fx = forceMag * ux;
        float fy = forceMag * uy;

        // a = F / m
		n1.xSpeed += fx / n1.mass * timestepScale;
		n1.ySpeed += fy / n1.mass * timestepScale;
		n2.xSpeed += -fx / n2.mass * timestepScale;
		n2.ySpeed += -fy / n2.mass * timestepScale;
    }

    // integrate positions, apply damping & simple ground collision
    for (int i = 0; i < nodeCount; ++i) {
        Node& n = nodes[i];

		n.ySpeed += World::gravity * gravityMult;

        n.xSpeed *= airFriction;
        n.ySpeed *= airFriction;

        n.y += n.ySpeed;

        if (n.y > FLOOR_HEIGHT - drawRadius) {
			float slip = 1.0f / n.friction;

			n.x += n.xSpeed * slip;
			n.xSpeed *= 0.84f + 0.16f * slip;
            n.y = FLOOR_HEIGHT - drawRadius;
			n.ySpeed = -n.ySpeed * bounceDamping;
        }
        else {
            n.x += n.xSpeed;
        }
    }
}

void Creature::normalizeForEvaluation()
{
	const float centerX = getInitialCenterX();
	float lowestY = nodes[0].initialY;
	for (int i = 1; i < nodeCount; ++i) {
		lowestY = std::max(lowestY, nodes[i].initialY);
	}

	const float floorY = (float)(FLOOR_HEIGHT - drawRadius);
	const float yOffset = floorY - lowestY;

	for (int i = 0; i < nodeCount; ++i) {
		nodes[i].initialX -= centerX;
		nodes[i].initialY += yOffset;
	}

	reset();
}

float Creature::getCenterX() const
{
	float centerX = 0;
	for (int i = 0; i < nodeCount; ++i) {
		centerX += nodes[i].x;
	}
	centerX /= nodeCount;
	return centerX;
}

float Creature::getInitialCenterX() const
{
	float centerX = 0;
	for (int i = 0; i < nodeCount; ++i) {
		centerX += nodes[i].initialX;
	}
	centerX /= nodeCount;
	return centerX;
}

void Creature::reset()
{
	for (int i = 0; i < nodeCount; ++i) {
		nodes[i].x = nodes[i].initialX;
		nodes[i].y = nodes[i].initialY;
		nodes[i].xSpeed = 0;
		nodes[i].ySpeed = 0;
	}
	for (int i = 0; i < muscleCount; ++i) {
		muscles[i].currentMuscleStage = 0;
		muscles[i].muscleTickCounter = 0;
	}
}

void Creature::draw() {
	const float massVariation = (maxNodeMass - minNodeMass);
	const float frictionVariation = (maxNodeFriction - minNodeFriction);
	
	for (int i = 0; i < nodeCount; ++i) {
		Node& n = nodes[i];

		float frictionRatio = frictionVariation != 0 ? (n.friction - minNodeFriction) / frictionVariation : 0.0f;
		float massRatio = massVariation != 0 ? (n.mass - minNodeMass) / massVariation : 0.0f;

		frictionRatio = std::clamp(frictionRatio, 0.0f, 1.0f);
		massRatio = std::clamp(massRatio, 0.0f, 1.0f);

		// Brightness: 255 (low mass) to 50 (high mass)
		unsigned char brightness = static_cast<unsigned char>(255 - 205 * massRatio);

		// Red channel blends between brightness and full red based on friction
		unsigned char red = static_cast<unsigned char>(brightness + (255 - brightness) * frictionRatio);

		// Final color: only red and grayscale
		Color nodeColor = {red, brightness, brightness, 255};
		DrawCircle(n.x,n.y,drawRadius, nodeColor);
	}
	for (int i = 0; i < muscleCount; i++) {
		DrawLine(
			nodes[muscles[i].node1].x,
			nodes[muscles[i].node1].y, 
			nodes[muscles[i].node2].x,
			nodes[muscles[i].node2].y, 
			BLUE);
	}
 }

Creature Creature::reproduce() {
	Creature child;
	child.initializeChild(this);
	child.normalizeForEvaluation();
	return child;
}
