#pragma once

#include "raylib.h"
#include "Helper.h"
#include <string>
#include "UIElement.h"

class InputField : public UIElement {
public:
	static constexpr int FRAMES_TO_CONTINUOUSLY_PRESS = FRAMES_PER_SECOND / 2;
	int keyPressFrameCounter = FRAMES_TO_CONTINUOUSLY_PRESS;
	int maxChars;
	std::string text;

	InputField(int id, int xPos, int yPos, int w, int h, const std::string& buttonText) {
		elementID = id;
		x = xPos;
		y = yPos;
		width = w;
		height = h;
		name = buttonText;
		text = "";
		maxChars = 16; // Default max characters
	}
	InputField(int id, int xPos, int yPos, int w, int h, const std::string& buttonText, const std::string& defaultText, int ch) {
		elementID = id;
		x = xPos;
		y = yPos;
		width = w;
		height = h;
		name = buttonText;
		text = defaultText;
		maxChars = ch;
		active = false;
	}
	void draw() override;
	void tick() override;
	std::string getContent() override;
};