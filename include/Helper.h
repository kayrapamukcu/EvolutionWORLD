#pragma once

#include <string>
#include <raylib.h>

inline unsigned int numberOfThreads = 1;

inline constexpr int FRAMES_PER_SECOND = 60;

inline constexpr int absoluteWidth = 854;
inline constexpr int absoluteHeight = 480;

inline int screenWidth = absoluteWidth;
inline int screenHeight = absoluteHeight;

inline float screenWidthRatio = 1;
inline float screenHeightRatio = 1;  

inline constexpr int ticksPerSecond = 100;
inline constexpr int secondsPerSimulation = 15;

inline constexpr int nodeCount = 3;
inline constexpr int muscleCount = 3;
inline constexpr int ticksToSwitchMuscleStage = 100;

inline constexpr int numOfCreatures = 1000;

inline constexpr auto versionString = "v0.1.1";

extern Font defaultFont;

inline bool inputActive = true;
inline int guiScale = 2;


inline constexpr Color PinkWORLD = { 255 , 196 , 240 , 255 };
inline constexpr Color LightBlueWORLD = { 173 , 216 , 230 , 255 };

// helper functions

void DrawTextCentered(const std::string& text, int x, int y, int fontSize, Color color);
void DrawTextCenteredHorizontal(const std::string& text, int x, int y, int fontSize, Color color);
void DrawTextB(const std::string& text, int x, int y, int fontSize, Color color);
Rectangle DrawRectangleB(int x, int y, int width, int height, Color color);
Rectangle DrawRectangleCentered(int x, int y, int width, int height, Color color);
Rectangle DrawRectangleCenteredLines(int x, int y, int width, int height, int thickness, Color color);