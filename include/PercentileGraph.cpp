#include "PercentileGraph.h"
#include "Helper.h"
#include "rlgl.h"
#include <iostream>
#include "World.h"
#include <format>

void PercentileGraph::draw()
{
	DrawRectangleB(x, y, width / 2, height / 2, backgroundColor);
	DrawRectangleLinesB(x, y, width / 2, height / 2, 2, BLACK);
	if (data[0].empty()) return;

	int dataPoints = std::min((int)data[0].size(), maxDataPoints);

	float currentGuiScale = (float)guiScale / 2.0f;
	float xOffset = x * screenWidthRatio + guiScale;
	float yOffset = y * screenHeightRatio + guiScale;
	float yScale = height / (maxValue - minValue);

	int start = std::max(0, world->viewGeneration - maxDataPoints);

	//draw max and min generations

	DrawTextB(std::format("{}", std::max(0,world->viewGeneration - dataPoints)), x - MeasureText(std::format("{}", std::max(0, world->viewGeneration - dataPoints)).c_str(), 1), y + height, 1, BLACK);
	DrawTextB(std::format("{}", std::max(world->viewGeneration, dataPoints)), x + width - MeasureText(std::format("{}", std::max(world->viewGeneration, dataPoints)).c_str(), 1), y + height, 1, BLACK);


	for (int i = 0; i < data.size(); ++i) {
		Color c = percentileColors[i];
		rlBegin(RL_LINES);
		rlColor4ub(c.r, c.g, c.b, c.a);

		float countMult = 1.0f / (dataPoints - 1);
		for (size_t j = 1; j < dataPoints; ++j) {
			float x1 = xOffset + (j - 1) * width * countMult * currentGuiScale * 0.98f;
			float y1 = yOffset + (height - (data[i][j - 1+start] - minValue) * yScale) * currentGuiScale;
			float x2 = xOffset + j * width * countMult * currentGuiScale * 0.98f;
			float y2 = yOffset + (height - (data[i][j+start] - minValue) * yScale) * currentGuiScale;

			rlVertex2f(x1, y1);
			rlVertex2f(x2, y2);
		}

		rlEnd();
	}
}

void PercentileGraph::updateExtremeValues() // Only called when new data is added, so only the last point of the first and last vector is checked.
{
	if (data[0].empty()) return;
	minValue = -1.0f;
	maxValue = 1.0f;
	int start = std::max(0, world->viewGeneration - maxDataPoints);
	int dataPoints = std::min((int)data[0].size(), maxDataPoints);

	for (int j = 0; j < dataPoints; j++) {
		if (data[0][j + start] < minValue) {
			minValue = data[0][j + start];
		}
		if (data[14][j + start] > maxValue) {
			maxValue = data[14][j + start];
		}

		
	}
	minValue *= 1.05f;
	maxValue *= 1.05f;
}

void PercentileGraph::compressGraph()
{
	for (auto& vec : data) {
		if (vec.size() > maxDataPoints) {
			std::vector<float> compressed;
			for (size_t i = 0; i < vec.size(); i += vec.size() / maxDataPoints) {
				compressed.push_back((vec[i] + vec[i-1]) / 2);
			}
			vec = compressed;
		}
	}
	updateExtremeValues();
}
