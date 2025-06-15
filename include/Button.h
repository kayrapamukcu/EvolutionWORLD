#pragma once

#include <string>
#include "raylib.h"
#include "Helper.h"

class Button {
public:
	int buttonID;
	int x, y, width, height;
	std::string text;
	Rectangle drawRect;

	Button(int id, int xPos, int yPos, int w, int h, const std::string& buttonText)
		: buttonID(id), x(xPos), y(yPos), width(w), height(h), text(buttonText) {
	}
	void onHover();
	int onClicked();
	void draw();
	bool checkCollision();
};