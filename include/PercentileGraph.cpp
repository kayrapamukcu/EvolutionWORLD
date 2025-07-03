#include "PercentileGraph.h"
#include "Helper.h"
#include "rlgl.h"
#include <iostream>
#include "World.h"
#include <format>

void PercentileGraph::draw()
{
	DrawRectangleB(x, y, width / 2, height / 2, backgroundColor);
	
	if (data[0].empty()) return;

	int dataPoints = std::min((int)data[0].size(), maxDataPoints);

	float currentGuiScale = (float)guiScale / 2.0f;
	float xOffset = x * screenWidthRatio + guiScale;
	float yOffset = y * screenHeightRatio + guiScale;
	float yScale = height / (maxValue - minValue);

	int startGen = std::max(0, world->viewGeneration - dataPoints);
	int endGen = world->viewGeneration;
	
	std::string startLabel = std::format("{}", startGen);
	std::string endLabel = std::format("{}", endGen);

	int labelFontSize = defaultFont.baseSize * guiScale;
	int startLabelWidth = MeasureText(startLabel.c_str(), labelFontSize);
	int endLabelWidth = MeasureText(endLabel.c_str(), labelFontSize);

	// Draw generation labels
	DrawTextEx(defaultFont, startLabel.c_str(), { xOffset - startLabelWidth + 4 * guiScale , yOffset + height * currentGuiScale + 4 * guiScale }, labelFontSize, 1 * guiScale, BLACK);
	DrawTextEx(defaultFont, endLabel.c_str(), { xOffset + width * currentGuiScale - endLabelWidth, yOffset + height * currentGuiScale + 4 * guiScale}, labelFontSize, 1 * guiScale, BLACK);

	// marker lines

	float range = maxValue - minValue;

	int maxTicks = 10;
	float roughStep = range / maxTicks;
	float exponent = std::floor(std::log10(roughStep));
	float fraction = roughStep / std::pow(10, exponent);

	float niceFraction;
	if (fraction <= 1) niceFraction = 1;
	else if (fraction <= 2) niceFraction = 2;
	else if (fraction <= 5) niceFraction = 5;
	else niceFraction = 10;

	float tickStep = niceFraction * std::pow(10, exponent);

	// Start drawing from the first rounded tick >= minValue
	float markerValue = std::ceil(minValue / tickStep) * tickStep;

	while (markerValue <= maxValue) {
		float yVal = (markerValue - minValue) * yScale * currentGuiScale;
		float yPos = yOffset + height * currentGuiScale - yVal;

		// Draw horizontal line
		rlBegin(RL_LINES);
		rlColor4ub(100, 100, 100, 255);
		rlVertex2f(xOffset, yPos);
		rlVertex2f(xOffset + (width * currentGuiScale * 0.99), yPos);
		rlEnd();

		std::string label = std::format("{:.1f}", markerValue);
		int labelWidth = MeasureText(label.c_str(), labelFontSize);
		DrawTextEx(defaultFont, label.c_str(), { xOffset - labelWidth - 6*guiScale, yPos - labelFontSize / 2 }, labelFontSize, 1 * guiScale, GRAY);

		markerValue += tickStep;
	}

	for (int i = 0; i < data.size(); ++i) {
		Color c = percentileColors[i];
		rlBegin(RL_LINES);
		rlColor4ub(c.r, c.g, c.b, c.a);

		float countMult = 1.0f / (dataPoints - 1);
		for (size_t j = 1; j < dataPoints; ++j) {
			float x1 = xOffset + (j - 1) * width * countMult * currentGuiScale * 0.98f;
			float y1 = yOffset + (height - (data[i][j - 1+startGen] - minValue) * yScale) * currentGuiScale;
			float x2 = xOffset + j * width * countMult * currentGuiScale * 0.98f;
			float y2 = yOffset + (height - (data[i][j+startGen] - minValue) * yScale) * currentGuiScale;

			rlVertex2f(x1, y1);
			rlVertex2f(x2, y2);
		}

		rlEnd();
	}
	DrawRectangleLinesB(x, y, width / 2, height / 2, 2, BLACK);
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
	/*if (data[14][data[14].size() - 1] > maxValue) {
		maxValue = data[14][data[14].size() - 1];
		minValue = -0.3 * maxValue;
		minValue *= 1.05f;
		maxValue *= 1.05f;
	}*/
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
