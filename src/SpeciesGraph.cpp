#include "SpeciesGraph.h"
#include "Creature.h"
#include "World.h"
#include "rlgl.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <format>
#include <unordered_map>
#include <utility>

std::pair<Color, Color> getSpeciesColor(int nodeCount, int muscleCount);

namespace {
	uint64_t MakeSpeciesKey(int nodeCount, int muscleCount)
	{
		return (static_cast<uint64_t>(static_cast<uint32_t>(nodeCount)) << 32)
			| static_cast<uint32_t>(muscleCount);
	}

	int GetSpeciesCount(const std::vector<SpeciesCount>& generationData, uint64_t key)
	{
		for (const SpeciesCount& species : generationData) {
			if (MakeSpeciesKey(species.nodeCount, species.muscleCount) == key) {
				return species.count;
			}
		}
		return 0;
	}
}

void SpeciesGraph::draw() const
{
	Rectangle graphRect = DrawRectUI((float)x, (float)y, (float)width, (float)height, backgroundColor);

	if (data.empty()) return;

	int dataSize = (int)data.size();
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

	int maxStartGen = std::max(0, dataSize - dataPoints);
	int startGen = std::clamp(viewDataIndex - dataPoints / 2, 0, maxStartGen);
	int endGen = startGen + dataPoints - 1;

	struct VisibleSpecies {
		uint64_t key = 0;
		int nodeCount = 0;
		int muscleCount = 0;
		int totalCount = 0;
	};
	std::unordered_map<uint64_t, VisibleSpecies> speciesTotals;
	for (int i = startGen; i <= endGen; ++i) {
		for (const SpeciesCount& species : data[i]) {
			uint64_t key = MakeSpeciesKey(species.nodeCount, species.muscleCount);
			VisibleSpecies& visible = speciesTotals[key];
			visible.key = key;
			visible.nodeCount = species.nodeCount;
			visible.muscleCount = species.muscleCount;
			visible.totalCount += species.count;
		}
	}

	std::vector<VisibleSpecies> visibleSpecies;
	visibleSpecies.reserve(speciesTotals.size());
	for (const auto& [key, species] : speciesTotals) {
		visibleSpecies.push_back(species);
	}
	std::sort(visibleSpecies.begin(), visibleSpecies.end(), [](const VisibleSpecies& a, const VisibleSpecies& b) {
		if (a.nodeCount != b.nodeCount) return a.nodeCount < b.nodeCount;
		return a.muscleCount < b.muscleCount;
		});

	std::vector<int> generationTotals;
	generationTotals.reserve(dataPoints);
	for (int i = startGen; i <= endGen; ++i) {
		int total = 0;
		for (const SpeciesCount& species : data[i]) {
			total += species.count;
		}
		generationTotals.push_back(std::max(1, total));
	}

	float countMult = 1.0f / (dataPoints - 1);
	BeginScissorMode((int)graphRect.x, (int)graphRect.y, (int)graphRect.width, (int)graphRect.height);
	std::vector<int> cumulativeCounts(dataPoints, 0);
	for (const VisibleSpecies& species : visibleSpecies) {
		auto colors = getSpeciesColor(species.nodeCount, species.muscleCount);
		Color color = colors.first;
		Color color2 = colors.second;

		for (int j = 0; j < dataPoints; ++j) {
			int speciesCount = GetSpeciesCount(data[j + startGen], species.key);
			int total = generationTotals[j];

			float x1 = plotLeft + j * plotWidth * countMult;
			float x2 = j == dataPoints - 1
				? plotLeft + plotWidth
				: plotLeft + (j + 1) * plotWidth * countMult;
			float bottom = plotTop + plotHeight - ((float)cumulativeCounts[j] / total) * plotHeight;
			float top = plotTop + plotHeight - ((float)(cumulativeCounts[j] + speciesCount) / total) * plotHeight;

			if (top < bottom) {
				DrawRectangleGradientEx({ x1, top, std::max(1.0f, x2 - x1), std::max(1.0f, bottom - top) }, color2, color, color, color2);
			}
			cumulativeCounts[j] += speciesCount;
		}
	}

	EndScissorMode();

	DrawRectUI((float)x, (float)y, (float)width, (float)height, BLACK, UIAnchor::TopLeft, 2);

	int genRange = endGen - startGen;
	int currentDrawGen = std::clamp(viewDataIndex - startGen, 0, genRange);
	float xLine = plotLeft + plotWidth * ((float)currentDrawGen / std::max(1, genRange));
	DrawRectangle((int)xLine, (int)plotTop, std::max(1, (int)UIScale()), (int)plotHeight, GREEN);

	if (viewDataIndex >= startGen && viewDataIndex <= endGen) {
		float labelFontSize = UIFontSize(0.55f);
		float labelSpacing = UISpacing(0.55f);
		float labelPadding = std::max(2.0f, 3.0f * UIScale());
		float labelGap = std::max(4.0f, 5.0f * UIScale());
		int localViewIndex = viewDataIndex - startGen;
		int total = generationTotals[localViewIndex];
		int pluralityCount = 0;
		for (const SpeciesCount& species : data[viewDataIndex]) {
			pluralityCount = std::max(pluralityCount, species.count);
		}
		int cumulativeCount = 0;

		for (const VisibleSpecies& species : visibleSpecies) {
			int speciesCount = GetSpeciesCount(data[viewDataIndex], species.key);
			int previousCumulativeCount = cumulativeCount;
			cumulativeCount += speciesCount;
			if (speciesCount < 20) {
				continue;
			}

			auto colors = getSpeciesColor(species.nodeCount, species.muscleCount);
			std::string speciesLabel = std::format("N{}M{}", species.nodeCount, species.muscleCount);
			std::string countLabel = std::format(": {}", speciesCount);
			Vector2 speciesLabelSize = MeasureTextEx(defaultFont, speciesLabel.c_str(), labelFontSize, labelSpacing);
			Vector2 countLabelSize = MeasureTextEx(defaultFont, countLabel.c_str(), labelFontSize, labelSpacing);
			float labelWidth = speciesLabelSize.x + countLabelSize.x;
			float labelHeight = std::max(speciesLabelSize.y, countLabelSize.y);

			float bandBottom = plotTop + plotHeight - ((float)previousCumulativeCount / total) * plotHeight;
			float bandTop = plotTop + plotHeight - ((float)cumulativeCount / total) * plotHeight;
			float labelX = xLine + labelGap;
			if (labelX + labelWidth + 2.0f * labelPadding > plotLeft + plotWidth) {
				labelX = xLine - labelGap - labelWidth - 2.0f * labelPadding;
			}
			labelX = std::clamp(labelX, plotLeft + labelPadding, plotLeft + plotWidth - labelWidth - labelPadding);

			float labelY = (bandTop + bandBottom - labelHeight) * 0.5f;
			labelY = std::clamp(labelY, plotTop + labelPadding, plotTop + plotHeight - labelHeight - labelPadding);
			Rectangle labelBackground = {
				labelX - labelPadding,
				labelY - labelPadding,
				labelWidth + 2.0f * labelPadding,
				labelHeight + 2.0f * labelPadding
			};

			DrawRectangleRec(labelBackground, BLACK);
			DrawRectangleLinesEx(labelBackground, std::max(1.0f, UIScale()), { 100, 100, 100, 255 });
			if (speciesCount == pluralityCount) {
				Rectangle pluralityOutline = {
					labelBackground.x - 2.0f * UIScale(),
					labelBackground.y - 2.0f * UIScale(),
					labelBackground.width + 4.0f * UIScale(),
					labelBackground.height + 4.0f * UIScale()
				};
				DrawRectangleLinesEx(pluralityOutline, std::max(2.0f, 2.0f * UIScale()), WHITE);
			}
			DrawGradientText(speciesLabel, { labelX, labelY }, labelFontSize, labelSpacing, colors.first, colors.second);
			DrawTextEx(defaultFont, countLabel.c_str(), { labelX + speciesLabelSize.x, labelY }, labelFontSize, labelSpacing, WHITE);
		}
	}
}

