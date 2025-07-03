#include "World.h"
#include "Helper.h"
#include "Creature.h"
#include <algorithm>
#include <iostream>
#include <thread>
#include <execution>
#include <unordered_set>

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

void World::SendGenerationalDataToPercentileGraph()
{
	percentileGraph.data[0].push_back(creatures[0].getCenterX() / 100.0f); // worst creature
	percentileGraph.data[1].push_back(creatures[(int)(0.01f * (numOfCreatures - 1))].getCenterX() / 100.0f);
	percentileGraph.data[2].push_back(creatures[(int)(0.02f * (numOfCreatures - 1))].getCenterX() / 100.0f);


	percentileGraph.data[3].push_back(creatures[(int)(0.05f * (numOfCreatures - 1))].getCenterX() / 100.0f);

	percentileGraph.data[4].push_back(creatures[(int)(0.10f * (numOfCreatures - 1))].getCenterX() / 100.0f); // 10th percentile

	percentileGraph.data[5].push_back(creatures[(int)(0.20f * (numOfCreatures - 1))].getCenterX() / 100.0f); // 20th percentile
	percentileGraph.data[6].push_back(creatures[(int)(0.35f * (numOfCreatures - 1))].getCenterX() / 100.0f); // 35th percentile
	percentileGraph.data[7].push_back(creatures[(int)(0.50f * (numOfCreatures - 1))].getCenterX() / 100.0f); // 50th percentile (median)
	percentileGraph.data[8].push_back(creatures[(int)(0.65f * (numOfCreatures - 1))].getCenterX() / 100.0f); // 65th percentile
	percentileGraph.data[9].push_back(creatures[(int)(0.80f * (numOfCreatures - 1))].getCenterX() / 100.0f); // 80th percentile
	percentileGraph.data[10].push_back(creatures[(int)(0.90f * (numOfCreatures - 1))].getCenterX() / 100.0f); // 90th percentile

	percentileGraph.data[11].push_back(creatures[(int)(0.95f * (numOfCreatures - 1))].getCenterX() / 100.0f); // 95th percentile

	percentileGraph.data[12].push_back(creatures[(int)(0.98f * (numOfCreatures - 1))].getCenterX() / 100.0f); // 98th percentile
	percentileGraph.data[13].push_back(creatures[(int)(0.99f * (numOfCreatures - 1))].getCenterX() / 100.0f); // 99th percentile
	percentileGraph.data[14].push_back(creatures[numOfCreatures - 1].getCenterX() / 100.0f); // best creature

    percentileGraph.updateExtremeValues();
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

	SendGenerationalDataToPercentileGraph();

    /*for (int i = 0; i < numOfCreatures; ++i) {
		creatures[i].fitness = creatures[i].getCenterX();
    }*/

	creatures[0].fitness = creatures[0].getCenterX();
	creatures[numOfCreatures / 2].fitness = creatures[numOfCreatures / 2].getCenterX();
	creatures[numOfCreatures - 1].fitness = creatures[numOfCreatures - 1].getCenterX();

    // now fill up 

    for (int i = 0; i < numOfCreatures; ++i) {
        creatures[numOfCreatures - 1 - i].reset();
    }

	worstGenerationalCreatures.push_back(creatures[0]);
	averageGenerationalCreatures.push_back(creatures[numOfCreatures / 2]);
	bestGenerationalCreatures.push_back(creatures[numOfCreatures - 1]);

    //for (int i = 0; i < numOfCreatures / 2; ++i) {
    //    creatures[i] = creatures[numOfCreatures - 1 - i].reproduce();
    // }
    
    for (int j = 0; j < numOfCreatures / 2; j++) {
        float f = static_cast<float>(j) / numOfCreatures;
        float rand = (std::pow(rng.randomFloat(-numOfCreatures, numOfCreatures) / numOfCreatures, 3.0f) + 1.0f) / 2.0f;

        int j2 = (f <= rand) ? j : numOfCreatures - 1 - j;

        creatures[j2] = creatures[numOfCreatures-j2-1].reproduce();
    }

    viewGeneration = generation;
}