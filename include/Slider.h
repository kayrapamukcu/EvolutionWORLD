#pragma once

#include <string>
#include "raylib.h"
#include "Helper.h"
#include "UIElement.h"

class Slider : public UIElement {
public:
	static constexpr int NUB_WIDTH = 8;
	int minVal, maxVal, curVal;
	bool precise = false;
	float sliderPos;
	double lastAdjustmentTime = 0.0;

	Slider(int id, int xPos, int yPos, int w, int h, const std::string& buttonText, int mnVal, int mxVal, int crVal) {
		elementID = id;
		x = xPos;
		y = yPos;
		width = w;
		height = h;
		name = buttonText;
		minVal = mnVal;
		maxVal = mxVal;
		curVal = crVal;
		active = false;
		sliderPos = 0.0f; // Initialize slider position
	}

	void draw() override;
	void tick() override;
	std::string getContent() override;
};