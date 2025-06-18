#include "Button.h"

void Button::draw()
{
	drawRect = DrawRectangleCentered(x, y, width, height, LIGHTGRAY);
	DrawRectangleCenteredLines(x, y, width, height, 1, BLACK);
	DrawTextCentered(name, x, y , 1, BLACK);
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
