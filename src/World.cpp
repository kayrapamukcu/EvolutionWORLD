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
#include <utility>
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
	DrawRectUI(x, y, width, height, backgroundColor);
	DrawRectUI(x, y + height - 10, width, 10, groundColor);
}

int World::GetHistoryIndexForGeneration(int generation) const
{
    if (storedHistoryGenerations.empty()) {
        return 0;
    }

    auto it = std::lower_bound(storedHistoryGenerations.begin(), storedHistoryGenerations.end(), generation);
    if (it == storedHistoryGenerations.end()) {
        return (int)storedHistoryGenerations.size() - 1;
    }
    if (it == storedHistoryGenerations.begin()) {
        return 0;
    }
    int rightIndex = (int)std::distance(storedHistoryGenerations.begin(), it);
    int leftIndex = rightIndex - 1;
    return std::abs(storedHistoryGenerations[rightIndex] - generation) < std::abs(generation - storedHistoryGenerations[leftIndex])
        ? rightIndex
        : leftIndex;
}

int World::GetFirstHistoryGeneration() const
{
    return storedHistoryGenerations.empty() ? firstStoredGeneration : storedHistoryGenerations.front();
}

int World::GetLastHistoryGeneration() const
{
    return storedHistoryGenerations.empty() ? firstStoredGeneration : storedHistoryGenerations.back();
}

bool World::HasHistoryDataForGeneration(int generation) const
{
    return std::binary_search(storedHistoryGenerations.begin(), storedHistoryGenerations.end(), generation);
}

bool World::HasHistoryDataInRange(int startGeneration, int endGeneration) const
{
    if (storedHistoryGenerations.empty() || endGeneration < startGeneration) {
        return false;
    }

    auto it = std::lower_bound(storedHistoryGenerations.begin(), storedHistoryGenerations.end(), startGeneration);
    return it != storedHistoryGenerations.end() && *it <= endGeneration;
}

