#include "raylib.h"
#include <string>
#include <stdexcept>
#include "RNG.h"
#include <array>
#include <algorithm>
#include <vector>
#include "Creature.h"
#include "PercentileGraph.h"
#include "SpeciesGraph.h"
#include <thread>
#include <condition_variable>
#include <atomic>
#include <fstream>
#include <sstream>
#include <iostream>


// 0 : v1.4.0
// 1 : v1.5.0 (but incompatible with older versions; compression added)
// 2 : v1.5.2+ history ranges and percentile graph generation offset
// 3 : sparse saved history generation indices
// 4 : creature node/muscle count ranges
// 5 : species graph history
constexpr uint64_t savefileVersion = 5;

// Helper functions for reading/writing binary data
template <typename T>
inline void writeValue(std::ostream& out, const T& value) {
	static_assert(std::is_trivially_copyable_v<T>);
	out.write(reinterpret_cast<const char*>(&value), sizeof(T));
}
template <typename T>
inline void readValue(std::istream& in, T& value) {
	static_assert(std::is_trivially_copyable_v<T>);
	in.read(reinterpret_cast<char*>(&value), sizeof(T));
}
inline void writeString(std::ostream& out, const std::string& str) {
	uint64_t size = str.size();
	writeValue(out, size);
	out.write(str.data(), size);
}
inline void readString(std::istream& in, std::string& str) {
	uint64_t size;
	readValue(in, size);
	str.resize(size);
	in.read(str.data(), size);
}
template <typename T>
void writeVector(std::ostream& out, const std::vector<T>& vec) {
    static_assert(std::is_trivially_copyable_v<T>);

    uint64_t size = vec.size();
    writeValue(out, size);

    out.write(reinterpret_cast<const char*>(vec.data()), sizeof(T) * size);
}
template <typename T>
void readVector(std::istream& in, std::vector<T>& vec) {
    static_assert(std::is_trivially_copyable_v<T>);

    uint64_t size;
    readValue(in, size);

    vec.resize(size);

    in.read(reinterpret_cast<char*>(vec.data()), sizeof(T) * size);
}
inline void writeCreature(std::ostream& out, const Creature& c) {
	writeValue(out, c.id);
	writeValue(out, c.fitness);
	writeValue(out, c.muscleCount);
	writeValue(out, c.nodeCount);
	
	for (int i = 0; i < c.nodeCount; ++i) {
		const Node& n = c.nodes[i];
		writeValue(out, n.initialX);
		writeValue(out, n.initialY);
		writeValue(out, n.x);
		writeValue(out, n.y);
		writeValue(out, n.xSpeed);
		writeValue(out, n.ySpeed);
		writeValue(out, n.mass);
		writeValue(out, n.friction);
	}
	for (int i = 0; i < c.muscleCount; ++i) {
		const Muscle& m = c.muscles[i];
		writeValue(out, m.node1);
		writeValue(out, m.node2);
		writeValue(out, m.state1Ticks);
		writeValue(out, m.state2Ticks);
		writeValue(out, m.currentMuscleStage);
		writeValue(out, m.muscleTickCounter);
		writeValue(out, m.length1);
		writeValue(out, m.length2);
		writeValue(out, m.strength);
	}
}
inline void readCreature(std::istream& in, Creature& c) {
	readValue(in, c.id);
	readValue(in, c.fitness);
	readValue(in, c.muscleCount);
	readValue(in, c.nodeCount);
	
	c.nodes = std::make_unique<Node[]>(c.nodeCount);
	for (int i = 0; i < c.nodeCount; ++i) {
		Node& n = c.nodes[i];
		readValue(in, n.initialX);
		readValue(in, n.initialY);
		readValue(in, n.x);
		readValue(in, n.y);
		readValue(in, n.xSpeed);
		readValue(in, n.ySpeed);
		readValue(in, n.mass);
		readValue(in, n.friction);
	}
	c.muscles = std::make_unique<Muscle[]>(c.muscleCount);
	for (int i = 0; i < c.muscleCount; ++i) {
		Muscle& m = c.muscles[i];
		readValue(in, m.node1);
		readValue(in, m.node2);
		readValue(in, m.state1Ticks);
		readValue(in, m.state2Ticks);
		readValue(in, m.currentMuscleStage);
		readValue(in, m.muscleTickCounter);
		readValue(in, m.length1);
		readValue(in, m.length2);
		readValue(in, m.strength);
	}
}
inline void writeCreatureVector(std::ostream& out, const std::vector<Creature>& vec) {
	uint64_t size = vec.size();
	writeValue(out, size);

	for (const Creature& c : vec) {
		writeCreature(out, c);
	}
}
inline void readCreatureVector(std::istream& in, std::vector<Creature>& vec) {
	uint64_t size;
	readValue(in, size);

	vec.resize(size);

	for (Creature& c : vec) {
		readCreature(in, c);
	}
}
inline void writePercentileGraph(std::ostream& out, const PercentileGraph& g) {
    writeValue(out, g.x);
    writeValue(out, g.y);
    writeValue(out, g.width);
    writeValue(out, g.height);

    writeValue(out, g.maxValue);
    writeValue(out, g.minValue);
    writeValue(out, g.maxDataPoints);

    for (const auto& vec : g.data) {
		writeVector(out, vec);
    }

    for (const Color& color : g.percentileColors) {
        writeValue(out, color);
    }

    writeValue(out, g.backgroundColor);
	writeValue(out, g.firstGeneration);
	writeVector(out, g.storedGenerations);
}
inline void writeSpeciesGraph(std::ostream& out, const SpeciesGraph& g) {
	writeValue(out, g.x);
	writeValue(out, g.y);
	writeValue(out, g.width);
	writeValue(out, g.height);
	writeValue(out, g.maxDataPoints);
	writeValue(out, g.firstGeneration);
	writeValue(out, g.backgroundColor);
	writeVector(out, g.storedGenerations);

	uint64_t generationCount = g.data.size();
	writeValue(out, generationCount);
	for (const auto& generationData : g.data) {
		writeVector(out, generationData);
	}
}
inline void readPercentileGraph(std::istream& in, PercentileGraph& g) {
	readValue(in, g.x);
	readValue(in, g.y);
	readValue(in, g.width);
	readValue(in, g.height);

	readValue(in, g.maxValue);
	readValue(in, g.minValue);
	readValue(in, g.maxDataPoints);

	for (auto& vec : g.data) {
		readVector(in, vec);
	}

	for (Color& color : g.percentileColors) {
		readValue(in, color);
	}

	readValue(in, g.backgroundColor);
	readValue(in, g.firstGeneration);
	readVector(in, g.storedGenerations);

	g.world = nullptr;
}
inline void readSpeciesGraph(std::istream& in, SpeciesGraph& g) {
	readValue(in, g.x);
	readValue(in, g.y);
	readValue(in, g.width);
	readValue(in, g.height);
	readValue(in, g.maxDataPoints);
	readValue(in, g.firstGeneration);
	readValue(in, g.backgroundColor);
	readVector(in, g.storedGenerations);

	uint64_t generationCount;
	readValue(in, generationCount);
	g.data.resize(generationCount);
	for (auto& generationData : g.data) {
		readVector(in, generationData);
	}

	g.world = nullptr;
}

