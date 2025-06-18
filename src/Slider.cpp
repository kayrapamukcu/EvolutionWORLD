#include "Slider.h"
#include <iostream>
#include <algorithm>

void Slider::draw()
{
	drawRect = DrawRectangleCentered(x, y, width, height, DARKGRAY);
	DrawRectangleCenteredLines(x, y, width, height, 1, BLACK);

	float sliderStart = x * screenWidthRatio - width * guiScale / 2 + (NUB_WIDTH * guiScale) / 2;
	float sliderEnd = x * screenWidthRatio + width * guiScale / 2 - (NUB_WIDTH * guiScale) / 2;

	sliderPos = sliderStart + static_cast<float>(curVal - minVal) / (maxVal - minVal) * (sliderEnd - sliderStart);

	DrawRectangle(sliderPos - NUB_WIDTH * guiScale / 2 + guiScale, y * screenHeightRatio - height * guiScale / 2 + guiScale, NUB_WIDTH * guiScale - 2*guiScale, height * guiScale - 2*guiScale, LIGHTGRAY);

	DrawTextCentered(name + ": " + std::to_string(curVal), x, y, 1, WHITE);
}

void Slider::tick()
{
	if (CheckCollisionPointRec(GetMousePosition(), drawRect)) {
		if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
			active = true;
		}
		else if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
			precise = true;
		}
	}
	if (!active && !precise) {
		return;
	}
	if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
		active = false;
	}
	if (IsMouseButtonReleased(MOUSE_RIGHT_BUTTON)) {
		precise = false;
	}
	float mouseX = GetMousePosition().x;
	float sliderStart = x * screenWidthRatio - width * guiScale / 2 + (NUB_WIDTH * guiScale) / 2;
	float sliderEnd = x * screenWidthRatio + width * guiScale / 2 - (NUB_WIDTH * guiScale) / 2;
	float xClicked = std::clamp(mouseX, sliderStart, sliderEnd);
	if (active) {
		curVal = minVal + static_cast<int>((xClicked - sliderStart) / (sliderEnd - sliderStart) * (maxVal - minVal));
	}
	if (precise) {
		if (GetTime() - lastAdjustmentTime < 0.1) {
			return;
		}
		float sliderPosClicked = sliderStart + static_cast<float>(minVal + static_cast<int>((xClicked - sliderStart) / (sliderEnd - sliderStart) * (maxVal - minVal)) - minVal) / (maxVal - minVal) * (sliderEnd - sliderStart);
		if (sliderPos > sliderPosClicked) {
			curVal = std::max(minVal, curVal - 1);
		}
		else if (sliderPos < sliderPosClicked) {
			curVal = std::min(maxVal, curVal + 1);
		}
		lastAdjustmentTime = GetTime();
	}
}

std::string Slider::getContent()
{
	return std::to_string(curVal);
}
