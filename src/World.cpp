#include "World.h"
#include "Helper.h"
#include "Creature.h"
#include <algorithm>
#include <iostream>
#include <thread>
#include <execution>
#include <unordered_set>
#include <chrono>
#include <cstring>
#include <limits>
#include "miniz.h"
#include "tinyfiledialogs.h"

namespace {
    constexpr char compressedWorldMagic[] = { 'E', 'W', 'C', 'M', 'P', 'R', 'S', '1' };

    void writeCompressedWorldFile(std::ostream& out, const std::string& uncompressedData)
    {
        if (uncompressedData.size() > std::numeric_limits<mz_ulong>::max()) {
            throw std::runtime_error("Save data is too large to compress");
        }

        mz_ulong compressedSize = mz_compressBound(static_cast<mz_ulong>(uncompressedData.size()));
        std::vector<unsigned char> compressedData(compressedSize);
        
        const int result = mz_compress2(
            compressedData.data(),
            &compressedSize,
            reinterpret_cast<const unsigned char*>(uncompressedData.data()),
            static_cast<mz_ulong>(uncompressedData.size()),
            MZ_BEST_COMPRESSION
        );

        if (result != MZ_OK) {
            throw std::runtime_error("Failed to compress save data");
        }

        const uint64_t uncompressedSize64 = static_cast<uint64_t>(uncompressedData.size());
        const uint64_t compressedSize64 = static_cast<uint64_t>(compressedSize);

        out.write(compressedWorldMagic, sizeof(compressedWorldMagic));
        writeValue(out, uncompressedSize64);
        writeValue(out, compressedSize64);
        out.write(reinterpret_cast<const char*>(compressedData.data()), compressedSize);
    }

    std::string readCompressedWorldFile(std::istream& in)
    {
        char magic[sizeof(compressedWorldMagic)];
        in.read(magic, sizeof(magic));
        if (!in || std::memcmp(magic, compressedWorldMagic, sizeof(compressedWorldMagic)) != 0) {
            throw std::runtime_error("Invalid compressed world file");
        }

        uint64_t uncompressedSize64;
        uint64_t compressedSize64;
        readValue(in, uncompressedSize64);
        readValue(in, compressedSize64);

        if (uncompressedSize64 > std::numeric_limits<mz_ulong>::max() ||
            compressedSize64 > std::numeric_limits<mz_ulong>::max()) {
            throw std::runtime_error("Compressed world file is too large");
        }

        std::vector<unsigned char> compressedData(static_cast<size_t>(compressedSize64));
        in.read(reinterpret_cast<char*>(compressedData.data()), compressedData.size());
        if (!in) {
            throw std::runtime_error("Failed to read compressed world data");
        }

        std::string uncompressedData(static_cast<size_t>(uncompressedSize64), '\0');
        mz_ulong uncompressedSize = static_cast<mz_ulong>(uncompressedSize64);

        const int result = mz_uncompress(
            reinterpret_cast<unsigned char*>(uncompressedData.data()),
            &uncompressedSize,
            compressedData.data(),
            static_cast<mz_ulong>(compressedData.size())
        );

        if (result != MZ_OK || uncompressedSize != uncompressedSize64) {
            throw std::runtime_error("Failed to decompress world data");
        }

        return uncompressedData;
    }
}

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
    
    int chunkSize = numOfCreatures / numberOfThreads;
    int leftover = numOfCreatures % numberOfThreads;
    int start = 0;

    for (int t = 0; t < numberOfThreads; ++t) {
        int count = chunkSize + (t < leftover ? 1 : 0);
        int begin = start;
        int end = begin + count;

        // Spawn the threads
        workers.emplace_back(&World::WorkerThread, this, begin, end);

        start = end;
    }
    // sleep for 1s
	std::this_thread::sleep_for(std::chrono::seconds(1));
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

void World::WorkerThread(int begin, int end)
{
    int localGen = generation; // Keep track of which generation this thread is on

    while (true) {
        std::unique_lock<std::mutex> lock(mtx);

        workerStart.wait(lock, [this, localGen] {
            return this->generation > localGen || terminate;
            });

        if (terminate) return;

        localGen = generation;
        int ticks = currentTicks;

        lock.unlock();

        for (int c = begin; c < end; ++c) {
            for (int t = 0; t < ticks; ++t) {
                creatures[c].tick();
            }
        }

        lock.lock();
        activeWorkers--;

        // If this is the last thread to finish, wake up the main thread
        if (activeWorkers == 0) {
            workerDone.notify_one();
        }
    }
}

