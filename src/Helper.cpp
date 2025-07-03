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

void DrawTextC(const std::string& text, int x, int y, int fontScale, Color color)
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

Rectangle DrawRectangleLinesB(int x, int y, int width, int height, int thickness, Color color)
{
	auto w = width * guiScale;
	auto h = height * guiScale;
	DrawRectangleLinesEx({ x * screenWidthRatio, y * screenHeightRatio, float(w), float(h) }, thickness * guiScale, color);
	return { x * screenWidthRatio, y * screenHeightRatio, float(w), float(h) };
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

void DrawLineB(int x1, int y1, int x2, int y2, Color color)
{
	DrawLineEx({ x1 * screenWidthRatio, y1 * screenHeightRatio }, { x1 * screenWidthRatio + (x2 - x1) , y1 * screenHeightRatio + (y2 - y1) }, 1.0f * guiScale, color);
}

Color ColorFromInt(int color)
{
	return { (unsigned char)((color >> 16) & 0xFF), (unsigned char)((color >> 8) & 0xFF), (unsigned char)(color & 0xFF), 255 };
}