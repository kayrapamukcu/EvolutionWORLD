#pragma once

#include <string>
#include <raylib.h>

inline unsigned int numberOfThreads = 1;

inline constexpr int FRAMES_PER_SECOND = 60;

inline constexpr int absoluteWidth = 854;
inline constexpr int absoluteHeight = 480;

inline constexpr int minWidth = 800; // minimum window width
inline constexpr int minHeight = 480; // minimum window height
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

inline constexpr auto versionString = "v1.2.1";

extern Font defaultFont;

inline bool inputActive = true;

inline int guiScale = 2;
inline bool justResized = false;

inline constexpr Color PinkWORLD = { 255 , 196 , 240 , 255 };
inline constexpr Color LightBlueWORLD = { 173 , 216 , 230 , 255 };
inline constexpr Color BeigeWORLD = { 240 , 188 , 120 , 255 };

// helper functions

void DrawTextCentered(const std::string& text, int x, int y, int fontSize, Color color);
void DrawTextCenteredHorizontal(const std::string& text, int x, int y, int fontSize, Color color);
void DrawTextB(const std::string& text, int x, int y, int fontSize, Color color);
void DrawTextC(const std::string& text, int x, int y, int fontSize, Color color);
void DrawTextCenteredNoScale(const std::string& text, int x, int y, int fontSize, Color color);
Rectangle DrawRectangleB(int x, int y, int width, int height, Color color);
Rectangle DrawRectangleLinesB(int x, int y, int width, int height, int thickness, Color color);
Rectangle DrawRectangleCentered(int x, int y, int width, int height, Color color);
Rectangle DrawRectangleCenteredLines(int x, int y, int width, int height, int thickness, Color color);

void DrawLineB(int x1, int y1, int x2, int y2, Color color);

Color ColorFromInt(int hexColor);