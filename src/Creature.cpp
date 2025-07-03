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
float Creature::minNodeFriction = 1.0f;
float Creature::maxNodeFriction = 4.0f;

void Creature::initializeChild(Creature* parent) {
	id = idCounter++;
	for (int i = 0; i < nodeCount; i++) {
		muscles[i].node1 = i;
		muscles[i].node2 = (i + 1) % nodeCount;
	}
	for (int i = 0; i < nodeCount; ++i) {
		nodes[i].initialX = std::clamp(parent->nodes[i].initialX + world->rng.randomFloat(-1.0f, 1.0f), -50.0f, 50.0f);
		nodes[i].initialY = std::clamp(parent->nodes[i].initialY + world->rng.randomFloat(-1.0f, 1.0f), -30.0f, 30.0f);
		nodes[i].x = nodes[i].initialX;
		nodes[i].y = nodes[i].initialY;
		nodes[i].xSpeed = 0.0f;
		nodes[i].ySpeed = 0.0f;
		nodes[i].mass = std::clamp(parent->nodes[i].mass + world->rng.randomFloat(-0.01f, 0.01f), 0.3f, 2.0f);
		nodes[i].friction = std::clamp(parent->nodes[i].friction + world->rng.randomFloat(-0.01f, 0.01f), minNodeFriction, maxNodeFriction);
	}
	for (int i = 0; i < muscleCount; ++i) {
		muscles[i].length1 = std::clamp(parent->muscles[i].length1 + world->rng.randomFloat(-10.0f, 10.0f), 0.0f, 200.0f);
		muscles[i].length2 = std::clamp(parent->muscles[i].length2 + world->rng.randomFloat(-10.0f, 10.0f), 0.0f, 200.0f);
		muscles[i].strength = std::clamp(parent->muscles[i].strength + world->rng.randomFloat(-0.01f, 0.01f), 0.8f, 2.0f);
	}
	tickCounter = world->ticksToSwitchMuscleStage;
}

void Creature::initialize()
{
	id = idCounter++;
	
	for (int i = 0; i < nodeCount; i++) {
		muscles[i].node1 = i;
		muscles[i].node2 = (i + 1) % nodeCount;
	}

	for (int i = 0; i < nodeCount; ++i) {
		nodes[i].initialX = world->rng.randomFloat(-50.0f, 50.0f);
		nodes[i].initialY = world->rng.randomFloat(-25.0f, 25.0f);
		nodes[i].x = nodes[i].initialX;
		nodes[i].y = nodes[i].initialY;
		nodes[i].mass = world->rng.randomFloat(0.5f, 0.5f);
		nodes[i].xSpeed = 0.0f;
		nodes[i].ySpeed = 0.0f;
		nodes[i].friction = world->rng.randomFloat(1.0f, 3.0f);
	}
	for (int i = 0; i < muscleCount; ++i) {
		muscles[i].length1 = world->rng.randomFloat(80.0f, 120.0f);
		muscles[i].length2 = world->rng.randomFloat(80.0f, 120.0f);
		muscles[i].strength = world->rng.randomFloat(0.8f, 1.2f);
	}
	tickCounter = world->ticksToSwitchMuscleStage;
}

void Creature::tick()
{
    // flip between the two rest lengths
    if (--tickCounter <= 0) {
        muscleStage = !muscleStage;
        tickCounter = world->ticksToSwitchMuscleStage;
    }

    constexpr float damping = 0.985f;
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
        float rest = muscleStage ? m.length1 : m.length2;

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
    }

    // integrate positions, apply damping & simple ground collision
    for (int i = 0; i < nodeCount; ++i) {
        Node& n = nodes[i];

		n.ySpeed += world->gravity * gravityMult;

        n.xSpeed *= damping;
        n.ySpeed *= damping;

        n.y += n.ySpeed;

        if (n.y > FLOOR_HEIGHT - n.mass * 10) {
            n.x += n.xSpeed * 1/n.friction;
            n.y = FLOOR_HEIGHT - n.mass * 10;
            n.ySpeed = 0.0f;
        }
        else {
            n.x += n.xSpeed;
        }
    }
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
	muscleStage = 0;
	tickCounter = world->ticksToSwitchMuscleStage;
}

void Creature::draw() {
	const float frictionVariation = (maxNodeFriction - minNodeFriction);
	
	for (int i = 0; i < nodeCount; ++i) {
		Node& n = nodes[i];
		Color nodeColor = { 50 + 205 - 205 * (n.friction - minNodeFriction) / frictionVariation, 255 - 255 * (n.friction - minNodeFriction) / frictionVariation , 255 - 255 * (n.friction - minNodeFriction) / frictionVariation , 255};
		DrawCircle(n.x,n.y,n.mass * 20, nodeColor);
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

int Creature::getMaxMuscleCountForNodeCount(int nodeCount)
{
	return (nodeCount * (nodeCount - 1)) / 2;
}