struct Savefile {
	uint64_t savefileVersion;
	std::string worldName;
	uint32_t worldSeed;
	int gravity;
	int ticksPerSecond;
	int secondsPerSimulation;
	int numOfCreatures;
	int minNodes;
	int maxNodes;
	int minMuscles;
	int maxMuscles;
	float mutabilityRange;
	float mutabilityFactor;
	Color backgroundColor;
	Color groundColor;

	int generation;

	std::vector<Creature> currentGenCreatures;

	std::vector<Creature> worstGenerationalCreatures;
	std::vector<Creature> averageGenerationalCreatures;
	std::vector<Creature> bestGenerationalCreatures;

	PercentileGraph percentileGraph;
	SpeciesGraph speciesGraph;
};

struct MiniWorld {
	unsigned constexpr static int creature_data_len = 160;
	unsigned constexpr static char creature_data[creature_data_len] = {
	  0x7d, 0xc5, 0x17, 0x00, 0x44, 0x6b, 0x3f, 0x45, 0x03, 0x03, 0xf7, 0x6a,
	  0x27, 0xc1, 0xb0, 0x83, 0x7c, 0xbf, 0x1b, 0x1a, 0x9b, 0x43, 0xa4, 0x09,
	  0x7f, 0xc1, 0xa6, 0xec, 0x3f, 0x3f, 0xf2, 0xad, 0x5d, 0x3e, 0x1c, 0x1a,
	  0x8a, 0x3f, 0xf1, 0xd7, 0xfb, 0x3f, 0x7c, 0xb9, 0x89, 0x42, 0x00, 0x00,
	  0xb4, 0x42, 0x7b, 0xbd, 0x9a, 0x43, 0xef, 0x7e, 0xa0, 0x42, 0x05, 0x79,
	  0xba, 0x40, 0x73, 0xaa, 0xab, 0xbf, 0xce, 0x44, 0xa7, 0x3f, 0x73, 0xee,
	  0xb6, 0x3f, 0x3b, 0x98, 0x69, 0xc2, 0xa5, 0x25, 0x0e, 0xc1, 0xbb, 0x77,
	  0x5a, 0x43, 0x00, 0x00, 0xb4, 0x42, 0xda, 0x47, 0x62, 0xc0, 0x00, 0x00,
	  0x00, 0x80, 0xf2, 0x43, 0x9a, 0x3f, 0x2a, 0xe1, 0x4a, 0x40, 0x00, 0x01,
	  0x41, 0x27, 0x01, 0x04, 0x00, 0x00, 0xa0, 0x42, 0x00, 0x00, 0xa0, 0x42,
	  0x92, 0xb9, 0x03, 0x40, 0x01, 0x02, 0x1f, 0x20, 0x01, 0x08, 0x00, 0x00,
	  0x20, 0x43, 0x00, 0x00, 0xa0, 0x42, 0x00, 0x00, 0x20, 0x40, 0x00, 0x02,
	  0x3b, 0x2a, 0x01, 0x0a, 0x3f, 0x5b, 0x0f, 0x43, 0xce, 0xe9, 0x11, 0x43,
	  0x4c, 0x4b, 0x97, 0x3f
	};
	

