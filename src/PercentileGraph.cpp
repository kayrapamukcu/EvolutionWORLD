#include "PercentileGraph.h"
#include "Helper.h"
#include "rlgl.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include "World.h"
#include <format>

void PercentileGraph::draw() const
{
	Rectangle graphRect = DrawRectUI(x, y, width, height, backgroundColor);
	
	if (data[0].empty()) return;

	int dataSize = (int)data[0].size();
	int dataPoints = std::min(dataSize, maxDataPoints);
	if (dataPoints < 2) return;
	auto generationAt = [&](int index) {
		if ((int)storedGenerations.size() == dataSize) {
			return storedGenerations[index];
		}
		return firstGeneration + index;
		};
	int viewDataIndex = 0;
	if ((int)storedGenerations.size() == dataSize && !storedGenerations.empty()) {
		auto it = std::lower_bound(storedGenerations.begin(), storedGenerations.end(), world->viewGeneration);
		if (it == storedGenerations.end()) {
			viewDataIndex = dataSize - 1;
		}
		else if (it == storedGenerations.begin()) {
			viewDataIndex = 0;
		}
		else {
			int rightIndex = (int)std::distance(storedGenerations.begin(), it);
			int leftIndex = rightIndex - 1;
			viewDataIndex = std::abs(storedGenerations[rightIndex] - world->viewGeneration) < std::abs(world->viewGeneration - storedGenerations[leftIndex])
				? rightIndex
				: leftIndex;
		}
	}
	else {
		viewDataIndex = std::clamp(world->viewGeneration - firstGeneration, 0, dataSize - 1);
	}

	float padding = std::max(1.0f, 2.0f * UIScale());
	float plotLeft = graphRect.x + padding;
	float plotTop = graphRect.y + padding;
	float plotWidth = std::max(1.0f, graphRect.width - 2.0f * padding);
	float plotHeight = std::max(1.0f, graphRect.height - 2.0f * padding);
	float yScale = plotHeight / (maxValue - minValue);
	
	int maxStartGen = std::max(0, dataSize - dataPoints);
	int startGen = std::clamp(viewDataIndex - dataPoints / 2, 0, maxStartGen);
	int endGen = startGen + dataPoints - 1;

	std::string startLabel = std::format("{}", generationAt(startGen));
	std::string endLabel = std::format("{}", generationAt(endGen));
	
	float labelFontSize = UIFontSize(0.5f);
	float labelSpacing = UISpacing(0.5f);
	float startLabelWidth = MeasureTextEx(defaultFont, startLabel.c_str(), labelFontSize, labelSpacing).x;
	float endLabelWidth = MeasureTextEx(defaultFont, endLabel.c_str(), labelFontSize, labelSpacing).x;

	// Draw generation labels
	DrawTextEx(defaultFont, startLabel.c_str(), { plotLeft - startLabelWidth/2.0f, graphRect.y + graphRect.height + 4 * UIScale() }, labelFontSize, labelSpacing, BLACK);
	DrawTextEx(defaultFont, endLabel.c_str(), { plotLeft + plotWidth - endLabelWidth/2.0f, graphRect.y + graphRect.height + 4 * UIScale() }, labelFontSize, labelSpacing, BLACK);

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
		float yVal = (markerValue - minValue) * yScale;
		float yPos = plotTop + plotHeight - yVal;

		// Draw horizontal line
		rlBegin(RL_LINES);
		rlColor4ub(100, 100, 100, 255);
		rlVertex2f(plotLeft, yPos);
		rlVertex2f(plotLeft + plotWidth, yPos);
		rlEnd();

		std::string label = std::format("{:.1f}", markerValue);
		int labelWidth = MeasureText(label.c_str(), labelFontSize);
		DrawTextEx(defaultFont, label.c_str(), { plotLeft - labelWidth - 6 * UIScale(), yPos - labelFontSize / 2 }, labelFontSize, labelSpacing, GRAY);

		markerValue += tickStep;
	}

	for (int i = 0; i < data.size(); ++i) {
		Color c = percentileColors[i];
		rlBegin(RL_LINES);
		rlColor4ub(c.r, c.g, c.b, c.a);

		float countMult = 1.0f / (dataPoints - 1);
		for (size_t j = 1; j < dataPoints; ++j) {
			float x1 = plotLeft + (j - 1) * plotWidth * countMult;
			float y1 = plotTop + plotHeight - (data[i][j - 1 + startGen] - minValue) * yScale;
			float x2 = plotLeft + j * plotWidth * countMult;
			float y2 = plotTop + plotHeight - (data[i][j + startGen] - minValue) * yScale;

			rlVertex2f(x1, y1);
			rlVertex2f(x2, y2);
		}

		rlEnd();
	}
	DrawRectUI(x, y, width, height, BLACK, UIAnchor::TopLeft, 2);
	int genRange = endGen - startGen;
	int currentDrawGen = std::clamp(viewDataIndex - startGen, 0, genRange);

	// Draw vertical green line at the position of currentDrawGen
	float xLine = plotLeft + plotWidth * ((float)currentDrawGen / std::max(1, genRange));
	DrawRectangle((int)xLine, (int)plotTop, std::max(1, (int)UIScale()), (int)plotHeight, GREEN);
}

void PercentileGraph::updateExtremeValues() // Only called when new data is added, so only the last point of the first and last vector is checked.
{
	if (data[0].empty()) return;
	/*iminValue = -1.0f;
	maxValue = 1.0f;
	nt start = std::max(0, world->viewGeneration - maxDataPoints);
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
	maxValue *= 1.05f;*/

	if (data[14][data[14].size() - 1] > maxValue) {
		maxValue = data[14][data[14].size() - 1];
		maxValue *= 1.05f;
	}
	if (data[0][data[0].size() - 1] < minValue) {
		minValue = data[0][data[0].size() - 1];
		minValue *= 1.05f;
	}
}

void PercentileGraph::compressGraph()
{
	std::vector<int> compressedGenerations;
	if (storedGenerations.size() > maxDataPoints) {
		size_t stride = std::max<size_t>(1, storedGenerations.size() / maxDataPoints);
		for (size_t i = 0; i < storedGenerations.size(); i += stride) {
			compressedGenerations.push_back(storedGenerations[i]);
		}
	}
	for (auto& vec : data) {
		if (vec.size() > maxDataPoints) {
			std::vector<float> compressed;
			size_t stride = std::max<size_t>(1, vec.size() / maxDataPoints);
			for (size_t i = 0; i < vec.size(); i += stride) {
				compressed.push_back(vec[i]);
			}
			vec = compressed;
		}
	}
	if (!compressedGenerations.empty()) {
		storedGenerations = compressedGenerations;
	}
	updateExtremeValues();
}
