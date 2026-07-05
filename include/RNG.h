#pragma once

#include <random>

class RNG {
    static inline std::random_device rd;
    static inline std::mt19937 mt{ rd() };

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

    // Returns an int in [min, max], biased toward lower values.
    // lowerBias should be between 0 and 1.
    // Higher lowerBias = stronger preference for lower numbers.
    static inline int biasedLowerInt(int min, int max, float lowerBias = 0.65f) {
        int value = min;

        while (value < max && randomFloat(0.0f, 1.0f) > lowerBias) {
            ++value;
        }

        return value;
    }
};