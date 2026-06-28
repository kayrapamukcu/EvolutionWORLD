#include "Button.h"

void Button::draw()
{
	drawRect = DrawRectUI(x, y, width, height, LIGHTGRAY, UIAnchor::Center);
	DrawRectUI(x, y, width, height, BLACK, UIAnchor::Center, 2);
	DrawTextUI(name, x, y, 1, BLACK, UIAnchor::Center);
}

void Button::tick()
{
	if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
		if (CheckCollisionPointRec(GetMousePosition(), drawRect)) {
			active = true;
		}
		else {
			active = false;
		}
	}
	else {
		active = false;
	}
}

std::string Button::getContent()
{
	return name;
}
