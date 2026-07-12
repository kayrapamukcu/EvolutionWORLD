
#include <iostream>
#include "Creature.h"
#include "raylib.h"
#include "Helper.h"
#include <array>
#include <algorithm>
#include "Button.h"
#include "InputField.h"
#include <vector>
#include "World.h"
#include "Slider.h"
#include "UIElement.h"
#include "tinyfiledialogs.h"
#include <thread>
#include <atomic>
#include <chrono>
#include <string>
#include <format>
#include <future>
#include <fstream>
#include <cstdint>
#include <cmath>

float guiScalePrev = 1.0f;
std::unique_ptr<World> world;
const char* versionString = "v2.0.0";

constexpr int32_t settingsFileMagic = 0x53545745; // EWTS
constexpr int32_t settingsFileVersion = 1;
constexpr int32_t settingsFieldCount = 5;

std::unordered_map<uint64_t, std::pair<Color, Color>> speciesColorMap;

enum State {
	STATE_MENU_INIT,
	STATE_MENU,
	STATE_OPTIONS,
	STATE_ABOUT,
	STATE_CREATE,
	STATE_GAME,
	STATE_SAVE,
	STATE_DRAW_CREATURE,
	STATE_VIEW_ALL
};

uint64_t makeSpeciesKey(int nodeCount, int muscleCount)
{
	return (static_cast<uint64_t>(static_cast<uint32_t>(nodeCount)) << 32)
		| static_cast<uint32_t>(muscleCount);
}

Color generateSpeciesColor(int nodeCount, int muscleCount)
{
	uint32_t hash = static_cast<uint32_t>(nodeCount) * 73856093u
		^ static_cast<uint32_t>(muscleCount) * 19349663u;

	hash ^= hash >> 16;
	hash *= 0x7feb352du;
	hash ^= hash >> 13;
	hash *= 0x846ca68bu;
	hash ^= hash >> 16;

	uint8_t r = 60 + static_cast<uint8_t>(((hash >> 16) & 0xFF) * 195 / 255);
	uint8_t g = 60 + static_cast<uint8_t>(((hash >> 8) & 0xFF) * 195 / 255);
	uint8_t b = 60 + static_cast<uint8_t>((hash & 0xFF) * 195 / 255);

	return Color{ r, g, b, 255 };
}

std::pair<Color,Color> getSpeciesColor(int nodeCount, int muscleCount)
{
	uint64_t key = makeSpeciesKey(nodeCount, muscleCount);

	auto it = speciesColorMap.find(key);
	if (it != speciesColorMap.end()) {
		return it->second;
	}

	Color newColor = generateSpeciesColor(nodeCount, muscleCount);
	Color newColor2 = generateSpeciesColor(nodeCount + 128, muscleCount + 256);
	auto newColorPair = std::pair<Color, Color>(newColor, newColor2);
	speciesColorMap.emplace(key, newColorPair);

	return newColorPair;
}

bool SaveCreatureToBinaryFile(const Creature& creature)
{
	const char* filters[] = { "*.CREATURE" };
	std::string defaultName = std::format("Creature_{}.CREATURE", creature.id);
	const char* path = tinyfd_saveFileDialog(
		"Save creature as...",
		defaultName.c_str(),
		1,
		filters,
		"Creature files"
	);

	if (!path) {
		return false;
	}

	std::ofstream out(path, std::ios::binary);
	if (!out) {
		PushNotice("Failed to save creature", 5.0f);
		return false;
	}

	writeCreature(out, creature);
	PushNotice("Creature saved", 2.0f);
	return true;
}

void SaveAppSettings()
{
	std::ofstream out("settings", std::ios::binary);
	if (!out) return;

	auto writeInt = [&out](int32_t value) {
		out.write(reinterpret_cast<const char*>(&value), sizeof(value));
		};

	writeInt(settingsFileMagic);
	writeInt(settingsFileVersion);
	writeInt(settingsFieldCount);
	writeInt(appSettings.musicVolume);
	writeInt(appSettings.soundVolume);
	writeInt(appSettings.fullscreen ? 1 : 0);
	writeInt(appSettings.showGenerationsPerSecond ? 1 : 0);
	writeInt(appSettings.maxThreadCount);
}

void LoadAppSettings()
{
	std::ifstream in("settings", std::ios::binary);
	if (!in) return;

	auto readInt = [&in](int32_t& value) {
		in.read(reinterpret_cast<char*>(&value), sizeof(value));
		return (bool)in;
		};

	int32_t magic = 0;
	int32_t version = 0;
	int32_t fieldCount = 0;
	if (!readInt(magic) || magic != settingsFileMagic) return;
	if (!readInt(version) || !readInt(fieldCount) || fieldCount < 0) return;

	for (int32_t field = 0; field < fieldCount; ++field) {
		int32_t value = 0;
		if (!readInt(value)) return;

		switch (field) {
		case 0:
			appSettings.musicVolume = std::clamp(value, 0, 100);
			break;
		case 1:
			appSettings.soundVolume = std::clamp(value, 0, 100);
			break;
		case 2:
			appSettings.fullscreen = value != 0;
			break;
		case 3:
			appSettings.showGenerationsPerSecond = value != 0;
			break;
		case 4:
			appSettings.maxThreadCount = value;
			break;
		default:
			break;
		}
	}
}

std::vector<int> CreateUIFontCodepoints() {
	std::vector<int> codepoints;
	auto addRange = [&codepoints](int first, int last) {
		for (int codepoint = first; codepoint <= last; ++codepoint) {
			codepoints.push_back(codepoint);
		}
	};

	addRange(0x0020, 0x007E); // Basic Latin
	addRange(0x00A0, 0x024F); // Latin-1 Supplement, Latin Extended-A/B
	addRange(0x0370, 0x03FF); // Greek and Coptic
	addRange(0x0400, 0x052F); // Cyrillic and Cyrillic Supplement
	addRange(0x2000, 0x206F); // General punctuation

	return codepoints;
}

void ApplyFullscreenSetting()
{
	if (appSettings.fullscreen == IsWindowFullscreen()) return;

	if (appSettings.fullscreen) {
		int monitor = GetCurrentMonitor();
		SetWindowSize(GetMonitorWidth(monitor), GetMonitorHeight(monitor));
		ToggleFullscreen();
	}
	else {
		ToggleFullscreen();
		SetWindowSize(absoluteWidth, absoluteHeight);
	}
}

void ClampWindowSize() {
	screenWidth = GetScreenWidth();
	screenHeight = GetScreenHeight();

	bool needsResize = false;

	if (screenWidth < minWidth) {
		screenWidth = minWidth;
		needsResize = true;
	}
	if (screenHeight < minHeight) {
		screenHeight = minHeight;
		needsResize = true;
	}

	float aspect = (float)screenWidth / screenHeight;
	if (aspect < minAspectRatio) {
		screenWidth = static_cast<int>(screenHeight * minAspectRatio);
		needsResize = true;
	}

	screenWidthRatio = float(screenWidth) / float(absoluteWidth);
	screenHeightRatio = float(screenHeight) / float(absoluteHeight);

	float guiScaleWidth = screenWidth / float(absoluteWidth);
	float guiScaleHeight = screenHeight / float(absoluteHeight);

	guiScale = std::min(guiScaleWidth, guiScaleHeight);

	if (guiScalePrev != guiScale) justResized = true;
	else justResized = false;

	guiScalePrev = guiScale;
	if (needsResize) {
		SetWindowSize(screenWidth, screenHeight);
	}
}

