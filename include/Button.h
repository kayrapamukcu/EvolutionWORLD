#pragma once

#include <string>
#include "raylib.h"
#include "Helper.h"
#include "UIElement.h"

class Button : public UIElement {
public:
	Button(int id, int xPos, int yPos, int w, int h, const std::string& buttonText) {
		elementID = id;
		x = xPos;
		y = yPos;
		width = w;
		height = h;
		name = buttonText;
		active = false;
	}

	void draw() override;
	void tick() override;
	std::string getContent() override;
};