Creature* World::DrawWithCreatureCentered(int index, int generation) {
    
	Creature* creature;
    int storedGenerationIndex = GetHistoryIndexForGeneration(generation);
    switch (index) {
    case 0:
        creature = &worstGenerationalCreatures[storedGenerationIndex];
        break;
    case 1:
        creature = &averageGenerationalCreatures[storedGenerationIndex];
        break;
    case 2:
        creature = &bestGenerationalCreatures[storedGenerationIndex];
        break;
    default:
        creature = &averageGenerationalCreatures[storedGenerationIndex];
        break;
    }
    constexpr float groundTopRatio = 0.75f;
    DrawRectUI(0, absoluteHeight * groundTopRatio, absoluteWidth, absoluteHeight * (1.0f - groundTopRatio), groundColor);

	const Vector2 groundScreen = UIToScreen(absoluteWidth / 2.0f, absoluteHeight * groundTopRatio);
	camera.offset = groundScreen;
    camera.target = { creature->getCenterX(), (float)Creature::FLOOR_HEIGHT };
    camera.zoom = 1.0f * UIScale();
    BeginMode2D(camera);
    // draw rectangles 1 meter apart (100 pixels)
    auto atMeter = (int)creature->getCenterX() / 100;
	int meterRange = (int)(screenWidth / std::max(1.0f, camera.zoom) / 100.0f) + 2;
	for (int i = -meterRange; i <= meterRange; ++i) {
        DrawRectangle(((int)creature->getCenterX() / 100) * 100 + i * 100, Creature::FLOOR_HEIGHT, 3, Creature::FLOOR_HEIGHT/3, WHITE);
		std::string meterLabel = std::to_string(i + atMeter);
		const float labelFontSize = defaultFont.baseSize * 0.75f;
		const float labelSpacing = 0.75f;
		Vector2 labelSize = MeasureTextEx(defaultFont, meterLabel.c_str(), labelFontSize, labelSpacing);
		DrawTextEx(defaultFont, meterLabel.c_str(), { ((int)creature->getCenterX() / 100) * 100 + i * 100 + 2 - labelSize.x * 0.5f, 3 * Creature::FLOOR_HEIGHT / 2 + 10 - labelSize.y * 0.5f }, labelFontSize, labelSpacing, WHITE);
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

Creature* World::DrawCreatureCentered(Creature& creature) {
    constexpr float groundTopRatio = 0.75f;
    DrawRectUI(0, absoluteHeight * groundTopRatio, absoluteWidth, absoluteHeight * (1.0f - groundTopRatio), groundColor);

    const Vector2 groundScreen = UIToScreen(absoluteWidth / 2.0f, absoluteHeight * groundTopRatio);
    camera.offset = groundScreen;
    camera.target = { creature.getCenterX(), (float)Creature::FLOOR_HEIGHT };
    camera.zoom = 1.0f * UIScale();
    BeginMode2D(camera);

    auto atMeter = (int)creature.getCenterX() / 100;
    int meterRange = (int)(screenWidth / std::max(1.0f, camera.zoom) / 100.0f) + 2;
    for (int i = -meterRange; i <= meterRange; ++i) {
        DrawRectangle(((int)creature.getCenterX() / 100) * 100 + i * 100, Creature::FLOOR_HEIGHT, 3, Creature::FLOOR_HEIGHT / 3, WHITE);
        std::string meterLabel = std::to_string(i + atMeter);
        const float labelFontSize = defaultFont.baseSize * 0.75f;
        const float labelSpacing = 0.75f;
        Vector2 labelSize = MeasureTextEx(defaultFont, meterLabel.c_str(), labelFontSize, labelSpacing);
        DrawTextEx(defaultFont, meterLabel.c_str(), { ((int)creature.getCenterX() / 100) * 100 + i * 100 + 2 - labelSize.x * 0.5f, 3 * Creature::FLOOR_HEIGHT / 2 + 10 - labelSize.y * 0.5f }, labelFontSize, labelSpacing, WHITE);
    }

    creature.draw();
    EndMode2D();
    accumulatedTime += drawSpeedMult;
    while (accumulatedTime > 1) {
        creature.tick();
        accumulatedTime--;
    }
    return &creature;
}

Creature* World::DrawCurrentCreatureCentered(int index) {
    index = std::clamp(index, 0, numOfCreatures - 1);
    Creature* creature = &creatures[index];
    DrawCreatureCentered(*creature);
    return creature;
}

void World::InitializeWorld() {
	for (int i = 0; i < numOfCreatures; ++i) {
		creatures[i].initialize();
	}

    StartWorkerThreads();
    DoGeneration();
}

void World::StartWorkerThreads() {
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
}

void World::SendGenerationalDataToPercentileGraph()
{
    percentileGraph.storedGenerations.push_back(generation);
	percentileGraph.data[0].push_back(creatures[0].fitness / 100.0f); // worst creature
	percentileGraph.data[1].push_back(creatures[(int)(0.01f * (numOfCreatures - 1))].fitness / 100.0f);
	percentileGraph.data[2].push_back(creatures[(int)(0.02f * (numOfCreatures - 1))].fitness / 100.0f);


	percentileGraph.data[3].push_back(creatures[(int)(0.05f * (numOfCreatures - 1))].fitness / 100.0f);

	percentileGraph.data[4].push_back(creatures[(int)(0.10f * (numOfCreatures - 1))].fitness / 100.0f); // 10th percentile

	percentileGraph.data[5].push_back(creatures[(int)(0.20f * (numOfCreatures - 1))].fitness / 100.0f); // 20th percentile
	percentileGraph.data[6].push_back(creatures[(int)(0.35f * (numOfCreatures - 1))].fitness / 100.0f); // 35th percentile
	percentileGraph.data[7].push_back(creatures[(int)(0.50f * (numOfCreatures - 1))].fitness / 100.0f); // 50th percentile (median)
	percentileGraph.data[8].push_back(creatures[(int)(0.65f * (numOfCreatures - 1))].fitness / 100.0f); // 65th percentile
	percentileGraph.data[9].push_back(creatures[(int)(0.80f * (numOfCreatures - 1))].fitness / 100.0f); // 80th percentile
	percentileGraph.data[10].push_back(creatures[(int)(0.90f * (numOfCreatures - 1))].fitness / 100.0f); // 90th percentile

	percentileGraph.data[11].push_back(creatures[(int)(0.95f * (numOfCreatures - 1))].fitness / 100.0f); // 95th percentile

	percentileGraph.data[12].push_back(creatures[(int)(0.98f * (numOfCreatures - 1))].fitness / 100.0f); // 98th percentile
	percentileGraph.data[13].push_back(creatures[(int)(0.99f * (numOfCreatures - 1))].fitness / 100.0f); // 99th percentile
	percentileGraph.data[14].push_back(creatures[numOfCreatures - 1].fitness / 100.0f); // best creature

    percentileGraph.updateExtremeValues();
    speciesGraph.addGeneration(generation, creatures.get(), numOfCreatures);
}

void World::WorkerThread(int begin, int end)
{
    int localGen;
    {
        std::unique_lock<std::mutex> lock(mtx);
        localGen = generation - 1; // Process an already-started generation if this worker starts late.
    }

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

static std::vector<int> CreateContiguousGenerations(int firstGeneration, int count)
{
    std::vector<int> generations;
    generations.reserve(std::max(0, count));
    for (int i = 0; i < count; ++i) {
        generations.push_back(firstGeneration + i);
    }
    return generations;
}

static std::pair<int, int> GetGenerationSliceIndices(const std::vector<int>& generations, int startGeneration, int endGeneration)
{
    if (generations.empty() || endGeneration < startGeneration) {
        return { 0, -1 };
    }

    auto firstIt = std::lower_bound(generations.begin(), generations.end(), startGeneration);
    auto lastIt = std::upper_bound(generations.begin(), generations.end(), endGeneration);
    if (firstIt == generations.end() || firstIt == lastIt) {
        return { 0, -1 };
    }

    int firstIndex = (int)std::distance(generations.begin(), firstIt);
    int lastIndex = (int)std::distance(generations.begin(), lastIt) - 1;
    return { firstIndex, lastIndex };
}

static std::vector<int> SliceGenerationHistory(const std::vector<int>& source, int startGeneration, int endGeneration)
{
    auto [firstIndex, lastIndex] = GetGenerationSliceIndices(source, startGeneration, endGeneration);
    if (lastIndex < firstIndex) {
        return {};
    }
    return std::vector<int>(source.begin() + firstIndex, source.begin() + lastIndex + 1);
}

static std::vector<Creature> SliceCreatureHistory(const std::vector<Creature>& source, const std::vector<int>& generations, int startGeneration, int endGeneration)
{
    std::vector<Creature> result;
    if (source.empty() || source.size() != generations.size()) {
        return result;
    }

    auto [firstIndex, lastIndex] = GetGenerationSliceIndices(generations, startGeneration, endGeneration);
    if (lastIndex < firstIndex) {
        return result;
    }

    result.assign(source.begin() + firstIndex, source.begin() + lastIndex + 1);
    return result;
}

static PercentileGraph SlicePercentileGraph(const PercentileGraph& source, int startGeneration, int endGeneration, bool includeGraph)
{
    PercentileGraph result = source;
    for (auto& vec : result.data) {
        vec.clear();
    }
    result.storedGenerations.clear();
    result.firstGeneration = startGeneration;
    result.minValue = -1.0f;
    result.maxValue = 1.0f;

    if (!includeGraph || source.data[0].empty() || source.storedGenerations.size() != source.data[0].size()) {
        return result;
    }

    auto [firstIndex, lastIndex] = GetGenerationSliceIndices(source.storedGenerations, startGeneration, endGeneration);
    if (lastIndex < firstIndex) {
        return result;
    }

    result.storedGenerations.assign(source.storedGenerations.begin() + firstIndex, source.storedGenerations.begin() + lastIndex + 1);
    result.firstGeneration = result.storedGenerations.empty() ? startGeneration : result.storedGenerations.front();
    for (int i = 0; i < result.data.size(); ++i) {
        result.data[i].assign(source.data[i].begin() + firstIndex, source.data[i].begin() + lastIndex + 1);
    }
    for (const auto& vec : result.data) {
        for (float value : vec) {
            result.minValue = std::min(result.minValue, value);
            result.maxValue = std::max(result.maxValue, value);
        }
    }
    result.minValue *= 1.05f;
    result.maxValue *= 1.05f;
    if (result.minValue == result.maxValue) {
        result.minValue -= 1.0f;
        result.maxValue += 1.0f;
    }
    return result;
}

static SpeciesGraph SliceSpeciesGraph(const SpeciesGraph& source, int startGeneration, int endGeneration, bool includeGraph)
{
    SpeciesGraph result = source;
    result.data.clear();
    result.storedGenerations.clear();
    result.firstGeneration = startGeneration;

    if (!includeGraph || source.data.empty() || source.storedGenerations.size() != source.data.size()) {
        return result;
    }

    auto [firstIndex, lastIndex] = GetGenerationSliceIndices(source.storedGenerations, startGeneration, endGeneration);
    if (lastIndex < firstIndex) {
        return result;
    }

    result.storedGenerations.assign(source.storedGenerations.begin() + firstIndex, source.storedGenerations.begin() + lastIndex + 1);
    result.data.assign(source.data.begin() + firstIndex, source.data.begin() + lastIndex + 1);
    result.firstGeneration = result.storedGenerations.empty() ? startGeneration : result.storedGenerations.front();
    return result;
}

void World::Save(std::atomic<bool>* workStarted, int saveStartGeneration, int saveEndGeneration, bool savePercentileGraph, bool saveSpeciesGraph)
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

    if (workStarted) {
        workStarted->store(true);
    }

    std::string savePath = path;
    std::string worldNameSnapshot;
    uint32_t worldSeedSnapshot;
    int gravitySnapshot;
    int ticksPerSecondSnapshot;
    int secondsPerSimulationSnapshot;
    int numOfCreaturesSnapshot;
    int minNodesSnapshot;
    int maxNodesSnapshot;
    int minMusclesSnapshot;
    int maxMusclesSnapshot;
    float mutabilityRangeSnapshot;
    float mutabilityFactorSnapshot;
    Color backgroundColorSnapshot;
    Color groundColorSnapshot;
    int generationSnapshot;
    int firstStoredGenerationSnapshot;
    int historyStartGenerationSnapshot;
    int historyEndGenerationSnapshot;
    std::vector<int> storedHistoryGenerationsSnapshot;
    std::vector<int> savedHistoryGenerationsSnapshot;
    std::vector<Creature> creaturesSnapshot;
    std::vector<Creature> worstGenerationalCreaturesSnapshot;
    std::vector<Creature> averageGenerationalCreaturesSnapshot;
    std::vector<Creature> bestGenerationalCreaturesSnapshot;
    PercentileGraph percentileGraphSnapshot(610, 70, 360, 340);
    SpeciesGraph speciesGraphSnapshot(610, 430, 360, 120);

    {
        std::lock_guard<std::mutex> dataLock(dataMutex);
        worldNameSnapshot = worldName;
        worldSeedSnapshot = worldSeed;
        gravitySnapshot = gravity;
        ticksPerSecondSnapshot = ticksPerSecond;
        secondsPerSimulationSnapshot = secondsPerSimulation;
        numOfCreaturesSnapshot = numOfCreatures;
        minNodesSnapshot = minNodes;
        maxNodesSnapshot = maxNodes;
        minMusclesSnapshot = minMuscles;
        maxMusclesSnapshot = maxMuscles;
        mutabilityRangeSnapshot = mutabilityRange;
        mutabilityFactorSnapshot = mutabilityFactor;
        backgroundColorSnapshot = backgroundColor;
        groundColorSnapshot = groundColor;
        generationSnapshot = generation;
        firstStoredGenerationSnapshot = firstStoredGeneration;
        if (storedHistoryGenerations.size() != worstGenerationalCreatures.size()) {
            storedHistoryGenerations = CreateContiguousGenerations(firstStoredGeneration, (int)worstGenerationalCreatures.size());
        }
        storedHistoryGenerationsSnapshot = storedHistoryGenerations;
        int lastStoredGenerationSnapshot = storedHistoryGenerationsSnapshot.empty() ? firstStoredGenerationSnapshot : storedHistoryGenerationsSnapshot.back();
        int firstStoredGenerationInHistorySnapshot = storedHistoryGenerationsSnapshot.empty() ? firstStoredGenerationSnapshot : storedHistoryGenerationsSnapshot.front();
        historyStartGenerationSnapshot = saveStartGeneration < 0 ? firstStoredGenerationSnapshot : saveStartGeneration;
        historyEndGenerationSnapshot = saveEndGeneration < 0 ? lastStoredGenerationSnapshot : saveEndGeneration;
        historyStartGenerationSnapshot = std::clamp(historyStartGenerationSnapshot, firstStoredGenerationInHistorySnapshot, lastStoredGenerationSnapshot);
        historyEndGenerationSnapshot = std::clamp(historyEndGenerationSnapshot, historyStartGenerationSnapshot, lastStoredGenerationSnapshot);
        savedHistoryGenerationsSnapshot = SliceGenerationHistory(storedHistoryGenerationsSnapshot, historyStartGenerationSnapshot, historyEndGenerationSnapshot);
        if (!savedHistoryGenerationsSnapshot.empty()) {
            historyStartGenerationSnapshot = savedHistoryGenerationsSnapshot.front();
        }
        creaturesSnapshot.assign(creatures.get(), creatures.get() + numOfCreatures);
        worstGenerationalCreaturesSnapshot = SliceCreatureHistory(worstGenerationalCreatures, storedHistoryGenerationsSnapshot, historyStartGenerationSnapshot, historyEndGenerationSnapshot);
        averageGenerationalCreaturesSnapshot = SliceCreatureHistory(averageGenerationalCreatures, storedHistoryGenerationsSnapshot, historyStartGenerationSnapshot, historyEndGenerationSnapshot);
        bestGenerationalCreaturesSnapshot = SliceCreatureHistory(bestGenerationalCreatures, storedHistoryGenerationsSnapshot, historyStartGenerationSnapshot, historyEndGenerationSnapshot);
        percentileGraphSnapshot = SlicePercentileGraph(percentileGraph, historyStartGenerationSnapshot, historyEndGenerationSnapshot, savePercentileGraph);
        speciesGraphSnapshot = SliceSpeciesGraph(speciesGraph, historyStartGenerationSnapshot, historyEndGenerationSnapshot, saveSpeciesGraph);
    }

    try {
        std::ostringstream serialized(std::ios::binary);

        writeValue(serialized, savefileVersion);
        writeString(serialized, worldNameSnapshot);
        writeValue(serialized, worldSeedSnapshot);
        writeValue(serialized, gravitySnapshot);
        writeValue(serialized, ticksPerSecondSnapshot);
        writeValue(serialized, secondsPerSimulationSnapshot);
        writeValue(serialized, numOfCreaturesSnapshot);
        writeValue(serialized, minNodesSnapshot);
        writeValue(serialized, maxNodesSnapshot);
        writeValue(serialized, minMusclesSnapshot);
        writeValue(serialized, maxMusclesSnapshot);
        writeValue(serialized, mutabilityRangeSnapshot);
        writeValue(serialized, mutabilityFactorSnapshot);
        writeValue(serialized, backgroundColorSnapshot);
        writeValue(serialized, groundColorSnapshot);
	    writeValue(serialized, generationSnapshot);
        writeValue(serialized, historyStartGenerationSnapshot);
        writeVector(serialized, savedHistoryGenerationsSnapshot);
        writeCreatureVector(serialized, creaturesSnapshot);
        writeCreatureVector(serialized, worstGenerationalCreaturesSnapshot);
        writeCreatureVector(serialized, averageGenerationalCreaturesSnapshot);
        writeCreatureVector(serialized, bestGenerationalCreaturesSnapshot);
	    writePercentileGraph(serialized, percentileGraphSnapshot);
        writeSpeciesGraph(serialized, speciesGraphSnapshot);

        std::ofstream out(savePath, std::ios::binary);
        if (!out) {
            throw std::runtime_error("Failed to create save file");
        }

        writeCompressedWorldFile(out, serialized.str());

        out.close();
        std::cout << "World saved to " << savePath << std::endl;
        PushNotice("World saved: " + worldNameSnapshot, 3.0f);
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to save world: " << e.what() << std::endl;
        PushNotice("Failed to save world: " + std::string(e.what()), 5.0f);
    }
}

bool World::Load()
{
    std::unique_ptr<World> loadedWorld = LoadFromDialog();
    if (!loadedWorld) {
        return false;
    }

    world = std::move(loadedWorld);
    PushNotice("World loaded: " + world->worldName, 2.0f);
    return true;
}

std::unique_ptr<World> World::LoadFromDialog(std::atomic<bool>* workStarted)
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
        return nullptr;
    }
    if (workStarted) {
        workStarted->store(true);
    }
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        PushNotice("Failed to open world file", 5.0f);
        return nullptr;
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
        if (fileVersion < 1 || fileVersion > savefileVersion) {
            std::cerr << "Unsupported savefile version: " << fileVersion << std::endl;
            PushNotice("Unsupported savefile version " + std::to_string(fileVersion), 5.0f);
            return nullptr;
        }

        auto loadedWorld = std::make_unique<World>(false);
		loadedWorld->worldName = worldName;
		loadedWorld->worldSeed = worldSeed;
        readValue(dataIn, loadedWorld->gravity);
        readValue(dataIn, loadedWorld->ticksPerSecond);
        readValue(dataIn, loadedWorld->secondsPerSimulation);
        readValue(dataIn, loadedWorld->numOfCreatures);
        if (fileVersion >= 4) {
            readValue(dataIn, loadedWorld->minNodes);
            readValue(dataIn, loadedWorld->maxNodes);
            readValue(dataIn, loadedWorld->minMuscles);
            readValue(dataIn, loadedWorld->maxMuscles);
        }
        else {
            loadedWorld->minNodes = 4;
            loadedWorld->maxNodes = 10;
            loadedWorld->minMuscles = loadedWorld->minNodes;
            loadedWorld->maxMuscles = loadedWorld->maxNodes * (loadedWorld->maxNodes - 1) / 2;
        }
        loadedWorld->minNodes = std::clamp(loadedWorld->minNodes, 3, 15);
        loadedWorld->maxNodes = std::clamp(loadedWorld->maxNodes, loadedWorld->minNodes, 15);
        loadedWorld->minMuscles = std::clamp(loadedWorld->minMuscles, loadedWorld->minNodes, loadedWorld->maxNodes * (loadedWorld->maxNodes - 1) / 2);
        loadedWorld->maxMuscles = std::clamp(loadedWorld->maxMuscles, loadedWorld->minMuscles, loadedWorld->maxNodes * (loadedWorld->maxNodes - 1) / 2);
        readValue(dataIn, loadedWorld->mutabilityRange);
        readValue(dataIn, loadedWorld->mutabilityFactor);
        readValue(dataIn, loadedWorld->backgroundColor);
        readValue(dataIn, loadedWorld->groundColor);
        readValue(dataIn, loadedWorld->generation);
        if (fileVersion >= 2) {
            readValue(dataIn, loadedWorld->firstStoredGeneration);
        }
        else {
            loadedWorld->firstStoredGeneration = 0;
        }
        if (fileVersion >= 3) {
            readVector(dataIn, loadedWorld->storedHistoryGenerations);
        }
        
        loadedWorld->creatures = std::make_unique<Creature[]>(loadedWorld->numOfCreatures);
        std::vector<Creature> vec;
        readCreatureVector(dataIn, vec);
        for (int i = 0; i < loadedWorld->numOfCreatures; ++i) {
            loadedWorld->creatures[i] = vec[i];
        }
        readCreatureVector(dataIn, loadedWorld->worstGenerationalCreatures);
        readCreatureVector(dataIn, loadedWorld->averageGenerationalCreatures);
        readCreatureVector(dataIn, loadedWorld->bestGenerationalCreatures);
        if (fileVersion < 3 || loadedWorld->storedHistoryGenerations.size() != loadedWorld->worstGenerationalCreatures.size()) {
            loadedWorld->storedHistoryGenerations = CreateContiguousGenerations(loadedWorld->firstStoredGeneration, (int)loadedWorld->worstGenerationalCreatures.size());
        }
        if (!loadedWorld->storedHistoryGenerations.empty()) {
            loadedWorld->firstStoredGeneration = loadedWorld->storedHistoryGenerations.front();
        }
        readPercentileGraph(dataIn, loadedWorld->percentileGraph, fileVersion);
        if (fileVersion >= 5) {
            readSpeciesGraph(dataIn, loadedWorld->speciesGraph);
        }
        loadedWorld->viewGeneration = std::clamp(
            loadedWorld->generation,
            loadedWorld->GetFirstHistoryGeneration(),
            loadedWorld->GetLastHistoryGeneration()
        );
        loadedWorld->percentileGraph.world = loadedWorld.get();
        loadedWorld->speciesGraph.world = loadedWorld.get();
        loadedWorld->StartWorkerThreads();
        in.close();

        std::cout << "World loaded from " << path << std::endl;
        return loadedWorld;
    }
    catch (const std::exception& e) {
		std::cerr << "Failed to load world: " << e.what() << " (most likely corrupted)" << std::endl;
        PushNotice("Failed to load world: " + std::string(e.what()) + " (most likely corrupted)", 5.0f);
        return nullptr;
    }
}