int main() {
	State currentState = STATE_MENU_INIT;
	State settingsReturnState = STATE_MENU;
	std::vector<Button> mainMenuButtons;

	std::vector<InputField> newGameInputFields;
	std::vector<Button> newGameButtons;
	std::vector<Slider> newGameSliders;

	std::vector<std::unique_ptr<UIElement>> createMenuUIElements;
	std::vector<std::unique_ptr<UIElement>> ingameUIElements;
	std::vector<std::unique_ptr<UIElement>> settingsUIElements;
	LoadAppSettings();
	
	SetConfigFlags(FLAG_MSAA_4X_HINT);
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(screenWidth, screenHeight, "EvolutionWORLD");

	SetExitKey(KEY_NULL);

	ApplyFullscreenSetting();

	SetTargetFPS(FRAMES_PER_SECOND);
	InitAudioDevice();
	Music musicMenu = LoadMusicStream("resources/mus_menu.ogg");
	std::vector<int> uiFontCodepoints = CreateUIFontCodepoints();
	defaultFont = LoadFontEx("resources/Lato-Regular.ttf", 72, uiFontCodepoints.data(), (int)uiFontCodepoints.size());

	SetMusicVolume(musicMenu, appSettings.musicVolume / 100.0f);
	SetMusicPitch(musicMenu, 0.0f);

	hardwareThreadCount = std::thread::hardware_concurrency();
	if (hardwareThreadCount == 0) {
		hardwareThreadCount = 1;
	}
	if (appSettings.maxThreadCount < 1) {
		appSettings.maxThreadCount = (int)hardwareThreadCount;
	}
	appSettings.maxThreadCount = std::clamp(appSettings.maxThreadCount, 1, (int)hardwareThreadCount);
	numberOfThreads = (unsigned int)appSettings.maxThreadCount;
	std::cout << "Processors: " << numberOfThreads << std::endl;

	MiniWorld previewWorld;

	int creatureIndexToBeDrawn = 0;
	bool drawCreatureFromCurrentPopulation = false;
	Creature selectedCurrentCreature;
	int viewAllCreaturePage = 0;
	int saveStartGeneration = 0;
	int saveEndGeneration = 0;
	int activeSaveRangeHandle = -1;
	int createMinNodes = 3;
	int createMaxNodes = 15;
	int createMinMuscles = createMinNodes;
	int createMaxMuscles = createMaxNodes * (createMaxNodes - 1) / 2;
	int activeCreateRangeSlider = -1;
	int activeCreateRangeHandle = -1;
	bool createManualValueActive = false;
	int createManualTargetKind = 0; // 1: normal slider, 2: range slider handle
	int createManualTargetId = -1;
	int createManualRangeHandle = -1;
	std::string createManualValueText;
	std::string createManualValueLabel;
	bool saveIncludeGraph = true;
	bool saveIncludeSpeciesGraph = true;
	bool confirmLeaveWorld = false;
	float watchTime = 0.0f;
	float creaturePlaybackSpeed = 1.0f;
	bool creaturePlaybackPaused = false;
	bool doGenerationsNonstop = false;
	float timeForOneGen = 0.0f;
	double generationStartTime = 0.0;
	double generationsPerSecondWindowStart = GetTime();
	int generationsCompletedInWindow = 0;
	float generationsPerSecond = 0.0f;
	bool saveInProgress = false;
	std::atomic<bool> saveWorkStarted{ false };
	std::future<void> saveFuture;
	bool loadInProgress = false;
	std::atomic<bool> loadWorkStarted{ false };
	std::future<std::unique_ptr<World>> loadFuture;
	bool createInProgress = false;
	std::future<std::unique_ptr<World>> createFuture;
	bool continuousGenerationThreadRunning = false;
	std::future<void> continuousGenerationFuture;
	std::atomic<bool> stopContinuousGenerations{ false };
	std::atomic<int> continuousGenerationsCompleted{ 0 };
	std::atomic<double> latestContinuousGenerationSeconds{ 0.0 };

	auto drawBusyPanel = [](const std::string& text, float y) {
		DrawRectUI(absoluteWidth / 2, y + 15, 360, 86, RED, UIAnchor::Center, 2.0f);
		DrawRectUI(absoluteWidth / 2, y + 15, 360, 86, { 0, 0, 0, 255 }, UIAnchor::Center);
		DrawTextUI(text, absoluteWidth / 2, y - 4, 1, RED, UIAnchor::Center);

		int activeDot = (int)(GetTime() * 4.0) % 3;
		for (int i = 0; i < 3; ++i) {
			Vector2 dotPosition = UIToScreen(absoluteWidth / 2 - 22 + i * 22, y + 28);
			float radius = (i == activeDot ? 5.0f : 3.5f) * UIScale();
			DrawCircleV(dotPosition, radius, i == activeDot ? RED : Fade(RED, 0.45f));
		}
	};

	auto beginCreateManualValueEdit = [&](const std::string& label, int currentValue, int targetKind, int targetId, int rangeHandle) {
		createManualValueActive = true;
		createManualTargetKind = targetKind;
		createManualTargetId = targetId;
		createManualRangeHandle = rangeHandle;
		createManualValueText = std::to_string(currentValue);
		createManualValueLabel = label;
		activeCreateRangeSlider = -1;
		activeCreateRangeHandle = -1;
		};

	auto drawCreateManualValueEditor = [&]() {
		if (!createManualValueActive) {
			return;
		}

		DrawRectUI(absoluteWidth / 2, absoluteHeight / 2, 440, 170, Fade(WHITE, 0.96f), UIAnchor::Center);
		DrawRectUI(absoluteWidth / 2, absoluteHeight / 2, 440, 170, BLACK, UIAnchor::Center, 2.0f);
		DrawTextUI(createManualValueLabel, absoluteWidth / 2, absoluteHeight / 2 - 52, 1.0f, BLACK, UIAnchor::Center);
		DrawRectUI(absoluteWidth / 2, absoluteHeight / 2, 300, 52, WHITE, UIAnchor::Center);
		DrawRectUI(absoluteWidth / 2, absoluteHeight / 2, 300, 52, RED, UIAnchor::Center, 2.0f);
		DrawTextUI(createManualValueText.empty() ? "_" : createManualValueText, absoluteWidth / 2, absoluteHeight / 2, 1.1f, BLACK, UIAnchor::Center);
		DrawTextUI("Enter to apply, Esc to cancel", absoluteWidth / 2, absoluteHeight / 2 + 54, 0.75f, DARKGRAY, UIAnchor::Center);

		int key = GetCharPressed();
		while (key > 0) {
			if ((key >= '0' && key <= '9') || (key == '-' && createManualValueText.empty())) {
				createManualValueText.push_back((char)key);
			}
			key = GetCharPressed();
		}

		if (IsKeyPressed(KEY_BACKSPACE) && !createManualValueText.empty()) {
			createManualValueText.pop_back();
		}
		if (IsKeyPressed(KEY_ESCAPE)) {
			createManualValueActive = false;
		}
		if (IsKeyPressed(KEY_ENTER)) {
			try {
				int newValue = std::stoi(createManualValueText);
				if (createManualTargetKind == 1) {
					for (auto& element : createMenuUIElements) {
						if (element->elementID == createManualTargetId) {
							if (auto slider = dynamic_cast<Slider*>(element.get())) {
								slider->curVal = newValue;
							}
							break;
						}
					}
				}
				else if (createManualTargetKind == 2) {
					if (createManualTargetId == 0) {
						if (createManualRangeHandle == 0) {
							createMinNodes = newValue;
							createMaxNodes = std::max(createMaxNodes, createMinNodes);
						}
						else {
							createMaxNodes = newValue;
							createMinNodes = std::min(createMinNodes, createMaxNodes);
						}
						createMinMuscles = createMinNodes;
						createMaxMuscles = createMaxNodes * (createMaxNodes - 1) / 2;
					}
					else if (createManualTargetId == 1) {
						if (createManualRangeHandle == 0) {
							createMinMuscles = newValue;
							createMaxMuscles = std::max(createMaxMuscles, createMinMuscles);
						}
						else {
							createMaxMuscles = newValue;
							createMinMuscles = std::min(createMinMuscles, createMaxMuscles);
						}
					}
				}
				createManualValueActive = false;
			}
			catch (const std::exception&) {
				PushNotice("Invalid number", 2.0f);
			}
		}
		};

	auto drawRangeSlider = [&](const std::string& label, float x, float y, float width, float height, int minValue, int maxValue, int& selectedMin, int& selectedMax, int& activeSlider, int& activeHandle, int sliderId, bool enabled = true) {
		static bool precise = false;
		static double lastAdjustmentTime = 0.0;

		selectedMax = std::max(selectedMax, selectedMin);

		Rectangle rangeRect = DrawRectUI(x, y, width, height, DARKGRAY, UIAnchor::Center);
		DrawRectUI(x, y, width, height, BLACK, UIAnchor::Center, 2.0f);

		float nubWidth = 10.0f * UIScale();
		float trackStart = rangeRect.x + nubWidth;
		float trackEnd = rangeRect.x + rangeRect.width - nubWidth;
		int trackMinValue = std::min(minValue, maxValue);
		int trackMaxValue = std::max(minValue, maxValue);
		int valueRange = std::max(1, trackMaxValue - trackMinValue);
		auto valueToX = [&](int value) {
			float t = (float)(std::clamp(value, trackMinValue, trackMaxValue) - trackMinValue) / valueRange;
			return trackStart + t * (trackEnd - trackStart);
			};
		auto xToValue = [&](float xValue) {
			float t = std::clamp((xValue - trackStart) / std::max(1.0f, trackEnd - trackStart), 0.0f, 1.0f);
			return trackMinValue + (int)(t * valueRange + 0.5f);
			};

		float startX = valueToX(selectedMin);
		float endX = valueToX(selectedMax);
		float rangeBarY = rangeRect.y + 8 * UIScale();
		float rangeBarHeight = rangeRect.height - 16 * UIScale();

		DrawRectangle((int)startX, (int)rangeBarY, (int)std::max(1.0f, endX - startX), (int)rangeBarHeight, RED);
		DrawRectangle((int)(startX - nubWidth * 0.5f), (int)(rangeRect.y + 4 * UIScale()), (int)nubWidth, (int)(rangeRect.height - 8 * UIScale()), LIGHTGRAY);
		DrawRectangle((int)(endX - nubWidth * 0.5f), (int)(rangeRect.y + 4 * UIScale()), (int)nubWidth, (int)(rangeRect.height - 8 * UIScale()), LIGHTGRAY);
		DrawTextUI(std::format("{}: {} - {}", label, selectedMin, selectedMax), x, y, 1.0f, WHITE, UIAnchor::Center);

		if (!enabled) {
			if (activeSlider == sliderId) {
				activeSlider = -1;
				activeHandle = -1;
				precise = false;
			}
			DrawRectUI(x, y, width, height, Fade(LIGHTGRAY, 0.65f), UIAnchor::Center);
			DrawRectUI(x, y, width, height, BLACK, UIAnchor::Center, 2.0f);
			DrawTextUI(std::format("{}: {} - {}", label, selectedMin, selectedMax), x, y, 1.0f, GRAY, UIAnchor::Center);
			return false;
		}

		auto chooseHandle = [&](float mouseX) {
			if (selectedMin == selectedMax) {
				if (selectedMin <= trackMinValue) return 1;
				if (selectedMax >= trackMaxValue) return 0;
				return mouseX < startX ? 0 : 1;
			}
			return std::abs(mouseX - startX) <= std::abs(mouseX - endX) ? 0 : 1;
			};

		if (CheckCollisionPointRec(GetMousePosition(), rangeRect) && IsMouseButtonPressed(MOUSE_MIDDLE_BUTTON)) {
			float mouseX = GetMousePosition().x;
			int handle = chooseHandle(mouseX);
			beginCreateManualValueEdit(label + (handle == 0 ? " Min" : " Max"), handle == 0 ? selectedMin : selectedMax, 2, sliderId, handle);
			return false;
		}

		if (CheckCollisionPointRec(GetMousePosition(), rangeRect) && (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsMouseButtonPressed(MOUSE_RIGHT_BUTTON))) {
			float mouseX = GetMousePosition().x;
			activeSlider = sliderId;
			activeHandle = chooseHandle(mouseX);
			precise = IsMouseButtonPressed(MOUSE_RIGHT_BUTTON);
			lastAdjustmentTime = 0.0;
		}
		if (activeSlider == sliderId && ((!precise && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) || (precise && IsMouseButtonReleased(MOUSE_RIGHT_BUTTON)))) {
			activeSlider = -1;
			activeHandle = -1;
			precise = false;
		}
		if (activeSlider == sliderId && activeHandle >= 0) {
			int previousMin = selectedMin;
			int previousMax = selectedMax;
			if (precise) {
				if (GetTime() - lastAdjustmentTime < 0.1) {
					return false;
				}

				float mouseX = GetMousePosition().x;
				float handleX = activeHandle == 0 ? startX : endX;
				if (activeHandle == 0) {
					if (mouseX < handleX) {
						selectedMin = std::max(trackMinValue, selectedMin - 1);
					}
					else if (mouseX > handleX) {
						selectedMin = std::min(selectedMax, selectedMin + 1);
					}
				}
				else {
					if (mouseX < handleX) {
						selectedMax = std::max(selectedMin, selectedMax - 1);
					}
					else if (mouseX > handleX) {
						selectedMax = std::min(trackMaxValue, selectedMax + 1);
					}
				}
				lastAdjustmentTime = GetTime();
			}
			else {
				int selectedValue = xToValue(GetMousePosition().x);
				if (activeHandle == 0) {
					selectedMin = std::clamp(selectedValue, trackMinValue, selectedMax);
				}
				else {
					selectedMax = std::clamp(selectedValue, selectedMin, trackMaxValue);
				}
			}
			return previousMin != selectedMin || previousMax != selectedMax;
		}
		return false;
		};

	float frameTime = GetFrameTime();
	float currentTime = GetTime();
	
	while (!WindowShouldClose()) {

		ClampWindowSize();

		if (IsMusicStreamPlaying(musicMenu)) {
			UpdateMusicStream(musicMenu);
		}
		SetMusicVolume(musicMenu, appSettings.musicVolume / 100.0f);

		if (IsKeyPressed(KEY_F11)) {
			appSettings.fullscreen = !appSettings.fullscreen;
			ApplyFullscreenSetting();
		}

		if (saveInProgress && saveFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
			saveFuture.get();
			saveInProgress = false;
			saveWorkStarted.store(false);
		}

		if (continuousGenerationThreadRunning && continuousGenerationFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
			continuousGenerationFuture.get();
			continuousGenerationThreadRunning = false;
		}

		if (loadInProgress && loadFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
			std::unique_ptr<World> loadedWorld = loadFuture.get();
			loadInProgress = false;
			loadWorkStarted.store(false);
			if (loadedWorld) {
				world = std::move(loadedWorld);
				StopMusicStream(musicMenu);
				currentState = STATE_GAME;
				dynamic_cast<Slider*>(ingameUIElements[7].get())->minVal = world->GetFirstHistoryGeneration();
				dynamic_cast<Slider*>(ingameUIElements[7].get())->maxVal = world->GetLastHistoryGeneration();
				dynamic_cast<Slider*>(ingameUIElements[7].get())->curVal = world->viewGeneration;
				PushNotice("World loaded: " + world->worldName, 2.0f);
			}
		}

		if (createInProgress && createFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
			std::unique_ptr<World> createdWorld = createFuture.get();
			createInProgress = false;
			if (createdWorld) {
				world = std::move(createdWorld);
				StopMusicStream(musicMenu);
				currentState = STATE_GAME;
				dynamic_cast<Slider*>(ingameUIElements[7].get())->minVal = world->GetFirstHistoryGeneration();
				dynamic_cast<Slider*>(ingameUIElements[7].get())->maxVal = world->GetLastHistoryGeneration();
				dynamic_cast<Slider*>(ingameUIElements[7].get())->curVal = world->viewGeneration;
			}
		}
		
		BeginDrawing();

		switch (currentState) {
		case STATE_MENU_INIT:
			if (!IsMusicStreamPlaying(musicMenu)) PlayMusicStream(musicMenu);
			//set up menu buttons

			mainMenuButtons.clear();

			mainMenuButtons.push_back(Button(0, absoluteWidth / 2, 280, 240, 48, "Create WORLD"));
			mainMenuButtons.push_back(Button(1, absoluteWidth / 2, 360, 240, 48, "Load WORLD"));
			mainMenuButtons.push_back(Button(2, absoluteWidth / 2, 440, 240, 48, "Settings"));
			mainMenuButtons.push_back(Button(3, absoluteWidth / 2, 520, 240, 48, "About"));
			mainMenuButtons.push_back(Button(4, absoluteWidth / 2, 600, 240, 48, "Exit"));

			// set up new game input fields and buttons

			createMenuUIElements.clear();
			
			createMenuUIElements.push_back(std::make_unique<InputField>(100, 200, 220, 350, 48, "World Name"));
			createMenuUIElements.push_back(std::make_unique<InputField>(101, 200, 320, 350, 48, "World Seed"));
			createMenuUIElements.push_back(std::make_unique<InputField>(102, 100, 470, 150, 48, "BG Color", "6AB7F2", 6));
			createMenuUIElements.push_back(std::make_unique<InputField>(103, 300, 470, 150, 48, "Grnd Color", "005E00", 6));
			
			createMenuUIElements.push_back(std::make_unique<Button>(0, absoluteWidth / 2, 680, 320, 64, "Create WORLD"));
			createMenuUIElements.push_back(std::make_unique<Button>(1, 100, 680, 150, 64, "Back"));
			
			createMenuUIElements.push_back(std::make_unique<Slider>(200, 825, 180, 350, 48, "Creature Count", 100, 5000, numOfCreatures));
			createMenuUIElements.push_back(std::make_unique<Slider>(202, 825, 260, 350, 48, "Seconds per Sim", 5, 30, secondsPerSimulation));
			createMenuUIElements.push_back(std::make_unique<Slider>(203, 825, 340, 350, 48, "Mutability Range", 1, 200, 100));
			createMenuUIElements.push_back(std::make_unique<Slider>(204, 825, 420, 350, 48, "Mutability Factor", 1, 500, 100));
			createMenuUIElements.push_back(std::make_unique<Slider>(205, 825, 500, 350, 48, "Gravity", 20, 1000, gravityConst));
			createMinNodes = 3;
			createMaxNodes = 15;
			createMinMuscles = createMinNodes;
			createMaxMuscles = createMaxNodes * (createMaxNodes - 1) / 2;
			activeCreateRangeSlider = -1;
			activeCreateRangeHandle = -1;

			// ingame UI elements setup

			ingameUIElements.clear();

			ingameUIElements.push_back(std::make_unique<Button>(0, 155, 737, 270, 30, "Back"));
			ingameUIElements.push_back(std::make_unique<Button>(1, 155, 703, 270, 30, "Settings"));
			
			ingameUIElements.push_back(std::make_unique<Button>(2, 445, 720, 270, 64, "Save"));
																			 
			ingameUIElements.push_back(std::make_unique<Button>(3, 155, 620, 270, 64, "Next Generation"));
			ingameUIElements.push_back(std::make_unique<Button>(4, 445, 620, 270, 64, "Do Gens Nonstop"));
			
			ingameUIElements.push_back(std::make_unique<Button>(5, 155, 520, 270, 64, "View All Creatures"));
			ingameUIElements.push_back(std::make_unique<Button>(6, 445, 520, 270, 64, "Watch Best Crtr."));
			
			ingameUIElements.push_back(std::make_unique<Slider>(200, 270, 360, 500, 80, "View Generation", 0, 0, 0));

			settingsUIElements.clear();

			settingsUIElements.push_back(std::make_unique<Slider>(200, absoluteWidth / 2, 220, 420, 56, "Music Volume", 0, 100, appSettings.musicVolume));
			settingsUIElements.push_back(std::make_unique<Slider>(201, absoluteWidth / 2, 310, 420, 56, "Sound Volume", 0, 100, appSettings.soundVolume));
			settingsUIElements.push_back(std::make_unique<Button>(0, absoluteWidth / 2, 405, 340, 56, ""));
			settingsUIElements.push_back(std::make_unique<Button>(1, absoluteWidth / 2, 485, 420, 56, ""));
			settingsUIElements.push_back(std::make_unique<Slider>(202, absoluteWidth / 2, 570, 420, 56, "Max Threads", 1, (int)hardwareThreadCount, appSettings.maxThreadCount));
			settingsUIElements.push_back(std::make_unique<Button>(3, absoluteWidth / 2, 680, 150, 64, "Back"));

			currentState = STATE_MENU;
			break;
		case STATE_MENU:
			
			ClearBackground( PinkWORLD );
			DrawTextUI("EvolutionWORLD " + std::string(versionString), absoluteWidth / 2, 90, 2, BLACK, UIAnchor::Center);
			if (loadInProgress && loadWorkStarted.load()) {
				drawBusyPanel("Loading world...", 678);
			}

			for (int i = 0; i < mainMenuButtons.size(); i++) {

				mainMenuButtons[i].draw();
				mainMenuButtons[i].tick();

				if (mainMenuButtons[i].active && !loadInProgress) {
					switch (mainMenuButtons[i].elementID) {
					case 0: // Create World
						currentState = STATE_CREATE;
						break;
					case 1: // Load World
						if (!loadInProgress) {
							loadInProgress = true;
							loadWorkStarted.store(false);
							loadFuture = std::async(std::launch::async, [&loadWorkStarted] {
								return World::LoadFromDialog(&loadWorkStarted);
								});
						}
						break;
					case 2: // Settings
						settingsReturnState = STATE_MENU;
						currentState = STATE_OPTIONS;
						break;
					case 3: // About
						PushNotice("About screen not implemented yet!", 2.0f);
						break;
					case 4: // Exit
						SaveAppSettings();
						CloseWindow();
						return 0;
					}
				}
			}
			break;
		case STATE_OPTIONS: {
			ClearBackground(LightBlueWORLD);
			DrawTextUI("Settings", absoluteWidth / 2, 90, 2, BLACK, UIAnchor::Center);

			dynamic_cast<Button*>(settingsUIElements[2].get())->name = appSettings.fullscreen ? "Fullscreen: On" : "Fullscreen: Off";
			dynamic_cast<Button*>(settingsUIElements[3].get())->name = appSettings.showGenerationsPerSecond ? "Show Gens/Sec: On" : "Show Gens/Sec: Off";
			bool maxThreadsSliderEnabled = settingsReturnState != STATE_GAME && hardwareThreadCount > 1;

			for (int i = 0; i < settingsUIElements.size(); i++) {
				settingsUIElements[i]->enabled = !(settingsUIElements[i]->elementID == 202 && !maxThreadsSliderEnabled);
				settingsUIElements[i]->tick();
				settingsUIElements[i]->draw();

				if (settingsUIElements[i]->active) {
					bool settingsChanged = false;
					switch (settingsUIElements[i]->elementID) {
					case 0:
						appSettings.fullscreen = !appSettings.fullscreen;
						ApplyFullscreenSetting();
						settingsChanged = true;
						break;
					case 1:
						appSettings.showGenerationsPerSecond = !appSettings.showGenerationsPerSecond;
						settingsChanged = true;
						break;
					case 3:
						currentState = settingsReturnState;
						break;
					case 200:
					{
						int newMusicVolume = std::stoi(settingsUIElements[i]->getContent());
						settingsChanged = newMusicVolume != appSettings.musicVolume;
						appSettings.musicVolume = newMusicVolume;
						break;
					}
					case 201:
					{
						int newSoundVolume = std::stoi(settingsUIElements[i]->getContent());
						settingsChanged = newSoundVolume != appSettings.soundVolume;
						appSettings.soundVolume = newSoundVolume;
						break;
					}
					case 202:
					{
						int newMaxThreadCount = std::stoi(settingsUIElements[i]->getContent());
						settingsChanged = newMaxThreadCount != appSettings.maxThreadCount;
						appSettings.maxThreadCount = newMaxThreadCount;
						numberOfThreads = (unsigned int)appSettings.maxThreadCount;
						break;
					}
					}
					if (settingsChanged) {
						SaveAppSettings();
					}
				}
			}

			if (IsKeyPressed(KEY_B)) {
				currentState = settingsReturnState;
			}
			break;
		}
		case STATE_CREATE: {
			ClearBackground(LightBlueWORLD);
			DrawTextUI("Create WORLD", absoluteWidth / 2, 90, 2, BLACK, UIAnchor::Center);

			for (int i = 0; i < createMenuUIElements.size(); i++) {
				if (!createManualValueActive) {
					createMenuUIElements[i]->tick();
				}
				createMenuUIElements[i]->draw();
				if (!createManualValueActive) {
					if (auto slider = dynamic_cast<Slider*>(createMenuUIElements[i].get())) {
						if (CheckCollisionPointRec(GetMousePosition(), slider->drawRect) && IsMouseButtonPressed(MOUSE_MIDDLE_BUTTON)) {
							beginCreateManualValueEdit(slider->name, slider->curVal, 1, slider->elementID, -1);
						}
					}
				}

				if (createMenuUIElements[i]->active && !createInProgress && !createManualValueActive) {
					switch (createMenuUIElements[i]->elementID) {
					case 0: // Create World
					{
						if (createMenuUIElements[0]->getContent().empty()) {
							PushNotice("World name cannot be empty!", 3.0f);
							break;
						}
						int bgColor, grColor;
						try {
							bgColor = std::stoi(createMenuUIElements[2]->getContent(), nullptr, 16);
							grColor = std::stoi(createMenuUIElements[3]->getContent(), nullptr, 16);
						}
						catch (const std::invalid_argument&) {
							PushNotice("Invalid color code! Use hex format (e.g. A9A9A9)", 3.0f);
							break;
						}
						catch (const std::out_of_range&) {
							PushNotice("Color code out of range! Use hex format (e.g. A9A9A9)", 3.0f);
							break;
						}
						catch (const std::exception& e) {
							PushNotice("Error: " + std::string(e.what()), 3.0f);
							break;
						}

						Color backgroundColor = ColorFromInt(bgColor);
						Color groundColor = ColorFromInt(grColor);
						std::string newWorldName = createMenuUIElements[0]->getContent();
						std::string newWorldSeed = createMenuUIElements[1]->getContent();
						int newGravity = std::stoi(createMenuUIElements[10]->getContent());
						int newCreatureCount = std::stoi(createMenuUIElements[6]->getContent());
						int newSecondsPerSimulation = std::stoi(createMenuUIElements[7]->getContent());
						int newMutabilityRange = std::stoi(createMenuUIElements[8]->getContent());
						int newMutabilityFactor = std::stoi(createMenuUIElements[9]->getContent());
						int newMinNodes = createMinNodes;
						int newMaxNodes = createMaxNodes;
						int newMinMuscles = createMinMuscles;
						int newMaxMuscles = createMaxMuscles;

						createInProgress = true;
						createFuture = std::async(std::launch::async, [=] {
							return std::make_unique<World>(
								newWorldName,
								newWorldSeed,
								newGravity,
								newCreatureCount,
								newSecondsPerSimulation,
								newMutabilityRange,
								newMutabilityFactor,
								newMinNodes,
								newMaxNodes,
								newMinMuscles,
								newMaxMuscles,
								backgroundColor,
								groundColor
							);
							});
						break;
					}
					case 1: // Back
					{
						previewWorld.backgroundColor = ColorFromInt(0x6AB7F2);
						previewWorld.groundColor = ColorFromInt(0x005E00);
						currentState = STATE_MENU_INIT;
						break;
					}
					case 102:
					{
						// Input field for background color
						std::string content = createMenuUIElements[i]->getContent();
						if (content.length() == 6 && std::all_of(content.begin(), content.end(), ::isxdigit)) {
							previewWorld.backgroundColor = ColorFromInt(std::stoi(createMenuUIElements[2]->getContent(), nullptr, 16));
						}
						break;
					}
					case 103:
					{
						// Input field for ground color
						std::string content = createMenuUIElements[i]->getContent();
						if (content.length() == 6 && std::all_of(content.begin(), content.end(), ::isxdigit)) {
							previewWorld.groundColor = ColorFromInt(std::stoi(createMenuUIElements[3]->getContent(), nullptr, 16));
						}
						break;
					}
					default:
					{
						break;
					}
					}
				}

			}

			int previousMinNodes = createMinNodes;
			int previousMaxNodes = createMaxNodes;
			bool nodeRangeChanged = drawRangeSlider("Node Count", absoluteWidth / 2, 558, 350, 48, 3, 15, createMinNodes, createMaxNodes, activeCreateRangeSlider, activeCreateRangeHandle, 0, !createInProgress && !createManualValueActive);
			if (nodeRangeChanged || previousMinNodes != createMinNodes || previousMaxNodes != createMaxNodes) {
				createMinMuscles = createMinNodes;
				createMaxMuscles = createMaxNodes * (createMaxNodes - 1) / 2;
			}

			const int muscleSliderMin = createMinNodes;
			const int muscleSliderMax = createMaxNodes * (createMaxNodes - 1) / 2;
			drawRangeSlider("Muscle Count", absoluteWidth / 2, 615, 350, 48, muscleSliderMin, muscleSliderMax, createMinMuscles, createMaxMuscles, activeCreateRangeSlider, activeCreateRangeHandle, 1, !createInProgress && !createManualValueActive);

			// Draw preview world
			DrawTextUI("Preview", absoluteWidth / 2, 240, 1.2f, BLACK, UIAnchor::Center);
			previewWorld.drawtick(absoluteWidth / 2, 395, 240, 240);
			DrawRectUI(absoluteWidth / 2, 395, 240, 240, BLACK, UIAnchor::Center, 2);
			if (createInProgress) {
				drawBusyPanel("Loading world...", 678);
			}
			drawCreateManualValueEditor();
			break;
		}
		case STATE_SAVE: {
			ClearBackground(BeigeWORLD);

			std::string worldNameSnapshot;
			uint32_t worldSeedSnapshot = 0;
			int numCreaturesSnapshot = 0;
			int minNodesSnapshot = 0;
			int maxNodesSnapshot = 0;
			int minMusclesSnapshot = 0;
			int maxMusclesSnapshot = 0;
			int secondsPerSimulationSnapshot = 0;
			float mutabilityRangeSnapshot = 0.0f;
			float mutabilityFactorSnapshot = 0.0f;
			int gravitySnapshot = 0;
			int currentGenerationSnapshot = 0;
			int firstHistoryGeneration = 0;
			int lastHistoryGeneration = 0;
			float bestFitnessMeters = 0.0f;
			bool hasHistory = false;
			bool saveRangeHasData = false;
			std::vector<int> storedHistoryGenerationsSnapshot;
			{
				std::lock_guard<std::mutex> dataLock(world->dataMutex);
				worldNameSnapshot = world->worldName;
				worldSeedSnapshot = world->worldSeed;
				numCreaturesSnapshot = world->numOfCreatures;
				minNodesSnapshot = world->minNodes;
				maxNodesSnapshot = world->maxNodes;
				minMusclesSnapshot = world->minMuscles;
				maxMusclesSnapshot = world->maxMuscles;
				secondsPerSimulationSnapshot = world->secondsPerSimulation;
				mutabilityRangeSnapshot = world->mutabilityRange;
				mutabilityFactorSnapshot = world->mutabilityFactor;
				gravitySnapshot = world->gravity;
				currentGenerationSnapshot = world->generation;
				hasHistory = !world->bestGenerationalCreatures.empty();
				firstHistoryGeneration = hasHistory ? world->GetFirstHistoryGeneration() : world->generation;
				lastHistoryGeneration = hasHistory ? world->GetLastHistoryGeneration() : world->generation;
				storedHistoryGenerationsSnapshot = world->storedHistoryGenerations;
				for (const Creature& creature : world->bestGenerationalCreatures) {
					bestFitnessMeters = std::max(bestFitnessMeters, creature.fitness / 100.0f);
				}
			}

			saveStartGeneration = std::clamp(saveStartGeneration, firstHistoryGeneration, lastHistoryGeneration);
			saveEndGeneration = std::clamp(saveEndGeneration, saveStartGeneration, lastHistoryGeneration);
			saveRangeHasData = std::any_of(storedHistoryGenerationsSnapshot.begin(), storedHistoryGenerationsSnapshot.end(), [&](int generation) {
				return generation >= saveStartGeneration && generation <= saveEndGeneration;
				});

			DrawTextUI("Save World", absoluteWidth / 2, 54, 2.0f, BLACK, UIAnchor::Center);
			DrawTextUI("World Name: " + worldNameSnapshot, 110, 120, 1.0f, BLACK);
			DrawTextUI("World Seed: " + std::to_string(worldSeedSnapshot), 110, 158, 1.0f, BLACK);
			
			DrawTextUI("Current generation: " + std::to_string(currentGenerationSnapshot), 110, 234, 1.0f, BLACK);
			DrawTextUI("Total stored generations: " + std::to_string(storedHistoryGenerationsSnapshot.size()), 110, 272, 1.0f, BLACK);
			DrawTextUI(std::format("Best fitness: {:.2f} meters", bestFitnessMeters), 110, 310, 1.0f, BLACK);
			
			DrawTextUI("Seconds per sim: " + std::to_string(secondsPerSimulationSnapshot), 610, 120, 1.0f, BLACK);
			DrawTextUI(std::format("Mutability range: {:.2f}", mutabilityRangeSnapshot), 610, 158, 1.0f, BLACK);
			DrawTextUI(std::format("Mutability factor: {:.2f}", mutabilityFactorSnapshot), 610, 196, 1.0f, BLACK);
			DrawTextUI("Gravity: " + std::to_string(gravitySnapshot), 610, 234, 1.0f, BLACK);
			DrawTextUI("Amount of creatures: " + std::to_string(numCreaturesSnapshot), 110, 196, 1.0f, BLACK);
			DrawTextUI(std::format("Min node/muscle: {} / {}", minNodesSnapshot, minMusclesSnapshot), 610, 272, 1.0f, BLACK);
			DrawTextUI(std::format("Max node/muscle: {} / {}", maxNodesSnapshot, maxMusclesSnapshot), 610, 310, 1.0f, BLACK);

			DrawTextUI("History generations to save", absoluteWidth / 2, 400, 1.0f, BLACK, UIAnchor::Center);
			Rectangle rangeRect = DrawRectUI(absoluteWidth / 2, 454, 840, 64, DARKGRAY, UIAnchor::Center);
			DrawRectUI(absoluteWidth / 2, 454, 840, 64, BLACK, UIAnchor::Center, 2.0f);

			float nubWidth = 10.0f * UIScale();
			float trackStart = rangeRect.x + nubWidth;
			float trackEnd = rangeRect.x + rangeRect.width - nubWidth;
			int historyRange = std::max(1, lastHistoryGeneration - firstHistoryGeneration);
			auto generationToX = [&](int generationValue) {
				float t = (float)(generationValue - firstHistoryGeneration) / historyRange;
				return trackStart + t * (trackEnd - trackStart);
				};
			auto xToGeneration = [&](float xValue) {
				float t = std::clamp((xValue - trackStart) / std::max(1.0f, trackEnd - trackStart), 0.0f, 1.0f);
				return firstHistoryGeneration + (int)(t * historyRange + 0.5f);
				};

			float startX = generationToX(saveStartGeneration);
			float endX = generationToX(saveEndGeneration);
			float rangeBarY = rangeRect.y + 8 * UIScale();
			float rangeBarHeight = rangeRect.height - 16 * UIScale();
			DrawRectangle((int)startX, (int)rangeBarY, (int)std::max(1.0f, endX - startX), (int)rangeBarHeight, RED);
			auto drawExistingRun = [&](int runStart, int runEnd) {
				float runStartX = generationToX(std::max(runStart, saveStartGeneration));
				float runEndX = generationToX(std::min(runEnd, saveEndGeneration));
				DrawRectangle((int)runStartX, (int)rangeBarY, (int)std::max(1.0f, runEndX - runStartX), (int)rangeBarHeight, GREEN);
				};
			int runStart = -1;
			int previousGeneration = -1;
			for (int storedGeneration : storedHistoryGenerationsSnapshot) {
				if (storedGeneration < saveStartGeneration || storedGeneration > saveEndGeneration) {
					continue;
				}
				if (runStart < 0) {
					runStart = storedGeneration;
				}
				else if (storedGeneration != previousGeneration + 1) {
					drawExistingRun(runStart, previousGeneration);
					runStart = storedGeneration;
				}
				previousGeneration = storedGeneration;
			}
			if (runStart >= 0) {
				drawExistingRun(runStart, previousGeneration);
			}
			DrawRectangle((int)(startX - nubWidth * 0.5f), (int)(rangeRect.y + 4 * UIScale()), (int)nubWidth, (int)(rangeRect.height - 8 * UIScale()), LIGHTGRAY);
			DrawRectangle((int)(endX - nubWidth * 0.5f), (int)(rangeRect.y + 4 * UIScale()), (int)nubWidth, (int)(rangeRect.height - 8 * UIScale()), LIGHTGRAY);
			DrawTextUI(std::format("{} - {}", saveStartGeneration, saveEndGeneration), absoluteWidth / 2, 454, 1.0f, WHITE, UIAnchor::Center);

			if (hasHistory && CheckCollisionPointRec(GetMousePosition(), rangeRect) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
				float mouseX = GetMousePosition().x;
				activeSaveRangeHandle = std::abs(mouseX - startX) <= std::abs(mouseX - endX) ? 0 : 1;
			}
			if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
				activeSaveRangeHandle = -1;
			}
			if (activeSaveRangeHandle >= 0) {
				int selectedGeneration = xToGeneration(GetMousePosition().x);
				if (activeSaveRangeHandle == 0) {
					saveStartGeneration = std::clamp(selectedGeneration, firstHistoryGeneration, saveEndGeneration);
				}
				else {
					saveEndGeneration = std::clamp(selectedGeneration, saveStartGeneration, lastHistoryGeneration);
				}
			}
			saveRangeHasData = std::any_of(storedHistoryGenerationsSnapshot.begin(), storedHistoryGenerationsSnapshot.end(), [&](int generation) {
				return generation >= saveStartGeneration && generation <= saveEndGeneration;
				});
			if (!saveRangeHasData) {
				DrawTextUI(std::format("No data for generations {} to {}", saveStartGeneration, saveEndGeneration), absoluteWidth / 2, 505, 0.8f, RED, UIAnchor::Center);
			}

			Button graphToggleButton(1200, absoluteWidth / 2, 555, 480, 48, saveIncludeGraph ? "Save Percentile Graph: Yes" : "Save Percentile Graph: No");
			Button speciesGraphToggleButton(1203, absoluteWidth / 2, 610, 480, 48, saveIncludeSpeciesGraph ? "Save Species Graph: Yes" : "Save Species Graph: No");
			Button saveButton(1201, absoluteWidth / 2 + 150, 680, 240, 64, "Save");
			Button backButton(1202, absoluteWidth / 2 - 150, 680, 240, 64, "Back");
			saveButton.enabled = saveRangeHasData;

			graphToggleButton.draw();
			speciesGraphToggleButton.draw();
			saveButton.draw();
			backButton.draw();

			if (!saveInProgress) {
				graphToggleButton.tick();
				speciesGraphToggleButton.tick();
				saveButton.tick();
				backButton.tick();
			}

			if (graphToggleButton.active) {
				saveIncludeGraph = !saveIncludeGraph;
			}
			if (speciesGraphToggleButton.active) {
				saveIncludeSpeciesGraph = !saveIncludeSpeciesGraph;
			}
			if (saveButton.active && saveRangeHasData) {
				saveInProgress = true;
				saveWorkStarted.store(false);
				int startToSave = saveStartGeneration;
				int endToSave = saveEndGeneration;
				bool includePercentileGraph = saveIncludeGraph;
				bool includeSpeciesGraph = saveIncludeSpeciesGraph;
				saveFuture = std::async(std::launch::async, [worldPtr = world.get(), &saveWorkStarted, startToSave, endToSave, includePercentileGraph, includeSpeciesGraph] {
					worldPtr->Save(&saveWorkStarted, startToSave, endToSave, includePercentileGraph, includeSpeciesGraph);
					});
			}
			if (backButton.active) {
				activeSaveRangeHandle = -1;
				currentState = STATE_GAME;
			}
			if (saveInProgress && saveWorkStarted.load()) {
				drawBusyPanel("Saving world...", 610);
			}

			break;
		}
		case STATE_GAME: {

			ClearBackground(BeigeWORLD);

			bool generationFinishedThisFrame = !continuousGenerationThreadRunning && world->FinishGenerationIfReady();
			int continuousGenerationsFinishedThisFrame = continuousGenerationsCompleted.exchange(0);
			if (continuousGenerationsFinishedThisFrame > 0) {
				generationFinishedThisFrame = true;
				timeForOneGen = (float)latestContinuousGenerationSeconds.load();
				generationsCompletedInWindow += continuousGenerationsFinishedThisFrame;
			}

			if (generationFinishedThisFrame && continuousGenerationsFinishedThisFrame == 0) {
				timeForOneGen = (float)(GetTime() - generationStartTime);
				int currentGeneration = 0;
				{
					std::lock_guard<std::mutex> dataLock(world->dataMutex);
					currentGeneration = world->generation;
				}
				std::cout << "Generation " << currentGeneration << " took " << timeForOneGen << " seconds.\n";
			}

			if (generationFinishedThisFrame) {
				int currentGeneration = 0;
				int firstHistoryGeneration = 0;
				int lastHistoryGeneration = 0;
				{
					std::lock_guard<std::mutex> dataLock(world->dataMutex);
					currentGeneration = world->generation;
					firstHistoryGeneration = world->GetFirstHistoryGeneration();
					lastHistoryGeneration = world->GetLastHistoryGeneration();
				}
				dynamic_cast<Slider*>(ingameUIElements[7].get())->minVal = firstHistoryGeneration;
				dynamic_cast<Slider*>(ingameUIElements[7].get())->maxVal = lastHistoryGeneration;
				dynamic_cast<Slider*>(ingameUIElements[7].get())->curVal = std::clamp(currentGeneration, firstHistoryGeneration, lastHistoryGeneration);

				if (doGenerationsNonstop) {
					double now = GetTime();
					double elapsed = now - generationsPerSecondWindowStart;
					if (elapsed >= 1.0) {
						generationsPerSecond = (float)(generationsCompletedInWindow / elapsed);
						generationsCompletedInWindow = 0;
						generationsPerSecondWindowStart = now;
					}
				}
			}

			{
				std::lock_guard<std::mutex> dataLock(world->dataMutex);
				int viewHistoryIndex = world->GetHistoryIndexForGeneration(world->viewGeneration);
				viewHistoryIndex = std::clamp(viewHistoryIndex, 0, std::max(0, (int)world->worstGenerationalCreatures.size() - 1));
				bool viewGenerationHasData = world->HasHistoryDataForGeneration(world->viewGeneration);
				DrawTextUI("World \"" + world->worldName + "\", seed " + std::to_string(world->worldSeed), 20, 36, 1, BLACK);
				DrawTextUI("Generation: " + std::to_string(world->generation), 20, 72, 2, BLACK);
				if (viewGenerationHasData) {
					DrawTextUI(std::format("Worst fitness: {:.2f} meters", world->worstGenerationalCreatures[viewHistoryIndex].fitness / 100), 18, 160, 1, BLACK);
					DrawTextUI(std::format("Average fitness: {:.2f} meters", world->averageGenerationalCreatures[viewHistoryIndex].fitness / 100), 18, 200, 1, BLACK);
					DrawTextUI(std::format("Best fitness: {:.2f} meters", world->bestGenerationalCreatures[viewHistoryIndex].fitness / 100), 18, 240, 1, BLACK);
				}
			}

			{
				std::lock_guard<std::mutex> dataLock(world->dataMutex);
				world->percentileGraph.draw();
				world->speciesGraph.draw();
			}

			bool canViewAllCreatures = false;
			bool viewGenerationHasData = false;
			int selectedViewGeneration = 0;
			{
				std::lock_guard<std::mutex> dataLock(world->dataMutex);
				canViewAllCreatures = world->viewGeneration == world->generation;
				viewGenerationHasData = world->HasHistoryDataForGeneration(world->viewGeneration);
				selectedViewGeneration = world->viewGeneration;
			}

			for (int i = 0; i < ingameUIElements.size(); i++) {
				ingameUIElements[i]->enabled = !(ingameUIElements[i]->elementID == 5 && !canViewAllCreatures);
				ingameUIElements[i]->tick();
				ingameUIElements[i]->draw();
				if (ingameUIElements[i]->active && !confirmLeaveWorld && !doGenerationsNonstop && !world->IsGenerationInProgress() && !saveInProgress) {
					switch (ingameUIElements[i]->elementID) {
					case 0: // Main Menu
						confirmLeaveWorld = true;
						break;
					case 1: // Settings
						settingsReturnState = STATE_GAME;
						currentState = STATE_OPTIONS;
						confirmLeaveWorld = false;
						break;
					case 2: // Save
						{
							std::lock_guard<std::mutex> dataLock(world->dataMutex);
							saveStartGeneration = world->GetFirstHistoryGeneration();
							saveEndGeneration = world->GetLastHistoryGeneration();
						}
						saveIncludeGraph = true;
						saveIncludeSpeciesGraph = true;
						activeSaveRangeHandle = -1;
						currentState = STATE_SAVE;
						break;
					case 3: // Next Generation
						if (world->StartGeneration()) {
							generationStartTime = GetTime();
						}
						break;
					case 4: // Do Generations Nonstop
						doGenerationsNonstop = true;
						stopContinuousGenerations.store(false);
						continuousGenerationsCompleted.store(0);
						generationsCompletedInWindow = 0;
						generationsPerSecond = 0.0f;
						generationsPerSecondWindowStart = GetTime();
						continuousGenerationThreadRunning = true;
						continuousGenerationFuture = std::async(std::launch::async, [
							worldPtr = world.get(),
							&stopContinuousGenerations,
							&continuousGenerationsCompleted,
							&latestContinuousGenerationSeconds
						] {
							while (!stopContinuousGenerations.load()) {
								auto started = std::chrono::steady_clock::now();
								worldPtr->DoGeneration();
								auto finished = std::chrono::steady_clock::now();
								std::chrono::duration<double> elapsed = finished - started;
								latestContinuousGenerationSeconds.store(elapsed.count());
								continuousGenerationsCompleted.fetch_add(1);
							}
							});
						break;
					case 5: // View All Creatures
						viewAllCreaturePage = 0;
						currentState = STATE_VIEW_ALL;
						break;
					case 6: // See Best Creature
						creatureIndexToBeDrawn = 2;
						drawCreatureFromCurrentPopulation = false;
						creaturePlaybackSpeed = 1.0f;
						creaturePlaybackPaused = false;
						watchTime = 0.0f;
						world->accumulatedTime = 0.0f;
						currentState = STATE_DRAW_CREATURE;
						break;
					case 200: // View Generation Slider
						world->viewGeneration = std::stoi(ingameUIElements[i]->getContent());
						world->percentileGraph.updateExtremeValues();
						break;
					}
				}
			}
			if (!viewGenerationHasData) {
				DrawTextUI(std::format("No data for generation #{}", selectedViewGeneration), 300, 405, 0.8f, RED, UIAnchor::Center);
			}

			if (doGenerationsNonstop && !continuousGenerationThreadRunning && !world->IsGenerationInProgress()) {
				if (world->StartGeneration()) {
					generationStartTime = GetTime();
				}
			}

			if (world->IsGenerationInProgress()) {
				if (doGenerationsNonstop) {
					DrawRectUI(absoluteWidth / 2, 682, 460, 124, { 255, 255, 255, 255 }, UIAnchor::Center);
					DrawRectUI(absoluteWidth / 2, 682, 460, 124, BLACK, UIAnchor::Center, 2.0f);
					DrawTextUI("Doing generations continuously...", absoluteWidth / 2, 652, 1, BLACK, UIAnchor::Center);
				}
				else {
					DrawRectUI(absoluteWidth / 2, 642, 360, 44, { 255, 255, 255, 255 }, UIAnchor::Center);
					DrawRectUI(absoluteWidth / 2, 642, 360, 44, BLACK, UIAnchor::Center, 2.0f);
					DrawTextUI("Generation running...", absoluteWidth / 2, 642, 1, BLACK, UIAnchor::Center);
				}
			}

			if (doGenerationsNonstop) {
				Button stopButton(999, absoluteWidth / 2, 708, 180, 48, "Stop");
				stopButton.draw();
				stopButton.tick();
				if (stopButton.active) {
					doGenerationsNonstop = false;
					stopContinuousGenerations.store(true);
					generationsCompletedInWindow = 0;
					generationsPerSecond = 0.0f;
					generationsPerSecondWindowStart = GetTime();
				}
			}

			if (appSettings.showGenerationsPerSecond && doGenerationsNonstop) {
				DrawTextUI(std::format("Generations/sec: {:.2f}", generationsPerSecond), 720, 10, 0.75f, BLACK);
			}

			if (saveInProgress && saveWorkStarted.load()) {
				drawBusyPanel("Saving world...", 678);
			}

			if (confirmLeaveWorld) {
				DrawRectUI(absoluteWidth / 2, absoluteHeight / 2, 460, 220, Fade(WHITE, 0.96f), UIAnchor::Center);
				DrawRectUI(absoluteWidth / 2, absoluteHeight / 2, 460, 220, BLACK, UIAnchor::Center, 2.0f);
				DrawTextUI("Leave this world?", absoluteWidth / 2, absoluteHeight / 2 - 58, 1.4f, BLACK, UIAnchor::Center);
				DrawTextUI("Unsaved progress will be lost.", absoluteWidth / 2, absoluteHeight / 2 - 18, 0.8f, DARKGRAY, UIAnchor::Center);

				Button confirmLeaveButton(1000, absoluteWidth / 2 - 105, absoluteHeight / 2 + 58, 150, 56, "Leave");
				Button cancelLeaveButton(1001, absoluteWidth / 2 + 105, absoluteHeight / 2 + 58, 150, 56, "Cancel");
				confirmLeaveButton.draw();
				cancelLeaveButton.draw();
				confirmLeaveButton.tick();
				cancelLeaveButton.tick();

				if (confirmLeaveButton.active) {
					confirmLeaveWorld = false;
					ClearNotices();
					ClearGradientTextCache();
					currentState = STATE_MENU_INIT;
				}
				else if (cancelLeaveButton.active) {
					confirmLeaveWorld = false;
				}
			}

			break;
		}
		case STATE_VIEW_ALL: {
			ClearBackground(BeigeWORLD);

			constexpr int gridColumns = 40;
			constexpr int gridRows = 25;
			constexpr int creaturesPerPage = gridColumns * gridRows;
			const int creatureCount = world->numOfCreatures;
			const int pageCount = std::max(1, (creatureCount + creaturesPerPage - 1) / creaturesPerPage);
			viewAllCreaturePage = std::clamp(viewAllCreaturePage, 0, pageCount - 1);
			const int firstCreatureIndex = viewAllCreaturePage * creaturesPerPage;
			const int creaturesOnPage = std::min(creaturesPerPage, creatureCount - firstCreatureIndex);

			DrawTextUI("All Creatures of Generation #" + std::to_string(world->generation), absoluteWidth / 2, 36, 1.6f, BLACK, UIAnchor::Center);
			DrawTextUI("Click on the rectangles to view the creatures!", absoluteWidth / 2, 80, 0.8f, DARKGRAY, UIAnchor::Center);

			Button viewAllBackButton(1100, absoluteWidth / 2, 720, 260, 56, "Back");
			Button previousPageButton(1101, absoluteWidth / 2 - 80, 652, 64, 52, "<");
			Button nextPageButton(1102, absoluteWidth / 2 + 80, 652, 64, 52, ">");

			bool canGoPrevious = pageCount > 1 && viewAllCreaturePage > 0;
			bool canGoNext = pageCount > 1 && viewAllCreaturePage < pageCount - 1;
			previousPageButton.enabled = canGoPrevious;
			nextPageButton.enabled = canGoNext;
			previousPageButton.draw();
			nextPageButton.draw();
			previousPageButton.tick();
			if (previousPageButton.active) {
				viewAllCreaturePage--;
			}
			nextPageButton.tick();
			if (nextPageButton.active) {
				viewAllCreaturePage++;
			}
			DrawTextUI(std::format("{} / {}", viewAllCreaturePage + 1, pageCount), absoluteWidth / 2, 652, 0.8f, DARKGRAY, UIAnchor::Center);

			viewAllBackButton.draw();
			viewAllBackButton.tick();
			if (viewAllBackButton.active) {
				ClearGradientTextCache();
				currentState = STATE_GAME;
			}

			int hoveredCreatureIndex = -1;
			{
				std::lock_guard<std::mutex> dataLock(world->dataMutex);
				std::vector<int> sortedCreatureIndices(creatureCount);
				for (int i = 0; i < creatureCount; ++i) {
					sortedCreatureIndices[i] = i;
				}
				std::sort(sortedCreatureIndices.begin(), sortedCreatureIndices.end(), [](int a, int b) {
					return world->creatures[a].fitness > world->creatures[b].fitness;
					});

				const float gridX = 52.0f;
				const float gridY = 108.0f;
				const float cellWidth = 20.0f;
				const float cellHeight = 17.0f;
				const float cellGap = 3.0f;
				const Vector2 mousePosition = GetMousePosition();

				for (int i = 0; i < creaturesOnPage; ++i) {
					const int column = i % gridColumns;
					const int row = i / gridColumns;
					const int creatureIndex = sortedCreatureIndices[firstCreatureIndex + i];
					const float cellX = gridX + column * (cellWidth + cellGap);
					const float cellY = gridY + row * (cellHeight + cellGap);

					auto colors = getSpeciesColor(world->creatures[creatureIndex].nodeCount, world->creatures[creatureIndex].muscleCount);

					Rectangle cellRect = DrawGradientUI(cellX, cellY, cellWidth, cellHeight, colors.first, colors.second);
					// DrawRectUI(cellX, cellY, cellWidth, cellHeight, BLACK, UIAnchor::TopLeft, 1.0f);

					if (CheckCollisionPointRec(mousePosition, cellRect)) {
						hoveredCreatureIndex = creatureIndex;
						DrawRectUI(cellX, cellY, cellWidth, cellHeight, YELLOW, UIAnchor::TopLeft, 2.0f);
					}
				}

				if (hoveredCreatureIndex >= 0 && hoveredCreatureIndex < creatureCount) {
					if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
						selectedCurrentCreature = world->creatures[hoveredCreatureIndex];
						selectedCurrentCreature.reset();
						drawCreatureFromCurrentPopulation = true;
						watchTime = 0.0f;
						creaturePlaybackSpeed = 1.0f;
						creaturePlaybackPaused = false;
						world->accumulatedTime = 0.0f;
						currentState = STATE_DRAW_CREATURE;
					}

					const Creature& creature = world->creatures[hoveredCreatureIndex];
					std::string line1 = std::format("Creature #{}", creature.id);
					std::string line2Prefix = "Species ";
					std::string line2Species = std::format("N{}M{}", creature.nodeCount, creature.muscleCount);
					std::string line2 = line2Prefix + line2Species;
					std::string line3 = std::format("Fitness: {:.2f} meters", creature.fitness / 100.0f);
					auto colors = getSpeciesColor(creature.nodeCount, creature.muscleCount);
					const float tooltipFontSize = UIFontSize(1.0f);
					const float tooltipSpacing = UISpacing(1.0f);
					Vector2 line1Size = MeasureTextEx(defaultFont, line1.c_str(), tooltipFontSize, tooltipSpacing);
					Vector2 line2PrefixSize = MeasureTextEx(defaultFont, line2Prefix.c_str(), tooltipFontSize, tooltipSpacing);
					Vector2 line2Size = MeasureTextEx(defaultFont, line2.c_str(), tooltipFontSize, tooltipSpacing);
					Vector2 line3Size = MeasureTextEx(defaultFont, line3.c_str(), tooltipFontSize, tooltipSpacing);
					float tooltipWidth = std::max({ line1Size.x, line2Size.x, line3Size.x }) + 20.0f * UIScale();
					float tooltipHeight = line1Size.y + line2Size.y + line3Size.y + 20.0f * UIScale();
					Vector2 tooltipPosition = { mousePosition.x + 16.0f * UIScale(), mousePosition.y + 16.0f * UIScale() };

					if (tooltipPosition.x + tooltipWidth > GetScreenWidth()) {
						tooltipPosition.x = mousePosition.x - tooltipWidth - 16.0f * UIScale();
					}
					if (tooltipPosition.y + tooltipHeight > GetScreenHeight()) {
						tooltipPosition.y = mousePosition.y - tooltipHeight - 16.0f * UIScale();
					}

					DrawRectangle((int)tooltipPosition.x, (int)tooltipPosition.y, (int)tooltipWidth, (int)tooltipHeight, Fade(BLACK, 0.96f));
					DrawRectangleLinesEx({ tooltipPosition.x, tooltipPosition.y, tooltipWidth, tooltipHeight }, std::max(1.0f, UIScale()), WHITE);
					DrawTextEx(defaultFont, line1.c_str(), { tooltipPosition.x + (tooltipWidth - line1Size.x) * 0.5f, tooltipPosition.y + 8.0f * UIScale() }, tooltipFontSize, tooltipSpacing, WHITE);
					Vector2 line2Position = { tooltipPosition.x + (tooltipWidth - line2Size.x) * 0.5f, tooltipPosition.y + 8.0f * UIScale() + line1Size.y };
					DrawTextEx(defaultFont, line2Prefix.c_str(), line2Position, tooltipFontSize, tooltipSpacing, WHITE);
					DrawGradientText(line2Species, { line2Position.x + line2PrefixSize.x, line2Position.y }, tooltipFontSize, tooltipSpacing, colors.first, colors.second);
					DrawTextEx(defaultFont, line3.c_str(), { tooltipPosition.x + (tooltipWidth - line3Size.x) * 0.5f, tooltipPosition.y + 8.0f * UIScale() + line1Size.y + line2Size.y }, tooltipFontSize, tooltipSpacing, WHITE);
				}
			}

			break;
		}
		case STATE_DRAW_CREATURE:
			ClearBackground(world->backgroundColor);
			auto creature = drawCreatureFromCurrentPopulation
				? world->DrawCreatureCentered(selectedCurrentCreature, creaturePlaybackSpeed, creaturePlaybackPaused)
				: world->DrawWithCreatureCentered(creatureIndexToBeDrawn, world->viewGeneration, creaturePlaybackSpeed, creaturePlaybackPaused);

			DrawRectUI(absoluteWidth / 2, 16.0f, 800.0f, 180.0f, BLACK, UIAnchor::CenterHorizontal);
			DrawRectUI(absoluteWidth / 2, 16.0f, 800.0f, 180.0f, WHITE, UIAnchor::CenterHorizontal, 3.0f);
			{
				std::string watchPrefix = std::format("Watching creature #{} of species ", creature->id);
				std::string watchSpecies = std::format("N{}M{}", creature->nodeCount, creature->muscleCount);
				auto watchSpeciesColors = getSpeciesColor(creature->nodeCount, creature->muscleCount);
				const float watchFontSize = UIFontSize(1.0f);
				const float watchSpacing = UISpacing(1.0f);
				const Vector2 watchPrefixSize = MeasureTextEx(defaultFont, watchPrefix.c_str(), watchFontSize, watchSpacing);
				const Vector2 watchSpeciesSize = MeasureTextEx(defaultFont, watchSpecies.c_str(), watchFontSize, watchSpacing);
				Vector2 watchPosition = UIToScreen(absoluteWidth / 2, 48.0f);
				watchPosition.x -= (watchPrefixSize.x + watchSpeciesSize.x) * 0.5f;

				DrawTextEx(defaultFont, watchPrefix.c_str(), watchPosition, watchFontSize, watchSpacing, WHITE);
				DrawGradientText(watchSpecies, { watchPosition.x + watchPrefixSize.x, watchPosition.y }, watchFontSize, watchSpacing, watchSpeciesColors.first, watchSpeciesColors.second);
			}
			DrawTextUI(std::format("Current X position: {:.2f} meters", creature->getCenterX()/100), absoluteWidth / 2, 88.0f, 1, WHITE, UIAnchor::CenterHorizontal);
			if (watchTime > world->secondsPerSimulation) {
				DrawTextUI(std::format("Seconds: {:.2f}", watchTime), absoluteWidth / 2, 128.0f, 1, RED, UIAnchor::CenterHorizontal);
			}
			else {
				DrawTextUI(std::format("Seconds: {:.2f}", watchTime), absoluteWidth / 2, 128.0f, 1, WHITE, UIAnchor::CenterHorizontal);
			}

			if (!creaturePlaybackPaused) {
				watchTime += GetFrameTime() * creaturePlaybackSpeed;
			}

			Button slowPlaybackButton(1204, absoluteWidth / 2 - 150, 650, 110, 48, "3/4x");
			Button pausePlaybackButton(1205, absoluteWidth / 2, 650, 120, 48, creaturePlaybackPaused ? "Resume" : "Pause");
			Button fastPlaybackButton(1206, absoluteWidth / 2 + 150, 650, 110, 48, "4/3x");
			slowPlaybackButton.draw();
			pausePlaybackButton.draw();
			fastPlaybackButton.draw();
			slowPlaybackButton.tick();
			pausePlaybackButton.tick();
			fastPlaybackButton.tick();
			if (slowPlaybackButton.active) {
				creaturePlaybackSpeed = std::max(0.05f, creaturePlaybackSpeed * 0.75f);
			}
			if (pausePlaybackButton.active) {
				creaturePlaybackPaused = !creaturePlaybackPaused;
			}
			if (fastPlaybackButton.active) {
				creaturePlaybackSpeed = std::min(64.0f, creaturePlaybackSpeed * (4.0f / 3.0f));
			}
			DrawTextUI(std::format("Playback: {:.2f}x", creaturePlaybackSpeed), absoluteWidth / 2, 610, 0.8f, WHITE, UIAnchor::Center);

			Button creatureBackButton(998, absoluteWidth / 2, 720, 180, 56, "Back");
			creatureBackButton.draw();
			creatureBackButton.tick();
			if (creatureBackButton.active) {
				watchTime = 0.0f;
				creaturePlaybackSpeed = 1.0f;
				creaturePlaybackPaused = false;
				world->accumulatedTime = 0.0f;
				creature->reset();
				currentState = drawCreatureFromCurrentPopulation ? STATE_VIEW_ALL : STATE_GAME;
			}

			if (IsKeyPressed(KEY_V)) world->printCreature(*creature);
			if (IsKeyPressed(KEY_C)) SaveCreatureToBinaryFile(*creature);

			break;
		}

		// Draw notices
		{
			std::lock_guard<std::mutex> noticesLock(noticesMutex);
			for (auto it = notices.begin(); it != notices.end();) {
				auto textSize = MeasureUIText(it->text, 1);
				DrawRectUI(absoluteWidth / 2, absoluteHeight - 80, textSize.x / UIScale() + 28, 44, Fade(WHITE, 0.9f), UIAnchor::Center);
				DrawTextUI(it->text, absoluteWidth / 2, absoluteHeight - 80, 1, RED, UIAnchor::Center);
				it->duration -= GetFrameTime();
				if (it->duration <= 0) {
					it = notices.erase(it);
				}
				else {
					++it;
				}
			}
		}

		EndDrawing();
	}

	ClearGradientTextCache();

	stopContinuousGenerations.store(true);
	if (continuousGenerationFuture.valid()) {
		continuousGenerationFuture.wait();
	}

	SaveAppSettings();
	CloseWindow();
	return 0;
}
