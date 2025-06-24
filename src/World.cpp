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

Creature* World::DrawWithCreatureCentered(int index, int generation) {
    
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
    DrawRectangleB(0, (4 * screenHeight / 5 / screenHeightRatio), screenWidth, screenHeight / 5, groundColor);

    camera.target = { (creature->getCenterX() - (screenWidth / 2) * 2.0f / guiScale) , ((float)-4 * screenHeight / 5) * 2.0f / guiScale + Creature::FLOOR_HEIGHT };
    camera.zoom = 0.5f * guiScale;
    BeginMode2D(camera);
    // draw rectangles 1 meter apart (100 pixels)
    auto atMeter = (int)creature->getCenterX() / 100;
	for (int i = screenWidth / -100; i < screenWidth / 100; ++i) {
        DrawRectangle(((int)creature->getCenterX() / 100) * 100 + i * 100, Creature::FLOOR_HEIGHT, 3, Creature::FLOOR_HEIGHT/2, WHITE);
		DrawTextCenteredNoScale(std::to_string(i+atMeter), ((int)creature->getCenterX() / 100) * 100 + i * 100 + 2, 3*Creature::FLOOR_HEIGHT/2 + 10, 2, WHITE);
	}
    creature->draw();
    EndMode2D();
	accumulatedTime += drawSpeedMult;
    while (accumulatedTime > 1) {
        creature->tick();
        accumulatedTime--;
    }
	return creature;
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

    viewGeneration = generation;
}