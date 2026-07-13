#pragma once

#include <string>
#include <raylib.h>
#include <vector>
#include <memory>
#include <mutex>

struct Notice {
	std::string text;
	float duration;
};

struct AppSettings {
	int musicVolume = 50;
	int soundVolume = 50;
	bool fullscreen = false;
	bool showGenerationsPerSecond = true;
	int maxThreadCount = 0;
};

class World; // forward declaration

extern std::unique_ptr<World> world;

inline std::vector<Notice> notices;
inline std::mutex noticesMutex;
inline AppSettings appSettings;

inline void PushNotice(const std::string& text, float duration)
{
	std::lock_guard<std::mutex> lock(noticesMutex);
	notices.push_back({ text, duration });
}

inline void ClearNotices()
{
	std::lock_guard<std::mutex> lock(noticesMutex);
	notices.clear();
}

inline bool use_POT_textures = false;

inline unsigned int numberOfThreads = 1;
inline unsigned int hardwareThreadCount = 1;

inline constexpr int FRAMES_PER_SECOND = 60;

inline constexpr int absoluteWidth = 1024;
inline constexpr int absoluteHeight = 768;

inline constexpr int minWidth = 960; // minimum window width
inline constexpr int minHeight = 768; // minimum window height
inline constexpr float minAspectRatio = 1.25f; // 5:4 aspect ratio

inline int screenWidth = absoluteWidth;
inline int screenHeight = absoluteHeight;

inline float screenWidthRatio = 1;
inline float screenHeightRatio = 1;  

inline constexpr int ticksPerSecond = 100;
inline constexpr int secondsPerSimulation = 15;

inline constexpr int nodeCount = 3;
inline constexpr int muscleCount = 3;
inline constexpr int gravityConst = 98;
inline constexpr int numOfCreatures = 1000;

extern const char* versionString;

extern Font defaultFont;

inline bool inputActive = true;

inline float guiScale = 1.0f;
inline bool justResized = false;

enum class UIAnchor {
	TopLeft,
	Center,
	CenterHorizontal
};

inline constexpr Color PinkWORLD = { 255 , 196 , 240 , 255 };
inline constexpr Color LightBlueWORLD = { 173 , 216 , 230 , 255 };
inline constexpr Color BeigeWORLD = { 240 , 188 , 120 , 255 };

// helper functions

Vector2 UIToScreen(float x, float y);
float UIScale();
float UIFontSize(float fontScale);
float UISpacing(float fontScale);
Vector2 MeasureUIText(const std::string& text, float fontScale);

void DrawTextUI(const std::string& text, float x, float y, float fontScale, Color color, UIAnchor anchor = UIAnchor::TopLeft);
int NextPowerOfTwo(int value);
void DrawGradientText(const std::string& text, Vector2 position, float fontSize, float spacing, Color color1, Color color2);
void ClearGradientTextCache();
Rectangle DrawRectUI(float x, float y, float width, float height, Color color, UIAnchor anchor = UIAnchor::TopLeft, float lineThickness = 0.0f);
Rectangle DrawGradientUI(float x, float y, float width, float height, Color color1, Color color2, UIAnchor anchor = UIAnchor::TopLeft);
void DrawLineUI(float x1, float y1, float x2, float y2, Color color, float thickness = 1.0f);

Color ColorFromInt(int hexColor);