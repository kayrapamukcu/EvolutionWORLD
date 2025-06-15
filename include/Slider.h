#pragma once

#include <string>
#include "raylib.h"
#include "Helper.h"

class Slider {
public:
	static constexpr int NUB_WIDTH = 8;
	int sliderID;
	int x, y, width, height, minVal, maxVal, curVal;
	std::string text;
	bool active = false;
	bool precise = false;
	Rectangle drawRect;
	float sliderPos;
	double lastAdjustmentTime = 0.0;
	Slider(int id, int xPos, int yPos, int w, int h, const std::string& buttonText, int mnVal, int mxVal, int crVal)
		: sliderID(id), x(xPos), y(yPos), width(w), height(h), text(buttonText), maxVal(mxVal), minVal(mnVal), curVal(crVal), sliderPos(0){
	}
	void tick();
	void onHover();
	void onClicked();
	void draw();
	void checkCollision();
};