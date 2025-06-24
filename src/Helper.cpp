#include "Helper.h"

Font defaultFont;

void DrawTextCentered(const std::string& text, int x, int y, int fontScale, Color color) {
	
	 Vector2 textSize = MeasureTextEx(defaultFont, text.c_str(), defaultFont.baseSize * fontScale * guiScale, fontScale * guiScale);
	 DrawTextEx(defaultFont, text.c_str(), { x * screenWidthRatio - textSize.x / 2, y * screenHeightRatio - textSize.y / 2 } , defaultFont.baseSize * fontScale * guiScale, fontScale * guiScale, color);
}

void DrawTextCenteredHorizontal(const std::string& text, int x, int y, int fontScale, Color color) {

	Vector2 textSize = MeasureTextEx(defaultFont, text.c_str(), defaultFont.baseSize * fontScale * guiScale, fontScale * guiScale);
	DrawTextEx(defaultFont, text.c_str(), { x * screenWidthRatio - textSize.x / 2, y - textSize.y / 2 }, defaultFont.baseSize * fontScale * guiScale, fontScale * guiScale, color);
}

void DrawTextB(const std::string& text, int x, int y, int fontScale, Color color)
{
	DrawTextEx(defaultFont, text.c_str(), { x * screenWidthRatio, y * screenHeightRatio }, defaultFont.baseSize * fontScale * guiScale, fontScale * guiScale, color);
}

void DrawTextCenteredNoScale(const std::string& text, int x, int y, int fontSize, Color color)
{
	Vector2 textSize = MeasureTextEx(defaultFont, text.c_str(), defaultFont.baseSize * fontSize, fontSize);
	DrawTextEx(defaultFont, text.c_str(), { x - textSize.x / 2, y - textSize.y / 2 }, defaultFont.baseSize * fontSize, fontSize, color);
}

Rectangle DrawRectangleB(int x, int y, int w, int h, Color color)
{
	auto width = w * guiScale;
	auto height = h * guiScale;
	DrawRectangle(x * screenWidthRatio, y * screenHeightRatio, width, height, color);
	return { x * screenWidthRatio, y * screenHeightRatio, float(width), float(height) };
}

Rectangle DrawRectangleCentered(int xCenter, int yCenter, int w, int h, Color color)
{
	auto width = w * guiScale;
	auto height = h * guiScale;
	DrawRectangle(xCenter * screenWidthRatio - width / 2, yCenter * screenHeightRatio - height / 2, width, height, color);
	return { xCenter * screenWidthRatio - width / 2, yCenter * screenHeightRatio - height / 2, float(width), float(height) };
}

Rectangle DrawRectangleCenteredLines(int xCenter, int yCenter, int w, int h, int thickness, Color color)
{
	auto width = w * guiScale;
	auto height = h * guiScale;
	DrawRectangleLinesEx({ xCenter * screenWidthRatio - width / 2, yCenter * screenHeightRatio - height / 2, float(width), float(height) }, thickness * guiScale, color);
	return { xCenter * screenWidthRatio - width / 2, yCenter * screenHeightRatio - height / 2, float(width), float(height) };
}

Color ColorFromInt(int color)
{
	return { (unsigned char)((color >> 16) & 0xFF), (unsigned char)((color >> 8) & 0xFF), (unsigned char)(color & 0xFF), 255 };
}