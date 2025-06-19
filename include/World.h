#include "Raylib.h"
#include <string>
#include <stdexcept>
#include "RNG.h"
#include <array>
#include <vector>
#include "Creature.h"

class World {
public:
	RNG rng;
	std::string worldName;
	uint32_t worldSeed;
	uint32_t ticksPerSecond;
	uint32_t secondsPerSimulation;
	uint32_t numOfCreatures;
	uint32_t ticksToSwitchMuscleStage;
	uint32_t generation = 0; // Current generation of the world
	float drawSpeedMult;
	float accumulatedTime = 0.0f;
	std::unique_ptr<Creature[]> creatures;
	std::vector<Creature> generationalCreatures; // Worst, average, and best creature of all generations
	Color backgroundColor;
	Color groundColor;
	World() :
		worldName("Default"),
		worldSeed(0),
		ticksPerSecond(100),
		secondsPerSimulation(15),
		numOfCreatures(1000),
		ticksToSwitchMuscleStage(100),
		backgroundColor({ 255, 196, 240, 255 }),
		groundColor({ 1, 26, 20, 255 })
		, drawSpeedMult(1.66f) // Default draw speed multiplier
	{
		rng.setSeed(worldSeed);
		creatures = std::make_unique<Creature[]>(numOfCreatures);
		Creature::world = this;
		Creature::idCounter = 0;
		InitializeWorld();
	}
	World(const std::string& n, const std::string& s, int nc, int tps, int sps, int ttsms, Color bc, Color gc) :

		worldName(n),
		worldSeed(returnRandomWorldSeed(s)), // Convert hex string to unsigned long long
		ticksPerSecond(tps),
		secondsPerSimulation(sps),
		numOfCreatures(nc),
		ticksToSwitchMuscleStage(ttsms),
		backgroundColor(bc),
		groundColor(gc),
		drawSpeedMult(tps/FRAMES_PER_SECOND)
	{
		rng.setSeed(worldSeed);
		creatures = std::make_unique<Creature[]>(numOfCreatures);
		Creature::world = this;
		Creature::idCounter = 0;
		InitializeWorld();
	}
	static unsigned int const returnRandomWorldSeed(const std::string& entered) {
		// if the entire string is a number, return it as a seed
		// otherwise, return hashed seed
		// if empty, return a random seed
		
		if (entered.empty()) {
			return GetRandomValue(0, UINT32_MAX);
		}
		try {
			return (std::stoull(entered, nullptr, 10) % UINT32_MAX);
		}
		catch (const std::invalid_argument&) {
			return std::hash<std::string>{}(entered) % UINT32_MAX;
		}
		catch (const std::exception&) {
			return GetRandomValue(0, UINT32_MAX);
		}
	}
	void Draw(int x, int y, int width, int height);
	void DrawCentered(int x, int y, int width, int height);
	void DoGeneration();
	void InitializeWorld();
	void DrawCreature();
};