void World::Save()
{
    // new file
    const char* filters[] = { "*.WORLD" };

    const char* path = tinyfd_saveFileDialog(
        "Save file as...",
        (worldName + ".WORLD").c_str(),
        1,
        filters,
        "World files"
    );

    if (!path) {
        return;
    }

    std::vector<Creature> vec(creatures.get(), creatures.get() + numOfCreatures);

    try {
        std::ostringstream serialized(std::ios::binary);

        writeValue(serialized, savefileVersion);
        writeString(serialized, worldName);
        writeValue(serialized, worldSeed);
        writeValue(serialized, gravity);
        writeValue(serialized, ticksPerSecond);
        writeValue(serialized, secondsPerSimulation);
        writeValue(serialized, numOfCreatures);
        writeValue(serialized, mutabilityRange);
        writeValue(serialized, mutabilityFactor);
        writeValue(serialized, backgroundColor);
        writeValue(serialized, groundColor);
	    writeValue(serialized, generation);
        writeCreatureVector(serialized, vec);
        writeCreatureVector(serialized, worstGenerationalCreatures);
        writeCreatureVector(serialized, averageGenerationalCreatures);
        writeCreatureVector(serialized, bestGenerationalCreatures);
	    writePercentileGraph(serialized, percentileGraph);

        std::ofstream out(path, std::ios::binary);
        if (!out) {
            throw std::runtime_error("Failed to create save file");
        }

        writeCompressedWorldFile(out, serialized.str());

        out.close();
        std::cout << "World saved to " << path << std::endl;
        notices.push_back({ "World saved: " + worldName, 3.0f });
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to save world: " << e.what() << std::endl;
        notices.push_back({ "Failed to save world: " + std::string(e.what()), 5.0f });
    }
}

bool World::Load()
{
    const char* filters[] = { "*.WORLD" };
    const char* path = tinyfd_openFileDialog(
        "Select a world file...",
        "",
        1,
        filters,
        "World files",
        0
    );
    if (!path) {
        return false;
    }
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        notices.push_back({ "Failed to open world file", 5.0f });
        return false;
    }
    
    try {
        std::string uncompressedData = readCompressedWorldFile(in);
        std::istringstream dataIn(uncompressedData, std::ios::binary);

        uint64_t fileVersion;
        readValue(dataIn, fileVersion);
        std::string worldName;
        uint32_t worldSeed;
        readString(dataIn, worldName);
        readValue(dataIn, worldSeed);
		std::cout << "Loading world: " << worldName << " with seed " << worldSeed << std::endl;
        if (fileVersion != savefileVersion) {
            std::cerr << "Unsupported savefile version: " << fileVersion << std::endl;
            notices.push_back({ "Expected savefile version " + std::to_string(savefileVersion) + ", got " + std::to_string(fileVersion), 5.0f });
            return false;
        }

        world = std::make_unique<World>();
		world->worldName = worldName;
		world->worldSeed = worldSeed;
        readValue(dataIn, world->gravity);
        readValue(dataIn, world->ticksPerSecond);
        readValue(dataIn, world->secondsPerSimulation);
        readValue(dataIn, world->numOfCreatures);
        readValue(dataIn, world->mutabilityRange);
        readValue(dataIn, world->mutabilityFactor);
        readValue(dataIn, world->backgroundColor);
        readValue(dataIn, world->groundColor);
        readValue(dataIn, world->generation);
        
        world->viewGeneration = world->generation;
        std::vector<Creature> vec;
        readCreatureVector(dataIn, vec);
        for (int i = 0; i < world->numOfCreatures; ++i) {
            world->creatures[i] = vec[i];
        }
        readCreatureVector(dataIn, world->worstGenerationalCreatures);
        readCreatureVector(dataIn, world->averageGenerationalCreatures);
        readCreatureVector(dataIn, world->bestGenerationalCreatures);
        readPercentileGraph(dataIn, world->percentileGraph);
        world->percentileGraph.world = world.get();
        in.close();

        std::cout << "World loaded from " << path << std::endl;
        notices.push_back({ "World loaded: " + world->worldName, 2.0f });
        return true;
    }
    catch (const std::exception& e) {
		std::cerr << "Failed to load world: " << e.what() << " (most likely corrupted)" << std::endl;
        notices.push_back({ "Failed to load world: " + std::string(e.what()) + " (most likely corrupted)", 5.0f });
        return false;
    }
}

void World::SaveBestCreature() {
    std::ofstream file("bestcreature", std::ios::binary);
    writeCreature(file, bestGenerationalCreatures[generation]);

    file.close();
}


void World::DoGeneration()
{
    generation++;
    
    std::unique_lock<std::mutex> lock(mtx);

    currentTicks = ticksPerSecond * secondsPerSimulation;
    activeWorkers = numberOfThreads;

    // Wake up all sleeping worker threads
    workerStart.notify_all();

    // Make the main thread sleep until activeWorkers hits 0
    workerDone.wait(lock, [this] { return activeWorkers == 0; });
    std::sort(&creatures[0], &creatures[0] + numOfCreatures, [](Creature& a, Creature& b) {
        return a.getCenterX() < b.getCenterX();
        });

	SendGenerationalDataToPercentileGraph();

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

    for (int j = 0; j < numOfCreatures / 2; j++) {
        float f = static_cast<float>(j) / numOfCreatures;
        float rand = (std::pow(RNG::randomFloat(-numOfCreatures, numOfCreatures) / numOfCreatures, 3.0f) + 1.0f) / 2.0f;

        int j2 = (f <= rand) ? j : numOfCreatures - 1 - j;

        creatures[j2] = creatures[numOfCreatures-j2-1].reproduce();
    }

    viewGeneration = generation;
}
