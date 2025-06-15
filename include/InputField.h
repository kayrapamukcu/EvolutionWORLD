#pragma once

#include "raylib.h"
#include "Helper.h"
#include <string>

class InputField {
public:
	static constexpr int FRAMES_TO_CONTINUOUSLY_PRESS = FRAMES_PER_SECOND / 2;
	int keyPressFrameCounter = FRAMES_TO_CONTINUOUSLY_PRESS;
	int maxChars;
	int x, y, width, height, fieldID;
	std::string name;
	std::string text;
	bool active = false;
	Rectangle drawRect = { 0, 0, 0, 0 };
	
	InputField(int id, int xPos, int yPos, int w, int h, const std::string& buttonText)
		: fieldID(id), x(xPos), y(yPos), width(w), height(h), name(buttonText), maxChars(16) {
	}
	InputField(int id, int xPos, int yPos, int w, int h, const std::string& buttonText, const std::string& defaultText, int ch)
		: fieldID(id), x(xPos), y(yPos), width(w), height(h), name(buttonText), text(defaultText), maxChars(ch) {
	}
	void onHover();
	void tick();
	void draw();
	void checkCollision();
};