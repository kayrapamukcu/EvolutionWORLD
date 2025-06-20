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
	int ticksPerSecond;
	int secondsPerSimulation;
	int numOfCreatures;
	int ticksToSwitchMuscleStage;
	int gravity;
	int generation = -1; // Current generation of the world
	float drawSpeedMult;
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
		ticksToSwitchMuscleStage(100),
		gravity(98),
		backgroundColor({ 255, 196, 240, 255 }),
		groundColor({ 1, 26, 20, 255 })
		, drawSpeedMult(1.66f) // Default draw speed multiplier
	{
		rng.setSeed(worldSeed);
		creatures = std::make_unique<Creature[]>(numOfCreatures);
		Creature::world = this;
		Creature::idCounter = 0;
		InitializeWorld();
		camera.offset = { 0, 0 };
		camera.target = { 0, 0 };
		camera.rotation = 0.0f;
		camera.zoom = 1.0f;

	}
	World(const std::string& n, const std::string& s, int grav, int nc, int tps, int sps, int ttsms, Color bc, Color gc) :

		worldName(n),
		worldSeed(returnRandomWorldSeed(s)), // Convert hex string to unsigned long long
		gravity(grav),
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
	void DrawCreature();
	void DrawWithCreatureCentered(int index, int generation);
};