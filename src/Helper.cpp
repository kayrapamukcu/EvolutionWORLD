#include "Helper.h"

#include <algorithm>

Font defaultFont;

Vector2 UIToScreen(float x, float y)
{
	return { x * screenWidthRatio, y * screenHeightRatio };
}

float UIScale()
{
	return guiScale;
}

float UIFontSize(float fontScale)
{
	return defaultFont.baseSize * fontScale * 0.5f * UIScale();
}

float UISpacing(float fontScale)
{
	return fontScale * 0.5f * UIScale();
}

Vector2 MeasureUIText(const std::string& text, float fontScale)
{
	return MeasureTextEx(defaultFont, text.c_str(), UIFontSize(fontScale), UISpacing(fontScale));
}

void DrawTextUI(const std::string& text, float x, float y, float fontScale, Color color, UIAnchor anchor)
{
	Vector2 position = UIToScreen(x, y);
	const Vector2 textSize = MeasureUIText(text, fontScale);

	if (anchor == UIAnchor::Center || anchor == UIAnchor::CenterHorizontal) {
		position.x -= textSize.x * 0.5f;
	}
	if (anchor == UIAnchor::Center) {
		position.y -= textSize.y * 0.5f;
	}

	DrawTextEx(defaultFont, text.c_str(), position, UIFontSize(fontScale), UISpacing(fontScale), color);
}

Rectangle DrawRectUI(float x, float y, float width, float height, Color color, UIAnchor anchor, float lineThickness)
{
	Vector2 position = UIToScreen(x, y);
	const float scaledWidth = width * screenWidthRatio;
	const float scaledHeight = height * screenHeightRatio;

	if (anchor == UIAnchor::Center) {
		position.x -= scaledWidth * 0.5f;
		position.y -= scaledHeight * 0.5f;
	}
	else if (anchor == UIAnchor::CenterHorizontal) {
		position.x -= scaledWidth * 0.5f;
	}

	Rectangle rect = { position.x, position.y, scaledWidth, scaledHeight };
	if (lineThickness > 0.0f) {
		DrawRectangleLinesEx(rect, std::max(1.0f, lineThickness * UIScale()), color);
	}
	else {
		DrawRectangleRec(rect, color);
	}

	return rect;
}

void DrawLineUI(float x1, float y1, float x2, float y2, Color color, float thickness)
{
	DrawLineEx(UIToScreen(x1, y1), UIToScreen(x2, y2), std::max(1.0f, thickness * UIScale()), color);
}

Color ColorFromInt(int color)
{
	return { (unsigned char)((color >> 16) & 0xFF), (unsigned char)((color >> 8) & 0xFF), (unsigned char)(color & 0xFF), 255 };
}