void SpeciesGraph::addGeneration(int generation, const Creature* creatures, int creatureCount)
{
	std::unordered_map<uint64_t, SpeciesCount> speciesCounts;
	for (int i = 0; i < creatureCount; ++i) {
		int nodeCount = creatures[i].nodeCount;
		int muscleCount = creatures[i].muscleCount;
		uint64_t key = MakeSpeciesKey(nodeCount, muscleCount);
		SpeciesCount& species = speciesCounts[key];
		species.nodeCount = nodeCount;
		species.muscleCount = muscleCount;
		species.count++;
	}

	std::vector<SpeciesCount> generationData;
	generationData.reserve(speciesCounts.size());
	for (const auto& [key, species] : speciesCounts) {
		generationData.push_back(species);
	}
	std::sort(generationData.begin(), generationData.end(), [](const SpeciesCount& a, const SpeciesCount& b) {
		if (a.count != b.count) return a.count > b.count;
		if (a.nodeCount != b.nodeCount) return a.nodeCount < b.nodeCount;
		return a.muscleCount < b.muscleCount;
		});

	storedGenerations.push_back(generation);
	data.push_back(std::move(generationData));
}

void SpeciesGraph::compressGraph()
{
	if (data.size() <= maxDataPoints) {
		return;
	}

	size_t stride = std::max<size_t>(1, data.size() / maxDataPoints);
	std::vector<int> compressedGenerations;
	std::vector<std::vector<SpeciesCount>> compressedData;
	compressedGenerations.reserve(maxDataPoints + 1);
	compressedData.reserve(maxDataPoints + 1);

	for (size_t i = 0; i < data.size(); i += stride) {
		if (i < storedGenerations.size()) {
			compressedGenerations.push_back(storedGenerations[i]);
		}
		compressedData.push_back(data[i]);
	}

	storedGenerations = std::move(compressedGenerations);
	data = std::move(compressedData);
	if (!storedGenerations.empty()) {
		firstGeneration = storedGenerations.front();
	}
}