void World::DoGeneration()
{
    if (!StartGeneration()) {
        return;
    }

    while (!FinishGenerationIfReady()) {
        std::unique_lock<std::mutex> lock(mtx);
        workerDone.wait_for(lock, std::chrono::milliseconds(1), [this] { return activeWorkers == 0; });
    }
}

bool World::StartGeneration()
{
    {
        std::unique_lock<std::mutex> lock(mtx);
        if (generationInProgress) {
            return false;
        }

        std::lock_guard<std::mutex> dataLock(dataMutex);
        if (generation >= 0) {
            PrepareNextGeneration();
        }
        generation++;
        currentTicks = ticksPerSecond * secondsPerSimulation;
        activeWorkers = (int)workers.size();
        generationInProgress = true;
    }

    workerStart.notify_all();
    return true;
}

void World::PrepareNextGeneration()
{
    for (int j = 0; j < numOfCreatures / 2; j++) {
        float f = static_cast<float>(j) / numOfCreatures;
        float rand = (std::pow(RNG::randomFloat(-numOfCreatures, numOfCreatures) / numOfCreatures, 3.0f) + 1.0f) / 2.0f;

        int j2 = (f <= rand) ? j : numOfCreatures - 1 - j;

        creatures[j2] = creatures[numOfCreatures - j2 - 1].reproduce();
    }

}

