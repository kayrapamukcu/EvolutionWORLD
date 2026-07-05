#include "Helper.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <unordered_map>

#include "rlgl.h"

Font defaultFont;

namespace {
	struct GradientTextCacheEntry {
		RenderTexture2D texture;
		RenderTexture2D mask;
		int width;
		int height;
	};

	std::unordered_map<std::string, GradientTextCacheEntry> gradientTextCache;

	uint32_t PackColor(Color color)
	{
		return (static_cast<uint32_t>(color.r) << 24)
			| (static_cast<uint32_t>(color.g) << 16)
			| (static_cast<uint32_t>(color.b) << 8)
			| static_cast<uint32_t>(color.a);
	}

	std::string MakeGradientTextCacheKey(const std::string& text, float fontSize, float spacing, Color color1, Color color2, int width, int height)
	{
		return text
			+ "|" + std::to_string((int)std::lround(fontSize * 1000.0f))
			+ "|" + std::to_string((int)std::lround(spacing * 1000.0f))
			+ "|" + std::to_string(PackColor(color1))
			+ "|" + std::to_string(PackColor(color2))
			+ "|" + std::to_string(width)
			+ "|" + std::to_string(height);
	}
}

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

void DrawGradientText(const std::string& text, Vector2 position, float fontSize, float spacing, Color color1, Color color2)
{
	Vector2 textSize = MeasureTextEx(defaultFont, text.c_str(), fontSize, spacing);
	const int textureWidth = (int)std::ceil(textSize.x);
	const int textureHeight = (int)std::ceil(textSize.y);

	if (textureWidth <= 0 || textureHeight <= 0) {
		return;
	}

	const std::string cacheKey = MakeGradientTextCacheKey(text, fontSize, spacing, color1, color2, textureWidth, textureHeight);
	auto it = gradientTextCache.find(cacheKey);
	if (it == gradientTextCache.end()) {
		RenderTexture2D textMask = LoadRenderTexture(textureWidth, textureHeight);
		RenderTexture2D gradientText = LoadRenderTexture(textureWidth, textureHeight);

		BeginTextureMode(textMask);
		ClearBackground(BLANK);
		DrawTextEx(defaultFont, text.c_str(), { 0.0f, 0.0f }, fontSize, spacing, WHITE);
		EndTextureMode();

		BeginTextureMode(gradientText);
		ClearBackground(BLANK);
		DrawRectangleGradientV(0, 0, textureWidth, textureHeight, color2, color1);
		rlSetBlendFactorsSeparate(RL_ZERO, RL_ONE, RL_ZERO, RL_SRC_ALPHA, RL_FUNC_ADD, RL_FUNC_ADD);
		BeginBlendMode(BLEND_CUSTOM_SEPARATE);
		DrawTextureRec(textMask.texture, { 0.0f, 0.0f, (float)textureWidth, -(float)textureHeight }, { 0.0f, 0.0f }, WHITE);
		EndBlendMode();
		EndTextureMode();

		it = gradientTextCache.emplace(cacheKey, GradientTextCacheEntry{ gradientText, textMask, textureWidth, textureHeight }).first;
	}

	const GradientTextCacheEntry& cachedText = it->second;
	DrawTextureRec(cachedText.texture.texture, { 0.0f, 0.0f, (float)cachedText.width, -(float)cachedText.height }, position, WHITE);
}

void ClearGradientTextCache()
{
	rlDrawRenderBatchActive();
	for (auto& cachedText : gradientTextCache) {
		UnloadRenderTexture(cachedText.second.texture);
		UnloadRenderTexture(cachedText.second.mask);
	}
	gradientTextCache.clear();
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

Rectangle DrawGradientUI(float x, float y, float width, float height, Color color1, Color color2, UIAnchor anchor)
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
	DrawRectangleGradientV((int)rect.x, (int)rect.y, (int)rect.width, (int)rect.height, color2, color1);
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
