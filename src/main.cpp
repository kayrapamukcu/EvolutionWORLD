#include <iostream>
#include "Creature.h"
#include "Raylib.h"
#include "Helper.h"
#include <array>
#include <algorithm>
#include "Button.h"
#include "InputField.h"
#include <vector>
#include "World.h"
#include "Slider.h"
#include "UIElement.h"
#include <thread>
#include <atomic>
#include <chrono>
#include <string>
#include <format>
#include <future>

float guiScalePrev = 1.0f;
std::unique_ptr<World> world;
const char* versionString = "v1.5.0";

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

	if (guiScalePrev != guiScale) {
		justResized = true;
		std::cout << " New GUI scale!!!!!!!!!!!!!!!!\n";}
	else justResized = false;

	guiScalePrev = guiScale;
	if (needsResize) {
		SetWindowSize(screenWidth, screenHeight);
	}
}
 
enum State {
	STATE_MENU_INIT,
	STATE_MENU,
	STATE_OPTIONS,
	STATE_ABOUT,
	STATE_CREATE,
	STATE_GAME,
	STATE_DRAW_CREATURE
};


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
	
	SetConfigFlags(FLAG_MSAA_4X_HINT);
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(screenWidth, screenHeight, "EvolutionWORLD");

	SetTargetFPS(FRAMES_PER_SECOND);
	InitAudioDevice();
	Music musicMenu = LoadMusicStream("resources/mus_menu.ogg");
	std::vector<int> uiFontCodepoints = CreateUIFontCodepoints();
	defaultFont = LoadFontEx("resources/Lato-Regular.ttf", 72, uiFontCodepoints.data(), (int)uiFontCodepoints.size());

	SetMusicVolume(musicMenu, appSettings.musicVolume / 100.0f);
	SetMusicPitch(musicMenu, 0.0f);

	numberOfThreads = std::thread::hardware_concurrency();
	if (numberOfThreads == 0) {
		numberOfThreads = 1;
	}
	std::cout << "Processors: " << numberOfThreads << std::endl;

	MiniWorld previewWorld;

	int creatureIndexToBeDrawn = 0;
	float watchTime = 0.0f;
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
		DrawRectUI(absoluteWidth / 2, y + 15, 360, 86, { 0, 0, 0, 255 }, UIAnchor::Center);
		DrawTextUI(text, absoluteWidth / 2, y - 4, 1, RED, UIAnchor::Center);

		int activeDot = (int)(GetTime() * 4.0) % 3;
		for (int i = 0; i < 3; ++i) {
			Vector2 dotPosition = UIToScreen(absoluteWidth / 2 - 22 + i * 22, y + 28);
			float radius = (i == activeDot ? 5.0f : 3.5f) * UIScale();
			DrawCircleV(dotPosition, radius, i == activeDot ? RED : Fade(RED, 0.45f));
		}
	};

	float frameTime = GetFrameTime();
	float currentTime = GetTime();
	
	while (!WindowShouldClose()) {

		ClampWindowSize();

		if (IsMusicStreamPlaying(musicMenu)) {
			UpdateMusicStream(musicMenu);
		}
		SetMusicVolume(musicMenu, appSettings.musicVolume / 100.0f);

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
				dynamic_cast<Slider*>(ingameUIElements[7].get())->maxVal = world->generation;
				dynamic_cast<Slider*>(ingameUIElements[7].get())->curVal = world->generation;
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
				dynamic_cast<Slider*>(ingameUIElements[7].get())->maxVal = world->generation;
				dynamic_cast<Slider*>(ingameUIElements[7].get())->curVal = world->generation;
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

			// ingame UI elements setup

			ingameUIElements.clear();

			ingameUIElements.push_back(std::make_unique<Button>(0, 175, 720, 310, 64, "Back"));
			ingameUIElements.push_back(std::make_unique<Button>(1, 512, 720, 310, 64, "Save"));
			ingameUIElements.push_back(std::make_unique<Button>(2, 257, 520, 474, 64, "Next Generation"));
			ingameUIElements.push_back(std::make_unique<Button>(3, 768, 520, 474, 64, "Do Gens Continuously"));
																			  
			ingameUIElements.push_back(std::make_unique<Button>(4, 175, 620, 310, 64, "Watch Worst Creature"));
			ingameUIElements.push_back(std::make_unique<Button>(5, 512, 620, 310, 64, "Watch Avg Creature"));
			ingameUIElements.push_back(std::make_unique<Button>(6, 850, 620, 310, 64, "Watch Best Creature"));

			ingameUIElements.push_back(std::make_unique<Slider>(200, 300, 350, 528, 80, "View Generation", 0, 0, 0));
			ingameUIElements.push_back(std::make_unique<Button>(7, 850, 720, 310, 64, "Settings"));

			settingsUIElements.clear();

			settingsUIElements.push_back(std::make_unique<Slider>(200, absoluteWidth / 2, 220, 420, 56, "Music Volume", 0, 100, appSettings.musicVolume));
			settingsUIElements.push_back(std::make_unique<Slider>(201, absoluteWidth / 2, 310, 420, 56, "Sound Volume", 0, 100, appSettings.soundVolume));
			settingsUIElements.push_back(std::make_unique<Button>(0, absoluteWidth / 2, 405, 340, 56, ""));
			settingsUIElements.push_back(std::make_unique<Button>(1, absoluteWidth / 2, 485, 420, 56, ""));
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
						CloseWindow();
						return 0;
					}
				}
			}
			break;
		case STATE_OPTIONS:
			ClearBackground(LightBlueWORLD);
			DrawTextUI("Settings", absoluteWidth / 2, 90, 2, BLACK, UIAnchor::Center);

			dynamic_cast<Button*>(settingsUIElements[2].get())->name = appSettings.fullscreen ? "Fullscreen: On" : "Fullscreen: Off";
			dynamic_cast<Button*>(settingsUIElements[3].get())->name = appSettings.showGenerationsPerSecond ? "Show Gens/Sec: On" : "Show Gens/Sec: Off";

			for (int i = 0; i < settingsUIElements.size(); i++) {
				settingsUIElements[i]->tick();
				settingsUIElements[i]->draw();

				if (settingsUIElements[i]->active) {
					switch (settingsUIElements[i]->elementID) {
					case 0:
						appSettings.fullscreen = !appSettings.fullscreen;
						ApplyFullscreenSetting();
						break;
					case 1:
						appSettings.showGenerationsPerSecond = !appSettings.showGenerationsPerSecond;
						break;
					case 3:
						currentState = settingsReturnState;
						break;
					case 200:
						appSettings.musicVolume = std::stoi(settingsUIElements[i]->getContent());
						break;
					case 201:
						appSettings.soundVolume = std::stoi(settingsUIElements[i]->getContent());
						break;
					}
				}
			}
			
			if (IsKeyPressed(KEY_B)) {
				currentState = settingsReturnState;
			}
			break;
		case STATE_CREATE:
			ClearBackground(LightBlueWORLD);
			DrawTextUI("Create WORLD", absoluteWidth / 2, 90, 2, BLACK, UIAnchor::Center);

			for (int i = 0; i < createMenuUIElements.size(); i++) {
				createMenuUIElements[i]->tick();
				createMenuUIElements[i]->draw();

				if (createMenuUIElements[i]->active && !createInProgress) {
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
								backgroundColor,
								groundColor
							);
							});
						break;
					}
					case 1: // Back
					{
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

			// Draw preview world
			DrawTextUI("Preview", absoluteWidth / 2, 240, 1.2f, BLACK, UIAnchor::Center);
			previewWorld.drawtick(absoluteWidth / 2, 395, 240, 240);
			DrawRectUI(absoluteWidth / 2, 395, 240, 240, BLACK, UIAnchor::Center, 2);
			if (createInProgress) {
				drawBusyPanel("Loading world...", 678);
			}
			break;
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
				{
					std::lock_guard<std::mutex> dataLock(world->dataMutex);
					currentGeneration = world->generation;
				}
				dynamic_cast<Slider*>(ingameUIElements[7].get())->maxVal = currentGeneration;
				dynamic_cast<Slider*>(ingameUIElements[7].get())->curVal = currentGeneration;

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
				std::unique_lock<std::mutex> dataLock(world->dataMutex, std::try_to_lock);
				if (dataLock.owns_lock()) {
					DrawTextUI("On World '" + world->worldName + "' with seed " + std::to_string(world->worldSeed), 36, 36, 1, BLACK);
					DrawTextUI("Generation: " + std::to_string(world->generation), 36, 72, 2, BLACK);
					DrawTextUI(std::format("Worst fitness: {:.2f} meters", world->worstGenerationalCreatures[world->viewGeneration].fitness / 100), 36, 160, 1, BLACK);
					DrawTextUI(std::format("Average fitness: {:.2f} meters", world->averageGenerationalCreatures[world->viewGeneration].fitness / 100), 36, 200, 1, BLACK);
					DrawTextUI(std::format("Best fitness: {:.2f} meters", world->bestGenerationalCreatures[world->viewGeneration].fitness / 100), 36, 240, 1, BLACK);
				}
			}

			{
				std::unique_lock<std::mutex> dataLock(world->dataMutex, std::try_to_lock);
				if (dataLock.owns_lock()) {
					world->percentileGraph.draw();
				}
			}

			for (int i = 0; i < ingameUIElements.size(); i++) {
				ingameUIElements[i]->tick();
				ingameUIElements[i]->draw();
				if (ingameUIElements[i]->active && !doGenerationsNonstop && !world->IsGenerationInProgress() && !saveInProgress) {
					switch (ingameUIElements[i]->elementID) {
					case 0: // Main Menu
						ClearNotices();
						currentState = STATE_MENU_INIT;

						break;
					case 1: // Save
						saveInProgress = true;
						saveWorkStarted.store(false);
						saveFuture = std::async(std::launch::async, [worldPtr = world.get(), &saveWorkStarted] {
							worldPtr->Save(&saveWorkStarted);
							});
						break;
					case 2: // Next Generation
						if (world->StartGeneration()) {
							generationStartTime = GetTime();
						}
						break;
					case 3: // Do Generations Nonstop
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
					case 4: // See Worst Creature
						creatureIndexToBeDrawn = 0;
						currentState = STATE_DRAW_CREATURE;
						break;
					case 5: // See Average Creature
						creatureIndexToBeDrawn = 1;
						currentState = STATE_DRAW_CREATURE;
						break;
					case 6: // See Best Creature
						creatureIndexToBeDrawn = 2;
						currentState = STATE_DRAW_CREATURE;
						break;
					case 7: // Options
						settingsReturnState = STATE_GAME;
						currentState = STATE_OPTIONS;
						break;
					case 200: // View Generation Slider
						world->viewGeneration = std::stoi(ingameUIElements[i]->getContent());
						world->percentileGraph.updateExtremeValues();
						break;
					}
				}
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
				DrawTextUI(std::format("Generations/sec: {:.2f}", generationsPerSecond), absoluteWidth - 300, 30, 0.75, BLACK);
			}

			if (saveInProgress && saveWorkStarted.load()) {
				drawBusyPanel("Saving world...", 678);
			}

			break;
		}
		case STATE_DRAW_CREATURE:
			ClearBackground(world->backgroundColor);
			auto creature = world->DrawWithCreatureCentered(creatureIndexToBeDrawn, world->viewGeneration);

			DrawTextUI(std::format("Watching creature #{:}",creature->id), 36, 36, 1, BLACK);
			DrawTextUI(std::format("Current X position: {:.2f} meters", creature->getCenterX()/100), 36, 76, 1, BLACK);
			if (watchTime > world->secondsPerSimulation) {
				DrawTextUI(std::format("Seconds: {:.2f}", watchTime), 36, 116, 1, RED);
			}
			else {
				DrawTextUI(std::format("Seconds: {:.2f}", watchTime), 36, 116, 1, BLACK);
			}

			watchTime += GetFrameTime();

			Button creatureBackButton(998, absoluteWidth / 2, 720, 180, 56, "Back");
			creatureBackButton.draw();
			creatureBackButton.tick();
			if (creatureBackButton.active) {
				watchTime = 0.0f;
				creature->reset();
				currentState = STATE_GAME;
			}

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

	stopContinuousGenerations.store(true);
	if (continuousGenerationFuture.valid()) {
		continuousGenerationFuture.wait();
	}

	CloseWindow();
	return 0;
}
