#include "Button.h"

void Button::onHover()
{
	return;
}
int Button::onClicked()
{
	return 0;
}

void Button::draw()
{
	drawRect = DrawRectangleCentered(x, y, width, height, LIGHTGRAY);
	DrawRectangleCenteredLines(x, y, width, height, 1, BLACK);
	DrawTextCentered(text, x, y , 1, BLACK);
}

bool Button::checkCollision()
{
	if (CheckCollisionPointRec(GetMousePosition(), drawRect)) {
		onHover();
		if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
			onClicked();
			return true;
		}
	}
	return false;
}
