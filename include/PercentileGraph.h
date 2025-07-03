#pragma once
#include <vector>
#include <array>
#include "raylib.h"
#include "Helper.h"

class World;

class PercentileGraph {
public:
	World* world = nullptr;
	int x, y, width, height;
	float maxValue = 1.0f;
	float minValue = -1.0f;

	int maxDataPoints = 500;
	std::array<std::vector<float>, 15> data; // holds all the percentile data, worst = 0, best = 14
	std::array<Color, 15> percentileColors;
	Color backgroundColor = { 200, 200, 200, 255 };

	RenderTexture2D renderTex;

	void draw();
	void updateExtremeValues();
	void compressGraph();

	PercentileGraph(int xPos, int yPos, int w, int h) : x(xPos), y(yPos), width(w), height(h) {
		renderTex = LoadRenderTexture((int)(width * (float)guiScale / 2.0f), (int)(height * (float)guiScale / 2.0f));
		data.fill(std::vector<float>());
		for (int i = 0; i < data.size(); ++i) {
			data[i].reserve(maxDataPoints * 2);
		}
		percentileColors[0] = { 200, 0, 0, 255 }; // worst
		percentileColors[1] = { 180, 0, 0, 255 };
		percentileColors[2] = { 160, 0, 0, 255 };
		percentileColors[3] = { 140, 0, 0, 255 };
		percentileColors[4] = { 120, 0, 0, 255 };
		percentileColors[5] = { 100, 0, 0, 255 };
		percentileColors[6] = { 80, 0, 0, 255 };
		percentileColors[7] = { 0, 0, 200, 255 };
		percentileColors[8] = { 0, 80, 0, 255 };
		percentileColors[9] = { 0, 100, 0, 255 };
		percentileColors[10] = { 0, 120, 0, 255 };
		percentileColors[11] = { 0, 140, 0, 255 };
		percentileColors[12] = { 0, 160, 0, 255 };
		percentileColors[13] = { 0, 180, 0, 255 };
		percentileColors[14] = { 0, 200, 0, 255 }; // best 
	}

	
};