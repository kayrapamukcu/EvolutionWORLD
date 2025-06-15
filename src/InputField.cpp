#include "InputField.h"
#include "Helper.h"

void InputField::onHover()
{
	return;
}

void InputField::tick()
{
	int key = GetCharPressed();

    while (key > 0)
    {
        if ((key >= 32) && (key <= 125) && (text.length() < maxChars))
        {
			text += key;
        }
        key = GetCharPressed();
    }

    if (IsKeyDown(KEY_BACKSPACE))
    {
		if (keyPressFrameCounter == FRAMES_TO_CONTINUOUSLY_PRESS) {
			if (text.length() > 0)
			{
				text.pop_back();
			}
		} 
		keyPressFrameCounter--;
		if (keyPressFrameCounter < 0) {
			if (text.length() > 0)
			{
				text.pop_back();
			}
		}
	}
	else {
		keyPressFrameCounter = FRAMES_TO_CONTINUOUSLY_PRESS;
	}
}

void InputField::draw()
{
	if (active) {
		DrawRectangleCentered(x, y, width, height, WHITE);
		drawRect = DrawRectangleCenteredLines(x, y, width, height, 1, RED);
		DrawTextCentered(text, x, y, 1, BLACK);
	}
	else {
		DrawRectangleCentered(x, y, width, height, WHITE);
		drawRect = DrawRectangleCenteredLines(x, y, width, height, 1, BLACK);
		DrawTextCentered(text, x, y, 1, DARKGRAY);
	}
	DrawTextCenteredHorizontal(name, x, drawRect.y - 10 * guiScale, 1, BLACK);
	
}

void InputField::checkCollision()
{
	if (CheckCollisionPointRec(GetMousePosition(), drawRect)) {
		if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
			active = true;
		}
	}
}

