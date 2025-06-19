#include "World.h"
#include "Helper.h"
#include "Creature.h"
#include <algorithm>
#include <iostream>

void World::Draw(int x, int y, int width, int height) {
	DrawRectangleB(x, y, width, height, backgroundColor);
	DrawRectangleB(x, y + height - 10, width, 10, groundColor);
}

void World::DrawCentered(int x, int y, int width, int height) {
	DrawRectangleCentered(x, y, width, height, backgroundColor);
	DrawRectangleCentered(x, y + 4*height/5 / screenHeightRatio, width, height/5, groundColor);
}

void World::InitializeWorld() {
	for (int i = 0; i < numOfCreatures; ++i) {
		creatures[i].initialize();
	}
}

void World::DoGeneration()
{
	generation++;
	for (int i = 0; i < ticksPerSecond * secondsPerSimulation * numOfCreatures; i++) {
		creatures[i % numOfCreatures].tick();
	}

	std::sort(&creatures[0], &creatures[0] + numOfCreatures, [](Creature& a, Creature& b) {
		return a.getCenterX() < b.getCenterX();
	});

	std::cout << "Generation " << generation << " best creature : " << creatures[numOfCreatures - 1].id << " with center X : " << creatures[numOfCreatures - 1].getCenterX() << "\n";

	for (int i = 0; i < numOfCreatures / 2; i++) {
		creatures[numOfCreatures - 1 - i].reset();
	}
	for (int i = 0; i < numOfCreatures / 2; i++) {
		creatures[i] = creatures[numOfCreatures - 1 - i].reproduce();
	}
}

void World::DrawCreature() {
	if (IsKeyPressed(KEY_L)) {
		creatures[numOfCreatures - 1].reset();
	}
	accumulatedTime += drawSpeedMult;
	while (accumulatedTime > 1) {
		creatures[numOfCreatures - 1].tick();
		accumulatedTime -= 1;
	}

	creatures[numOfCreatures - 1].draw();
}