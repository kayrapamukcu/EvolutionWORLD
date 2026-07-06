#include "Slider.h"
#include <iostream>
#include <algorithm>

void Slider::draw()
{
	drawRect = DrawRectUI(x, y, width, height, DARKGRAY, UIAnchor::Center);
	DrawRectUI(x, y, width, height, BLACK, UIAnchor::Center, 2);

	float sliderStart = drawRect.x + (NUB_WIDTH * UIScale()) * 0.5f;
	float sliderEnd = drawRect.x + drawRect.width - (NUB_WIDTH * UIScale()) * 0.5f;
	curVal = std::clamp(curVal, minVal, maxVal);
	const int valueRange = maxVal - minVal;

	if (valueRange <= 0) {
		sliderPos = sliderStart;
	}
	else {
		sliderPos = sliderStart + static_cast<float>(curVal - minVal) / valueRange * (sliderEnd - sliderStart);
	}

	DrawRectangle(
		sliderPos - NUB_WIDTH * UIScale() * 0.5f + UIScale(),
		drawRect.y + UIScale(),
		NUB_WIDTH * UIScale() - 2 * UIScale(),
		drawRect.height - 2 * UIScale(),
		LIGHTGRAY);

	if (enabled) {
		DrawTextUI(name + ": " + std::to_string(curVal), x, y, 1, WHITE, UIAnchor::Center);
	}
	else {
		DrawRectUI(x, y, width, height, Fade(LIGHTGRAY, 0.65f), UIAnchor::Center);
		DrawRectUI(x, y, width, height, BLACK, UIAnchor::Center, 2);
		DrawTextUI(name + ": " + std::to_string(curVal), x, y, 1, GRAY, UIAnchor::Center);
	}
}

void Slider::tick()
{
	if (!enabled) {
		active = false;
		precise = false;
		return;
	}
	if (CheckCollisionPointRec(GetMousePosition(), drawRect)) {
		if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
			active = true;
		}
		else if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
			active = true;
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
		active = false;
		precise = false;
	}
	float mouseX = GetMousePosition().x;
	float sliderStart = drawRect.x + (NUB_WIDTH * UIScale()) * 0.5f;
	float sliderEnd = drawRect.x + drawRect.width - (NUB_WIDTH * UIScale()) * 0.5f;
	float xClicked = std::clamp(mouseX, sliderStart, sliderEnd);
	curVal = std::clamp(curVal, minVal, maxVal);
	const int valueRange = maxVal - minVal;
	if (valueRange <= 0) {
		curVal = minVal;
		active = false;
		precise = false;
		return;
	}
	
	if (precise) {
		if (GetTime() - lastAdjustmentTime < 0.1) {
			return;
		}
		float sliderPosClicked = sliderStart + static_cast<float>(minVal + static_cast<int>((xClicked - sliderStart) / (sliderEnd - sliderStart) * valueRange) - minVal) / valueRange * (sliderEnd - sliderStart);
		if (sliderPos > sliderPosClicked) {
			curVal = std::max(minVal, curVal - 1);
		}
		else if (sliderPos < sliderPosClicked) {
			curVal = std::min(maxVal, curVal + 1);
		}
		lastAdjustmentTime = GetTime();
	}
	else if (active) {
		curVal = minVal + static_cast<int>((xClicked - sliderStart) / (sliderEnd - sliderStart) * valueRange);
		curVal = std::clamp(curVal, minVal, maxVal);
	}
}

std::string Slider::getContent()
{
	return std::to_string(curVal);
}
