#pragma once

#include <random>

class RNG {
	std::random_device rd;
	std::mt19937 mt;
public:
	void randomSeed() {
		mt.seed(rd());
	}
	void setSeed(unsigned int seed) {
		mt.seed(seed);
	}
	int randomInt(int min, int max) {
		std::uniform_int_distribution<int> dist(min, max);
		return dist(mt);
	}
	float randomFloat(float min, float max) {
		std::uniform_real_distribution<float> dist(min, max);
		return dist(mt);
	}
};