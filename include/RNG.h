#pragma once

#include <random>

class RNG {
	static inline std::random_device rd;
	static inline std::mt19937 mt;
public:
	static inline void randomSeed() {
		mt.seed(rd());
	}
	static inline void setSeed(unsigned int seed) {
		mt.seed(seed);
	}
	static inline int randomInt(int min, int max) {
		std::uniform_int_distribution<int> dist(min, max);
		return dist(mt);
	}
	static inline float randomFloat(float min, float max) {
		std::uniform_real_distribution<float> dist(min, max);
		return dist(mt);
	}
};