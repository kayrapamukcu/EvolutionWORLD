#include "World.h"
#include "Helper.h"
#include "Creature.h"
#include <algorithm>
#include <iostream>
#include <thread>
#include <execution>

void World::Draw(int x, int y, int width, int height) {
	DrawRectangleB(x, y, width, height, backgroundColor);
	DrawRectangleB(x, y + height - 10, width, 10, groundColor);
}

void World::DrawCentered(int x, int y, int width, int height) {
	DrawRectangleCentered(x, y, width, height, backgroundColor);
    DrawRectangleCentered(x, y + (4 * height / 5 / screenHeightRatio) * (guiScale) / 2, width, height / 5, groundColor);
}

void World::DrawWithCreatureCentered(int index, int generation) {
    
    Creature* creature;
    switch (index) {
    case 0:
        creature = &worstGenerationalCreatures[generation];
        break;
    case 1:
        creature = &averageGenerationalCreatures[generation];
        break;
    case 2:
        creature = &bestGenerationalCreatures[generation];
        break;
    }
    camera.offset = { creature->getCenterX(), 0 };
    BeginMode2D(camera);
    creature->draw();
    EndMode2D();
	creature->tick();
}

void World::InitializeWorld() {
	for (int i = 0; i < numOfCreatures; ++i) {
		creatures[i].initialize();
	}
    DoGeneration();
}

void World::DoGeneration()
{
    generation++;

    int totalTicks = ticksPerSecond * secondsPerSimulation;

    // Divide creatures into chunks for each thread
    std::vector<std::thread> threads;
    int chunkSize = numOfCreatures / numberOfThreads;
    int leftover = numOfCreatures % numberOfThreads;

    int start = 0;
    for (int t = 0; t < numberOfThreads; ++t) {
        int count = chunkSize + (t < leftover ? 1 : 0); // distribute leftovers
        int begin = start;
        int end = begin + count;

        threads.emplace_back([this, begin, end, totalTicks]() {
            for (int c = begin; c < end; ++c) {
                for (int t = 0; t < totalTicks; ++t) {
                    creatures[c].tick();
                }
            }
            });

        start = end;
    }

    for (auto& th : threads) th.join();

    std::sort(&creatures[0], &creatures[0] + numOfCreatures, [](Creature& a, Creature& b) {
        return a.getCenterX() < b.getCenterX();
        });

    // std::cout << "Generation " << generation << " best creature : " << creatures[numOfCreatures - 1].id << " with center X : " << creatures[numOfCreatures - 1].getCenterX() << "\n";

	creatures[0].fitness = creatures[0].getCenterX();
	creatures[numOfCreatures / 2].fitness = creatures[numOfCreatures / 2].getCenterX();
	creatures[numOfCreatures - 1].fitness = creatures[numOfCreatures - 1].getCenterX();

    for (int i = 0; i < numOfCreatures / 2; ++i) {
        creatures[numOfCreatures - 1 - i].reset();
    }

    creatures[0].reset();

	worstGenerationalCreatures.push_back(creatures[0]);
	averageGenerationalCreatures.push_back(creatures[numOfCreatures / 2]);
	bestGenerationalCreatures.push_back(creatures[numOfCreatures - 1]);

    for (int i = 0; i < numOfCreatures / 2; ++i) {
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