bool World::FinishGenerationIfReady()
{
    {
        std::unique_lock<std::mutex> lock(mtx);
        if (!generationInProgress || activeWorkers > 0) {
            return false;
        }
    }

    {
        std::lock_guard<std::mutex> dataLock(dataMutex);

        for (int i = 0; i < numOfCreatures; ++i) {
            creatures[i].fitness = creatures[i].getCenterX() - creatures[i].getInitialCenterX();
        }

        std::sort(&creatures[0], &creatures[0] + numOfCreatures, [](Creature& a, Creature& b) {
            return a.fitness < b.fitness;
            });

        if (worstGenerationalCreatures.empty()) {
            firstStoredGeneration = generation;
            percentileGraph.firstGeneration = generation;
            percentileGraph.minValue = -1.0f;
            percentileGraph.maxValue = 1.0f;
        }

	    SendGenerationalDataToPercentileGraph();

        for (int i = 0; i < numOfCreatures; ++i) {
            creatures[numOfCreatures - 1 - i].reset();
        }

	    worstGenerationalCreatures.push_back(creatures[0]);
	    averageGenerationalCreatures.push_back(creatures[numOfCreatures / 2]);
	    bestGenerationalCreatures.push_back(creatures[numOfCreatures - 1]);
        storedHistoryGenerations.push_back(generation);

        viewGeneration = generation;
    }

    {
        std::unique_lock<std::mutex> lock(mtx);
        generationInProgress = false;
    }

    return true;
}

bool World::IsGenerationInProgress()
{
    std::unique_lock<std::mutex> lock(mtx);
    return generationInProgress;
}