	Color backgroundColor = { 106, 183, 242, 255 };
	Color groundColor = { 0, 94, 0, 255 };
	float accumulatedTime = 0.0f;
	constexpr static int gravity = 98;

	Creature creature = Creature();
	Camera2D camera = { 0 };
	inline MiniWorld() {
		std::string creatureDataStr(reinterpret_cast<const char*>(creature_data), creature_data_len);
		std::istringstream in(creatureDataStr, std::ios::binary);
		readCreature(in, creature);
	}
	inline void drawtick(int x, int y, int width, int height) {
		Rectangle viewport = DrawRectUI(x, y, width, height, backgroundColor, UIAnchor::Center);
		DrawRectangle(
			(int)viewport.x,
			(int)(viewport.y + 4.0f * viewport.height / 5.0f),
			(int)viewport.width,
			(int)(viewport.height / 5.0f),
			groundColor);

		BeginScissorMode((int)viewport.x, (int)viewport.y, (int)viewport.width, (int)viewport.height);

		camera.offset = { viewport.x + viewport.width / 2.0f, viewport.y + 4.0f * viewport.height / 5.0f };
		camera.target = { creature.getCenterX(), (float)Creature::FLOOR_HEIGHT };
		camera.zoom = 1.0f * UIScale();
		BeginMode2D(camera);
		// draw rectangles 1 meter apart (100 pixels)
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
		EndScissorMode();
		accumulatedTime += 1.66f;
		while (accumulatedTime > 1) {
			creature.tick();
			accumulatedTime--;
		}
	}
};

