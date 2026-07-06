#include "Creature.h"
#include "RNG.h"
#include "Raylib.h"
#include "World.h"

#include <iostream>
#include <cmath>
#include <algorithm>
#include <limits>
#include <string>
#include <vector>
#include <numeric>

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

void Creature::initializeChild(Creature* parent)
{
    id = idCounter++;

	float mutatedNode = 1.0f; // used to increase the chance of muscle mutation if a node mutation occurs
    int childNodeCount = parent->nodeCount;
    if (RNG::randomFloat(0.0f, 1.0f) < world->structuralMutationChance * world->mutabilityFactor) {
        const int delta = RNG::biasedLowerInt(1, 10, 0.70f);
        childNodeCount += RNG::randomInt(0, 1) == 0 ? -delta : delta;
        mutatedNode = world->muscleMutationMultiplierWhenNodeMutated;
    }

    childNodeCount = std::clamp(childNodeCount, world->minNodes, world->maxNodes);

    const int physicalMinMuscles = childNodeCount; // mandatory ring
    const int physicalMaxMuscles = childNodeCount * (childNodeCount - 1) / 2;
    const int maxStorableMuscles = std::min(physicalMaxMuscles, (int)std::numeric_limits<uint8_t>::max());
    const int minMuscles = std::clamp(world->minMuscles, physicalMinMuscles, maxStorableMuscles);
    const int maxMuscles = std::clamp(world->maxMuscles, minMuscles, maxStorableMuscles);

    int childMuscleCount = parent->muscleCount;
    if (RNG::randomFloat(0.0f, 1.0f) < world->structuralMutationChance * world->mutabilityFactor * mutatedNode) {
        const int delta = RNG::biasedLowerInt(1, 10, 0.625f);
        childMuscleCount += RNG::randomInt(0, 1) == 0 ? -delta : delta;
    }

    childMuscleCount = std::clamp(childMuscleCount, minMuscles, maxMuscles);

    nodeCount = (uint8_t)childNodeCount;
    muscleCount = (uint8_t)childMuscleCount;

    nodes = std::make_unique<Node[]>(nodeCount);
    muscles = std::make_unique<Muscle[]>(muscleCount);

    const int parentNodeCount = parent->nodeCount;
    const int n = nodeCount;
    const int m = muscleCount;

    // Randomly choose surviving parent nodes.
    std::vector<char> removed(parentNodeCount, false);

    for (int removedCount = 0; removedCount < parentNodeCount - n; ) {
        int victim = RNG::randomInt(0, parentNodeCount - 1);
        if (!removed[victim]) {
            removed[victim] = true;
            ++removedCount;
        }
    }

    std::vector<int> sourceOld(n, -1);
    std::vector<int> oldToNew(parentNodeCount, -1);

    for (int oldIndex = 0, newIndex = 0; oldIndex < parentNodeCount && newIndex < n; ++oldIndex) {
        if (!removed[oldIndex]) {
            sourceOld[newIndex] = oldIndex;
            oldToNew[oldIndex] = newIndex;
            ++newIndex;
        }
    }

    auto makeRandomNode = [&]() {
        Node node{};
        node.initialX = RNG::randomFloat(minNodeInitialX, maxNodeInitialX);
        node.initialY = RNG::randomFloat(minNodeInitialY, maxNodeInitialY);
        node.mass = RNG::randomFloat(minNodeMass, maxNodeMass);
        node.friction = RNG::randomFloat(minNodeFriction, maxNodeFriction);
        node.x = node.initialX;
        node.y = node.initialY;
        return node;
        };

    auto mutateNode = [&](const Node& p) {
        Node node{};
        node.initialX = std::clamp(p.initialX + RNG::randomFloat(-nodeInitialXVariation, nodeInitialXVariation), minNodeInitialX, maxNodeInitialX);
        node.initialY = std::clamp(p.initialY + RNG::randomFloat(-nodeInitialYVariation, nodeInitialYVariation), minNodeInitialY, maxNodeInitialY);
        node.mass = std::clamp(p.mass + RNG::randomFloat(-nodeMassVariation, nodeMassVariation), minNodeMass, maxNodeMass);
        node.friction = std::clamp(p.friction + RNG::randomFloat(-nodeFrictionVariation, nodeFrictionVariation), minNodeFriction, maxNodeFriction);
        node.x = node.initialX;
        node.y = node.initialY;
        return node;
        };

    for (int i = 0; i < n; ++i) {
        nodes[i] = sourceOld[i] >= 0 ? mutateNode(parent->nodes[sourceOld[i]]) : makeRandomNode();
    }

    auto normalizeEdge = [](int& a, int& b) {
        if (a > b) std::swap(a, b);
        };

    auto edgeIndex = [&](int a, int b) {
        normalizeEdge(a, b);
        return a * n + b;
        };

    auto parentEdgeIndex = [&](int a, int b) {
        normalizeEdge(a, b);
        return a * parentNodeCount + b;
        };

    std::vector<const Muscle*> parentEdge(parentNodeCount * parentNodeCount, nullptr);

    for (int i = 0; i < parent->muscleCount; ++i) {
        int a = parent->muscles[i].node1;
        int b = parent->muscles[i].node2;
        parentEdge[parentEdgeIndex(a, b)] = &parent->muscles[i];
    }

    auto parentMuscleForNewEdge = [&](int a, int b) -> const Muscle* {
        int oldA = sourceOld[a];
        int oldB = sourceOld[b];

        if (oldA < 0 || oldB < 0) {
            return nullptr;
        }

        return parentEdge[parentEdgeIndex(oldA, oldB)];
        };

    auto makeMuscle = [&](const Muscle* source, int a, int b) {
        normalizeEdge(a, b);

        Muscle muscle{};
        muscle.node1 = (uint8_t)a;
        muscle.node2 = (uint8_t)b;

        if (source) {
            muscle.state1Ticks = std::clamp(source->state1Ticks + RNG::randomInt(-muscleStateTicksVariation, muscleStateTicksVariation), (int)minMuscleStateTicks, (int)maxMuscleStateTicks);
            muscle.state2Ticks = std::clamp(source->state2Ticks + RNG::randomInt(-muscleStateTicksVariation, muscleStateTicksVariation), (int)minMuscleStateTicks, (int)maxMuscleStateTicks);
            muscle.length1 = std::clamp(source->length1 + RNG::randomFloat(-muscleLengthVariation, muscleLengthVariation), minMuscleLength, maxMuscleLength);
            muscle.length2 = std::clamp(source->length2 + RNG::randomFloat(-muscleLengthVariation, muscleLengthVariation), minMuscleLength, maxMuscleLength);
            muscle.strength = std::clamp(source->strength + RNG::randomFloat(-muscleStrengthVariation, muscleStrengthVariation), minMuscleStrength, maxMuscleStrength);
        }
        else {
            muscle.state1Ticks = RNG::randomInt((int)minMuscleStateTicks, (int)maxMuscleStateTicks);
            muscle.state2Ticks = RNG::randomInt((int)minMuscleStateTicks, (int)maxMuscleStateTicks);
            muscle.length1 = RNG::randomFloat(minMuscleLength, maxMuscleLength);
            muscle.length2 = RNG::randomFloat(minMuscleLength, maxMuscleLength);
            muscle.strength = RNG::randomFloat(minMuscleStrength, maxMuscleStrength);
        }

        muscle.currentMuscleStage = 0;
        muscle.muscleTickCounter = 0;

        return muscle;
        };

    auto isRingEdge = [&](int a, int b) {
        normalizeEdge(a, b);
        return b == a + 1 || (a == 0 && b == n - 1);
        };

    std::vector<char> used(n * n, false);
    std::vector<Muscle> childMuscles;
    childMuscles.reserve(m);

    auto addMuscle = [&](Muscle muscle) {
        used[edgeIndex(muscle.node1, muscle.node2)] = true;
        childMuscles.push_back(muscle);
        };

    // Mandatory full ring.
    for (int i = 0; i < n; ++i) {
        int a = i;
        int b = (i + 1) % n;
        addMuscle(makeMuscle(parentMuscleForNewEdge(a, b), a, b));
    }

    struct Candidate {
        int a;
        int b;
        const Muscle* source;
    };

    std::vector<Candidate> inheritedExtras;

    for (int i = 0; i < parent->muscleCount; ++i) {
        int oldA = parent->muscles[i].node1;
        int oldB = parent->muscles[i].node2;

        int a = oldToNew[oldA];
        int b = oldToNew[oldB];

        if (a < 0 || b < 0 || a == b) {
            continue;
        }

        normalizeEdge(a, b);

        if (!used[edgeIndex(a, b)] && !isRingEdge(a, b)) {
            inheritedExtras.push_back({ a, b, &parent->muscles[i] });
        }
    }

    // Randomly preserve parent extra muscles until target count is reached.
    while ((int)childMuscles.size() < m && !inheritedExtras.empty()) {
        int index = RNG::randomInt(0, (int)inheritedExtras.size() - 1);
        Candidate candidate = inheritedExtras[index];

        addMuscle(makeMuscle(candidate.source, candidate.a, candidate.b));

        inheritedExtras[index] = inheritedExtras.back();
        inheritedExtras.pop_back();
    }

    // Fill remaining slots with completely random new muscles.
    std::vector<std::pair<int, int>> freeEdges;

    for (int a = 0; a < n; ++a) {
        for (int b = a + 1; b < n; ++b) {
            if (!used[edgeIndex(a, b)]) {
                freeEdges.emplace_back(a, b);
            }
        }
    }

    while ((int)childMuscles.size() < m) {
        int index = RNG::randomInt(0, (int)freeEdges.size() - 1);
        auto [a, b] = freeEdges[index];

        addMuscle(makeMuscle(nullptr, a, b));

        freeEdges[index] = freeEdges.back();
        freeEdges.pop_back();
    }

    for (int i = 0; i < m; ++i) {
        muscles[i] = childMuscles[i];
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

	nodeCount = RNG::biasedLowerInt(world->minNodes, world->maxNodes, 0.4f);
	int nodeMaxMuscles = nodeCount * (nodeCount - 1) / 2;
	int minMuscles = std::clamp(world->minMuscles, (int)nodeCount, nodeMaxMuscles);
	int maxMuscles = std::clamp(world->maxMuscles, minMuscles, nodeMaxMuscles);
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
