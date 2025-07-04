#include "Raylib.h"
#include <string>
#include <stdexcept>
#include "RNG.h"
#include <array>
#include <vector>
#include "Creature.h"
#include "PercentileGraph.h"

class World {
public:
	PercentileGraph percentileGraph = PercentileGraph(520, 30, 300, 250);
	RNG rng;
	std::string worldName;
	uint32_t worldSeed;
	int ticksPerSecond;
	int secondsPerSimulation;
	int numOfCreatures;
	int gravity;
	int generation = -1; // Current generation of the world
	int viewGeneration = 0;
	float mutabilityRange = 1.0f;
	float mutabilityFactor = 1.0f;
	float drawSpeedMult = 1.66f;
	float accumulatedTime = 0.0f;
	std::unique_ptr<Creature[]> creatures;

	std::vector<Creature> worstGenerationalCreatures;
	std::vector<Creature> averageGenerationalCreatures;
	std::vector<Creature> bestGenerationalCreatures;

	Color backgroundColor;
	Color groundColor;
	Camera2D camera = { 0 };
	World() :
		worldName("Default"),
		worldSeed(0),
		ticksPerSecond(100),
		secondsPerSimulation(15),
		numOfCreatures(1000),
		mutabilityRange(1.0f),
		mutabilityFactor(1.0f),
		gravity(98),
		backgroundColor({ 106, 183, 242, 255 }),
		groundColor({ 0, 94, 0, 255 })
		, drawSpeedMult(1.66f) // Default draw speed multiplier
	{
		rng.setSeed(worldSeed);
		creatures = std::make_unique<Creature[]>(numOfCreatures);
		Creature::world = this;
		percentileGraph.world = this;
		Creature::idCounter = 0;
		Creature::updateNodeAndMuscleRangesAndVariations();
		InitializeWorld();
		camera.offset = { 0, 0 };
		camera.target = { 0, 0 };
		camera.rotation = 0.0f;
		camera.zoom = 1.0f;
	}
	World(const std::string& n, const std::string& s, int grav, int nc, int tps, int sps, int mr, int mf, Color bc, Color gc) :

		worldName(n),
		worldSeed(returnRandomWorldSeed(s)), // Convert hex string to unsigned long long
		gravity(grav),
		ticksPerSecond(tps),
		secondsPerSimulation(sps),
		numOfCreatures(nc),
		mutabilityRange((float)mr/100),
		mutabilityFactor((float)mf/100),
		backgroundColor(bc),
		groundColor(gc),
		drawSpeedMult((float)tps/(float)FRAMES_PER_SECOND)
	{
		rng.setSeed(worldSeed);
		creatures = std::make_unique<Creature[]>(numOfCreatures);
		Creature::world = this;
		percentileGraph.world = this;
		Creature::idCounter = 0;
		Creature::updateNodeAndMuscleRangesAndVariations();
		InitializeWorld();
		camera.offset = { 0, 0 };
		camera.target = { 0, 0 };
		camera.rotation = 0.0f;
		camera.zoom = 1.0f;	
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
	void DrawCentered(int x, int y, int width, int height);
	void DoGeneration();
	void InitializeWorld();
	void SendGenerationalDataToPercentileGraph();
	Creature* DrawWithCreatureCentered(int index, int generation);
};