class World {
public:
	std::vector<std::thread> workers;
	std::mutex mtx;
	std::mutex dataMutex;
	std::condition_variable workerStart;
	std::condition_variable workerDone;

	int activeWorkers = 0;
	bool terminate = false;
	bool generationInProgress = false;
	int currentTicks = 0; // Shared with workers

	PercentileGraph percentileGraph = PercentileGraph(600, 40, 400, 330);
	SpeciesGraph speciesGraph = SpeciesGraph(600, 400, 400, 330);
	inline static std::string worldName;
	inline static uint32_t worldSeed;
	inline static int ticksPerSecond = 100;
	inline static int secondsPerSimulation = 15;
	inline static int numOfCreatures = 1000;
	inline static int gravity = 98;

	inline static int minNodes = 3;
	inline static int maxNodes = 15;

	inline static int minMuscles = minNodes;
	inline static int maxMuscles = maxNodes * (maxNodes - 1) / 2;
	inline static float structuralMutationChance = 0.1f;
	inline static float muscleMutationMultiplierWhenNodeMutated = 6.0f;

	int generation = -1; // Current generation of the world
	int viewGeneration = 0;
	int firstStoredGeneration = 0;
	std::vector<int> storedHistoryGenerations;
	inline static float mutabilityRange = 1.0f;
	inline static float mutabilityFactor = 1.0f;
	inline static float drawSpeedMult = 1.66f;
	float accumulatedTime = 0.0f;
	std::unique_ptr<Creature[]> creatures;

	std::vector<Creature> worstGenerationalCreatures;
	std::vector<Creature> averageGenerationalCreatures;
	std::vector<Creature> bestGenerationalCreatures;

