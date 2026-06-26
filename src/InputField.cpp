#include "InputField.h"
#include "Helper.h"

namespace {
	bool IsAcceptedInputCodepoint(int codepoint)
	{
		return codepoint >= 32 && codepoint != 127;
	}

	void AppendCodepointAsUtf8(std::string& text, int codepoint)
	{
		int utf8Size = 0;
		const char* utf8 = CodepointToUTF8(codepoint, &utf8Size);
		if (utf8 != nullptr && utf8Size > 0) {
			text.append(utf8, utf8Size);
		}
	}

	void EraseLastUtf8Codepoint(std::string& text)
	{
		if (text.empty()) return;

		size_t start = text.size() - 1;
		while (start > 0 && (static_cast<unsigned char>(text[start]) & 0xC0) == 0x80) {
			start--;
		}

		text.erase(start);
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

void InputField::tick()
{
	if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
		if (CheckCollisionPointRec(GetMousePosition(), drawRect)) {
			active = true;
		}
		else {
			active = false;
		}
	}
	if (!active) return;

	int key = GetCharPressed();
	while (key > 0) {
		if (IsAcceptedInputCodepoint(key) && GetCodepointCount(text.c_str()) < maxCodepoints) {
			AppendCodepointAsUtf8(text, key);
		}
		key = GetCharPressed();
	}

	if (IsKeyDown(KEY_BACKSPACE)) {
		if (keyPressFrameCounter == FRAMES_TO_CONTINUOUSLY_PRESS && !text.empty()) {
			EraseLastUtf8Codepoint(text);
		}
		keyPressFrameCounter--;
		if (keyPressFrameCounter < 0 && !text.empty()) {
			EraseLastUtf8Codepoint(text);
		}
	}
	else {
		keyPressFrameCounter = FRAMES_TO_CONTINUOUSLY_PRESS;
	}
}

std::string InputField::getContent()
{
	return text;
}
