#include "Creature.h"
#include "RNG.h"
#include "Raylib.h"
#include "World.h"

#include <iostream>
#include <cmath>
#include <algorithm>
#include <string>

int Creature::idCounter = 0;
World* Creature::world = nullptr;
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
float Creature::minMuscleStateTicks = 20.0f * 255;
float Creature::maxMuscleStateTicks = 80.0f * 255;

float Creature::nodeFrictionVariation = 0.01f;
float Creature::nodeMassVariation = 0.01f;
float Creature::nodeInitialXVariation = 1.0f;
float Creature::nodeInitialYVariation = 1.0f;
float Creature::muscleStrengthVariation = 0.01f;
float Creature::muscleLengthVariation = 10.0f;
int Creature::muscleStateTicksVariation = 127;

void Creature::stretchRange(float baseMin,float baseMax,float r,float& outMin,float& outMax){
	if (r >= 1.0f) {
		outMin = baseMin / r;
		outMax = baseMax * r;
	}
	else {
		const float mean = 0.5f * (baseMin + baseMax);
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
	stretchRange(255 * 20.0f, 255 * 80.0f, r, minMuscleStateTicks, maxMuscleStateTicks);

	nodeFrictionVariation = 0.01f * world->mutabilityFactor;
	nodeMassVariation = 0.01f * world->mutabilityFactor;
	nodeInitialXVariation = 1.0f * world->mutabilityFactor;
	nodeInitialYVariation = 1.0f * world->mutabilityFactor;
	muscleStrengthVariation = 0.01f * world->mutabilityFactor;
	muscleLengthVariation = 10.0f * world->mutabilityFactor;
	muscleStateTicksVariation = 255 * 0.5f * world->mutabilityFactor;
}

void Creature::initializeChild(Creature* parent) {
	id = idCounter++;

	float mutabilityRange = world->mutabilityRange;
	float mutabilityFactor = world->mutabilityFactor;

	for (int i = 0; i < nodeCount; i++) {
		muscles[i].node1 = i;
		muscles[i].node2 = (i + 1) % nodeCount;
	}
	for (int i = 0; i < nodeCount; ++i) {
		nodes[i].initialX = std::clamp(parent->nodes[i].initialX + world->rng.randomFloat(-nodeInitialXVariation, nodeInitialXVariation), minNodeInitialX, maxNodeInitialX);
		nodes[i].initialY = std::clamp(parent->nodes[i].initialY + world->rng.randomFloat(-nodeInitialYVariation, nodeInitialYVariation), minNodeInitialY, maxNodeInitialY);
		nodes[i].x = nodes[i].initialX;
		nodes[i].y = nodes[i].initialY;
		nodes[i].xSpeed = 0.0f;
		nodes[i].ySpeed = 0.0f;
		nodes[i].mass = std::clamp(parent->nodes[i].mass + world->rng.randomFloat(-nodeMassVariation, nodeMassVariation), minNodeMass, maxNodeMass);
		nodes[i].friction = std::clamp(parent->nodes[i].friction + world->rng.randomFloat(-nodeFrictionVariation, nodeFrictionVariation), minNodeFriction, maxNodeFriction);
	}
	for (int i = 0; i < muscleCount; ++i) {
		muscles[i].state1Ticks = std::clamp(parent->muscles[i].state1Ticks + world->rng.randomInt(-muscleStateTicksVariation, muscleStateTicksVariation), (int)minMuscleStateTicks, (int)maxMuscleStateTicks);
		muscles[i].state2Ticks = std::clamp(parent->muscles[i].state2Ticks + world->rng.randomInt(-muscleStateTicksVariation, muscleStateTicksVariation), (int)minMuscleStateTicks, (int)maxMuscleStateTicks);
		muscles[i].length1 = std::clamp(parent->muscles[i].length1 + world->rng.randomFloat(-muscleLengthVariation, muscleLengthVariation), minMuscleLength, maxMuscleLength);
		muscles[i].length2 = std::clamp(parent->muscles[i].length2 + world->rng.randomFloat(-muscleLengthVariation, muscleLengthVariation), minMuscleLength, maxMuscleLength);
		muscles[i].strength = std::clamp(parent->muscles[i].strength + world->rng.randomFloat(-muscleStrengthVariation, muscleStrengthVariation), minMuscleStrength, maxMuscleStrength);
		muscles[i].currentMuscleStage = 0;
		muscles[i].muscleTickCounter = 0;
	}
	tickCounter = 0;
}

void Creature::initialize()
{
	id = idCounter++;
	
	for (int i = 0; i < nodeCount; i++) {
		muscles[i].node1 = i;
		muscles[i].node2 = (i + 1) % nodeCount;
	}

	for (int i = 0; i < nodeCount; ++i) {
		nodes[i].initialX = world->rng.randomFloat(minNodeInitialX, maxNodeInitialX);
		nodes[i].initialY = world->rng.randomFloat(minNodeInitialY, maxNodeInitialY);
		nodes[i].x = nodes[i].initialX;
		nodes[i].y = nodes[i].initialY;
		nodes[i].mass = world->rng.randomFloat(minNodeMass, maxNodeMass);
		nodes[i].xSpeed = 0.0f;
		nodes[i].ySpeed = 0.0f;
		nodes[i].friction = world->rng.randomFloat(minNodeFriction, maxNodeFriction);
	}
	for (int i = 0; i < muscleCount; ++i) {
		muscles[i].state1Ticks = world->rng.randomInt(minMuscleStateTicks, maxMuscleStateTicks);
		muscles[i].state2Ticks = world->rng.randomInt(minMuscleStateTicks, maxMuscleStateTicks);
		muscles[i].length1 = world->rng.randomFloat(minMuscleLength, maxMuscleLength);
		muscles[i].length2 = world->rng.randomFloat(minMuscleLength, maxMuscleLength);
		muscles[i].strength = world->rng.randomFloat(minMuscleStrength, maxMuscleStrength);
		muscles[i].currentMuscleStage = 0;
		muscles[i].muscleTickCounter = 0;
	}
	tickCounter = 0;
}

void Creature::tick()
{
    constexpr float airFriction = 0.95f;
	constexpr float timestepScale = 1.0f / 250.f;
	constexpr float gravityMult = 1.0f / 3266.0f;

    for (int i = 0; i < muscleCount; ++i)
    {
        Muscle& m = muscles[i];
        Node& n1 = nodes[m.node1];
        Node& n2 = nodes[m.node2];

        // vector from n1 to n2
        float dx = n2.x - n1.x;
        float dy = n2.y - n1.y;
        float dist = std::sqrt(dx * dx + dy * dy);
        if (dist < 1e-5f) continue;

        // which rest length are we aiming for this tick?
		
		if (m.muscleTickCounter >= (m.currentMuscleStage ? m.state1Ticks/255 : m.state2Ticks/255)) {
			m.currentMuscleStage = !m.currentMuscleStage;
			m.muscleTickCounter = 0;
		}

        float rest = m.currentMuscleStage ? m.length1 : m.length2;

        float stretch = dist - rest;
        float forceMag = m.strength * stretch;

        // unit vector along the muscle
        float ux = dx / dist;
        float uy = dy / dist;

        // force components
        float fx = forceMag * ux;
        float fy = forceMag * uy;

        // a = F / m
		n1.xSpeed += fx / n1.mass * timestepScale;
		n1.ySpeed += fy / n1.mass * timestepScale;
		n2.xSpeed += -fx / n2.mass * timestepScale;
		n2.ySpeed += -fy / n2.mass * timestepScale;
		// update muscle stage
		m.muscleTickCounter++;
    }

    // integrate positions, apply damping & simple ground collision
    for (int i = 0; i < nodeCount; ++i) {
        Node& n = nodes[i];

		n.ySpeed += world->gravity * gravityMult;

        n.xSpeed *= airFriction;
        n.ySpeed *= airFriction;

        n.y += n.ySpeed;

        if (n.y > FLOOR_HEIGHT - drawRadius) {
            n.x += n.xSpeed * 1/n.friction;
            n.y = FLOOR_HEIGHT - drawRadius;
            n.ySpeed = 0.0f;
        }
        else {
            n.x += n.xSpeed;
        }
    }

	tickCounter++;
}

const float Creature::getCenterX() const
{
	float centerX = 0;
	for (int i = 0; i < nodeCount; ++i) {
		centerX += nodes[i].x;
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
	tickCounter = 0;

}

void Creature::draw() {
	const float massVariation = (maxNodeMass - minNodeMass);
	const float frictionVariation = (maxNodeFriction - minNodeFriction);
	
	for (int i = 0; i < nodeCount; ++i) {
		Node& n = nodes[i];
		//Color nodeColor = { 50 + 205 - 205 * (n.friction - minNodeFriction) / frictionVariation, 255 - 255 * (n.mass - minNodeMass) / massVariation , 255 - 255 * (n.mass - minNodeMass) / massVariation , 255};

		float frictionRatio = frictionVariation != 0 ? (n.friction - minNodeFriction) / frictionVariation : 0.0f;
		float massRatio = massVariation != 0 ? (n.mass - minNodeMass) / massVariation : 0.0f;

		// Clamp to [0, 1]
		frictionRatio = std::clamp(frictionRatio, 0.0f, 1.0f);
		massRatio = std::clamp(massRatio, 0.0f, 1.0f);

		// Brightness: 255 (low mass) to 50 (high mass)
		unsigned char brightness = static_cast<unsigned char>(255 - 205 * massRatio);

		// Red channel blends between brightness and full red based on friction
		unsigned char red = static_cast<unsigned char>(brightness + (255 - brightness) * frictionRatio);

		// Final color: only red and grayscale
		Color nodeColor = {
			red,
			brightness,
			brightness,
			255
		};
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
	return child;
}