	Color backgroundColor;
	Color groundColor;
	Camera2D camera = { 0 };
	World(bool initialize = true) :
		backgroundColor({ 106, 183, 242, 255 }),
		groundColor({ 0, 94, 0, 255 })
	{
		worldName = "Default";
		worldSeed = 0;
		ticksPerSecond = 100;
		secondsPerSimulation = 15;
		numOfCreatures = 1000;
		minNodes = 3;
		maxNodes = 15;
		minMuscles = minNodes;
		maxMuscles = maxNodes * (maxNodes - 1) / 2;
		mutabilityRange = 1.0f;
		mutabilityFactor = 1.0f;
		drawSpeedMult = 1.6666666f;
		gravity = 98;
		RNG::setSeed(worldSeed);
		creatures = std::make_unique<Creature[]>(numOfCreatures);
		percentileGraph.world = this;
		speciesGraph.world = this;
		if (initialize) {
			Creature::idCounter = 0;
			Creature::updateNodeAndMuscleRangesAndVariations();
			InitializeWorld();
		}
		camera.offset = { 0, 0 };
		camera.target = { 0, 0 };
		camera.rotation = 0.0f;
		camera.zoom = 1.0f;
	}
	World(const std::string& n, const std::string& s, int grav, int nc, int sps, int mr, int mf, int minNodeCount, int maxNodeCount, int minMuscleCount, int maxMuscleCount, Color bc, Color gc) :
		backgroundColor(bc),
		groundColor(gc)
	{
		worldName = n;
		worldSeed = returnRandomWorldSeed(s); // Convert hex string to unsigned long long
		secondsPerSimulation = sps;
		numOfCreatures = nc;
		minNodes = minNodeCount;
		maxNodes = maxNodeCount;
		minMuscles = minMuscleCount;
		maxMuscles = maxMuscleCount;
		mutabilityRange = (float)mr / 100;
		mutabilityFactor = (float)mf / 100;
		drawSpeedMult = 1.6666666f;
		gravity = grav;
		RNG::setSeed(worldSeed);
		creatures = std::make_unique<Creature[]>(numOfCreatures);
		percentileGraph.world = this;
		speciesGraph.world = this;
		Creature::idCounter = 0;
		Creature::updateNodeAndMuscleRangesAndVariations();
		InitializeWorld();
		camera.offset = { 0, 0 };
		camera.target = { 0, 0 };
		camera.rotation = 0.0f;
		camera.zoom = 1.0f;	

	}
	~World() {
		{
			std::unique_lock<std::mutex> lock(mtx);
			terminate = true; // Flip the kill switch
		}

		workerStart.notify_all();

		for (auto& th : workers) {
			if (th.joinable()) {
				th.join();
			}
		}	
	}
	static unsigned int const returnRandomWorldSeed(const std::string& entered) {
		// if the entire string is a number, return it as a seed
		// otherwise, return hashed seed
		// if empty, return a random seed
		
		if (entered.empty()) {
			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_int_distribution<unsigned int> dis(0, UINT32_MAX);
			return dis(gen);
		}
		try {
			return (std::stoull(entered, nullptr, 10) % UINT32_MAX);
		}
		catch (const std::invalid_argument&) {
			return std::hash<std::string>{}(entered) % UINT32_MAX;
		}
		catch (const std::exception&) {
			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_int_distribution<unsigned int> dis(0, UINT32_MAX);
			return dis(gen);
		}
	}
	void Draw(int x, int y, int width, int height);
	void DoGeneration();
	bool StartGeneration();
	bool FinishGenerationIfReady();
	bool IsGenerationInProgress();
	void InitializeWorld();
	void StartWorkerThreads();
	void SendGenerationalDataToPercentileGraph();
	void WorkerThread(int begin, int end);
	void Save(std::atomic<bool>* workStarted = nullptr, int saveStartGeneration = -1, int saveEndGeneration = -1, bool savePercentileGraph = true, bool saveSpeciesGraph = true);
	static bool Load();
	static std::unique_ptr<World> LoadFromDialog(std::atomic<bool>* workStarted = nullptr);
	Creature* DrawWithCreatureCentered(int index, int generation, float playbackSpeed = 1.0f, bool paused = false);
	Creature* DrawCreatureCentered(Creature& creature, float playbackSpeed = 1.0f, bool paused = false);
	Creature* DrawCurrentCreatureCentered(int index);
	void PrepareNextGeneration();
	int GetHistoryIndexForGeneration(int generation) const;
	int GetFirstHistoryGeneration() const;
	int GetLastHistoryGeneration() const;
	bool HasHistoryDataForGeneration(int generation) const;
	bool HasHistoryDataInRange(int startGeneration, int endGeneration) const;

	inline void printCreature(const Creature& c) {
		std::cout << "\nCreature ID: " << c.id << "\n";
		std::cout << "Fitness: " << c.fitness << "\n";
		std::cout << "Muscle Count: " << (int)c.muscleCount << "\n";
		std::cout << "Node Count: " << (int)c.nodeCount << "\n";
		
		std::cout << "Nodes:\n";
		for (int i = 0; i < c.nodeCount; ++i) {
			const Node& n = c.nodes[i];
			std::cout << "  Node " << i << ": initialX=" << n.initialX << ", initialY=" << n.initialY
				<< ", x=" << n.x << ", y=" << n.y
				<< ", xSpeed=" << n.xSpeed << ", ySpeed=" << n.ySpeed
				<< ", mass=" << n.mass << ", friction=" << n.friction << "\n";
		}
		std::cout << "Muscles:\n";
		for (int i = 0; i < c.muscleCount; ++i) {
			const Muscle& m = c.muscles[i];
			std::cout << "  Muscle " << i << ": node1=" << (int)m.node1
				<< ", node2=" << (int)m.node2
				<< ", state1Ticks=" << (int)m.state1Ticks
				<< ", state2Ticks=" << (int)m.state2Ticks
				<< ", currentMuscleStage=" << m.currentMuscleStage
				<< ", muscleTickCounter=" << (int)m.muscleTickCounter
				<< ", length1=" << m.length1
				<< ", length2=" << m.length2
				<< ", strength=" << m.strength << "\n";
		}
	}
};
