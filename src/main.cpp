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
#include <string>
#include <format>

void ClampWindowSize() {
	screenWidth = GetScreenWidth();
	screenHeight = GetScreenHeight();

	screenWidthRatio = float(screenWidth) / float(absoluteWidth);
	screenHeightRatio = float(screenHeight) / float(absoluteHeight);

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

	int guiScaleWidth = screenWidth * 2 / float(minWidth);
	int guiScaleHeight = screenHeight * 2 / float(minHeight);

	guiScale = std::min(guiScaleWidth, guiScaleHeight);

	if (needsResize) {
		SetWindowSize(screenWidth, screenHeight);
	}
}

struct Notice {
	std::string text;
	float duration;
};
 
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
	std::vector<Button> mainMenuButtons;

	std::vector<InputField> newGameInputFields;
	std::vector<Button> newGameButtons;
	std::vector<Slider> newGameSliders;

	std::vector<std::unique_ptr<UIElement>> createMenuUIElements;
	std::vector<std::unique_ptr<UIElement>> ingameUIElements;

	std::vector<Notice> notices;
	
	InitWindow(screenWidth, screenHeight, "EvolutionWORLD");

	SetTargetFPS(FRAMES_PER_SECOND);
	InitAudioDevice();
	Music musicMenu = LoadMusicStream("resources/mus_menu.ogg");
	defaultFont = LoadFont("resources/romulus.png");
	SetMusicVolume(musicMenu, 0.5f);
	SetMusicPitch(musicMenu, 0.0f);

	numberOfThreads = std::thread::hardware_concurrency();
	if (numberOfThreads == 0) {
		numberOfThreads = 1;
	}
	std::cout << "Processors: " << numberOfThreads << std::endl;
	std::unique_ptr<World> world;
	std::unique_ptr<World> previewWorld = std::make_unique<World>();

	int creatureIndexToBeDrawn = 0;
	float watchTime = 0.0f;
	bool doGenerationsNonstop = false;
	float timeForOneGen = 0.0f;

	float frameTime = GetFrameTime();
	float currentTime = GetTime();
	
	while (!WindowShouldClose()) {

		ClampWindowSize();

		if (IsMusicStreamPlaying(musicMenu)) {
			UpdateMusicStream(musicMenu);
		}
		
		BeginDrawing();

		switch (currentState) {
		case STATE_MENU_INIT:
			if (!IsMusicStreamPlaying(musicMenu)) PlayMusicStream(musicMenu);
			//set up menu buttons

			mainMenuButtons.clear();

			mainMenuButtons.push_back(Button(0, absoluteWidth / 2, absoluteHeight / 2 - 64, 120, 24, "Create WORLD"));
			mainMenuButtons.push_back(Button(1, absoluteWidth / 2, absoluteHeight / 2 , 120, 24, "Load WORLD"));
			mainMenuButtons.push_back(Button(2, absoluteWidth / 2, absoluteHeight / 2 + 64, 120, 24, "Settings"));
			mainMenuButtons.push_back(Button(3, absoluteWidth / 2, absoluteHeight / 2 + 128, 120, 24, "About"));
			mainMenuButtons.push_back(Button(4, absoluteWidth / 2, absoluteHeight / 2 + 192, 120, 24, "Exit"));

			// set up new game input fields and buttons

			createMenuUIElements.clear();
			
			createMenuUIElements.push_back(std::make_unique<InputField>(100, 160, 112, 130, 20, "World Name"));
			createMenuUIElements.push_back(std::make_unique<InputField>(101, 160, 192, 130, 20, "World Seed"));
			createMenuUIElements.push_back(std::make_unique<InputField>(102, 90, 272, 60, 20, "BG Color", "A9A9A9", 6));
			createMenuUIElements.push_back(std::make_unique<InputField>(103, 230, 272, 60, 20, "Grnd Color", "111111", 6));
			
			createMenuUIElements.push_back(std::make_unique<Button>(0, absoluteWidth / 2, 450, 100, 25, "Create WORLD"));
			createMenuUIElements.push_back(std::make_unique<Button>(1, 60, 450, 50, 25, "Back"));
			
			createMenuUIElements.push_back(std::make_unique<Slider>(200, absoluteWidth - 160, 112, 130, 20, "Creature Count", 100, 2500, numOfCreatures));
			createMenuUIElements.push_back(std::make_unique<Slider>(201, absoluteWidth - 160, 192, 130, 20, "Ticks per Second", 50, 150, ticksPerSecond));
			createMenuUIElements.push_back(std::make_unique<Slider>(202, absoluteWidth - 160, 272, 130, 20, "Seconds per Sim", 5, 30, secondsPerSimulation));
			createMenuUIElements.push_back(std::make_unique<Slider>(203, absoluteWidth - 160, 352, 130, 20, "Muscle Speed", 20, 200, ticksToSwitchMuscleStage));
			createMenuUIElements.push_back(std::make_unique<Slider>(204, absoluteWidth - 160, 432, 130, 20, "Gravity", 20, 1000, gravityConst));

			// ingame UI elements setup

			ingameUIElements.clear();

			ingameUIElements.push_back(std::make_unique<Button>(0, 144, absoluteHeight - 34, 120, 14, "Back"));
			ingameUIElements.push_back(std::make_unique<Button>(1, 144, absoluteHeight - 66, 120, 14, "Save"));
			ingameUIElements.push_back(std::make_unique<Button>(2, 427, absoluteHeight - 50, 120, 30, "Next Gen"));
			ingameUIElements.push_back(std::make_unique<Button>(3, 710, absoluteHeight - 50, 120, 30, "Do Gens Cont."));

			ingameUIElements.push_back(std::make_unique<Button>(4, 144, absoluteHeight - 120, 120, 30, "Watch Worst Crtr"));
			ingameUIElements.push_back(std::make_unique<Button>(5, 427, absoluteHeight - 120, 120, 30, "Watch Avg Crtr"));
			ingameUIElements.push_back(std::make_unique<Button>(6, 710, absoluteHeight - 120, 120, 30, "Watch Best Crtr"));

			ingameUIElements.push_back(std::make_unique<Slider>(200, 204, 250, 180, 30, "View Generation", 0, 0, 0));


			currentState = STATE_MENU;
			break;
		case STATE_MENU:
			
			ClearBackground( PinkWORLD );
			DrawTextCentered("EvolutionWORLD " + std::string(versionString), absoluteWidth / 2, 32, 2, BLACK);	
			for (int i = 0; i < mainMenuButtons.size(); i++) {

				mainMenuButtons[i].draw();
				mainMenuButtons[i].tick();

				if (mainMenuButtons[i].active) {
					switch (mainMenuButtons[i].elementID) {
					case 0: // Create World
						currentState = STATE_CREATE;
						break;
					case 1: // Load World
						notices.push_back({ "Saving / loading worlds is not implemented yet!", 2.0f });
						break;
					case 2: // Settings
						notices.push_back({ "Settings are not implemented yet!", 2.0f });
						//currentState = STATE_OPTIONS;
						break;
					case 3: // About
						notices.push_back({ "About screen not implemented yet!", 2.0f });
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
			DrawTextCentered("Options", absoluteWidth / 2, 32, 2, BLACK);
			
			if (IsKeyPressed(KEY_B)) {
				currentState = STATE_MENU;
			}
			break;
		case STATE_CREATE:
			ClearBackground(LightBlueWORLD);
			DrawTextCentered("Create WORLD", absoluteWidth / 2, 32, 2, BLACK);

			for (int i = 0; i < createMenuUIElements.size(); i++) {
				createMenuUIElements[i]->tick();
				createMenuUIElements[i]->draw();

				if (createMenuUIElements[i]->active) {
					switch (createMenuUIElements[i]->elementID) {
					case 0: // Create World
					{
						if (createMenuUIElements[0]->getContent().empty()) {
							notices.push_back({ "World name cannot be empty!", 3.0f });
							break;
						}
						int bgColor, grColor;
						try {
							bgColor = std::stoi(createMenuUIElements[2]->getContent(), nullptr, 16);
							grColor = std::stoi(createMenuUIElements[3]->getContent(), nullptr, 16);
						}
						catch (const std::invalid_argument&) {
							notices.push_back({ "Invalid color code! Use hex format (e.g. A9A9A9)", 3.0f });
							break;
						}
						catch (const std::out_of_range&) {
							notices.push_back({ "Color code out of range! Use hex format (e.g. A9A9A9)", 3.0f });
							break;
						}
						catch (const std::exception& e) {
							notices.push_back({ "Error: " + std::string(e.what()), 3.0f });
							break;
						}
					
						Color backgroundColor = ColorFromInt(bgColor);
						Color groundColor = ColorFromInt(grColor);

						world = std::make_unique<World>(
							createMenuUIElements[0]->getContent(), // worldName
							createMenuUIElements[1]->getContent(), // worldSeed
							std::stoi(createMenuUIElements[10]->getContent()), // gravity
							std::stoi(createMenuUIElements[6]->getContent()), // numOfCreatures
							std::stoi(createMenuUIElements[7]->getContent()), // ticksPerSecond
							std::stoi(createMenuUIElements[8]->getContent()), // secondsPerSimulation
							std::stoi(createMenuUIElements[9]->getContent()), // ticksToSwitchMuscleStage
							backgroundColor,
							groundColor
						);
						currentState = STATE_GAME;
						StopMusicStream(musicMenu);
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
							previewWorld->backgroundColor = ColorFromInt(std::stoi(createMenuUIElements[2]->getContent(), nullptr, 16));
						}
						break;
					}
					case 103:
					{
						// Input field for ground color
						std::string content = createMenuUIElements[i]->getContent();
						if (content.length() == 6 && std::all_of(content.begin(), content.end(), ::isxdigit)) {
							previewWorld->groundColor = ColorFromInt(std::stoi(createMenuUIElements[3]->getContent(), nullptr, 16));
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
			DrawTextCentered("Preview WORLD", absoluteWidth / 2, 100, 1, BLACK);
			previewWorld->DrawCentered(absoluteWidth / 2, absoluteHeight / 2, 110, 110);
			DrawRectangleCenteredLines(absoluteWidth / 2, absoluteHeight / 2, 110, 110, 2, BLACK);
			break;
		case STATE_GAME:

			ClearBackground(BeigeWORLD);
			DrawTextB("On World '" + world->worldName + "' with seed " + std::to_string(world->worldSeed), 24, 24, 1, BLACK);
			
			DrawTextB("Generation: " + std::to_string(world->generation), 24, 48, 2, BLACK);
			DrawTextB(std::format("Worst fitness: {:.2f} meters", world->worstGenerationalCreatures[world->viewGeneration].fitness / 100), 24, 96, 1, BLACK);
			DrawTextB(std::format("Average fitness: {:.2f} meters", world->averageGenerationalCreatures[world->viewGeneration].fitness / 100), 24, 120, 1, BLACK);
			DrawTextB(std::format("Best fitness: {:.2f} meters", world->bestGenerationalCreatures[world->viewGeneration].fitness / 100), 24, 144, 1, BLACK);
			
			for (int i = 0; i < ingameUIElements.size(); i++) {
				ingameUIElements[i]->tick();
				ingameUIElements[i]->draw();
				if (ingameUIElements[i]->active && !doGenerationsNonstop) {
					switch (ingameUIElements[i]->elementID) {
					case 0: // Main Menu
						notices.clear();
						currentState = STATE_MENU_INIT;
						break;
					case 1: // Save
						notices.push_back({ "Save functionality not implemented yet.", 3.0f });
						break;
					case 2: // Next Generation
						timeForOneGen = GetTime();
						world->DoGeneration();
						timeForOneGen = GetTime() - timeForOneGen;
						std::cout << "Generation " << world->generation << " took " << timeForOneGen << " seconds." << std::endl;
						dynamic_cast<Slider*>(ingameUIElements[7].get())->maxVal = world->generation;
						dynamic_cast<Slider*>(ingameUIElements[7].get())->curVal = world->generation;
						break;
					case 3: // Do Generations Nonstop
						SetTargetFPS(0);
						doGenerationsNonstop = true;
						notices.push_back({ "Doing generations nonstop; hold L to stop", 99999999.0f });
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
					case 200: // View Generation Slider
						world->viewGeneration = std::stoi(ingameUIElements[i]->getContent());
						break;
					}
				}
			}
			
			if (doGenerationsNonstop) {
				world->DoGeneration();
				dynamic_cast<Slider*>(ingameUIElements[7].get())->maxVal = world->generation;
				dynamic_cast<Slider*>(ingameUIElements[7].get())->curVal = world->generation;
			}

			if (IsKeyDown(KEY_L) && doGenerationsNonstop) {
				SetTargetFPS(FRAMES_PER_SECOND);
				doGenerationsNonstop = false;
				notices.clear();
			}
			
			break;
		case STATE_DRAW_CREATURE:
			ClearBackground(world->backgroundColor);
			auto creature = world->DrawWithCreatureCentered(creatureIndexToBeDrawn, world->viewGeneration);

			DrawTextB(std::format("Watching creature #{:}",creature->id), 24, 24, 1, BLACK);
			DrawTextB(std::format("Current X position: {:.2f} meters", creature->getCenterX()/100), 24, 48, 1, BLACK);
			if (watchTime > world->secondsPerSimulation) {
				DrawTextB(std::format("Seconds: {:.2f}", watchTime), 24, 72, 1, RED);
			}
			else {
				DrawTextB(std::format("Seconds: {:.2f}", watchTime), 24, 72, 1, BLACK);
			}

			watchTime += GetFrameTime();

			if (IsKeyPressed(KEY_B)) {
				watchTime = 0.0f;
				creature->reset();
				currentState = STATE_GAME;
			}
			
			DrawTextB("Press B to go back!", 20, absoluteHeight - 32, 1, WHITE);

			break;
		}

		// Draw notices
		for (auto it = notices.begin(); it != notices.end();) {
			auto textLength = MeasureText(it->text.c_str(), guiScale*8);
			DrawRectangleCentered(absoluteWidth / 2, absoluteHeight - 50, textLength, 20, Fade(WHITE, 0.9f));
			DrawTextCentered(it->text, absoluteWidth / 2, absoluteHeight - 50, 1, RED);
			it->duration -= GetFrameTime();
			if (it->duration <= 0) {
				it = notices.erase(it);
			}
			else {
				++it;
			}
		}

		EndDrawing();
	}
	CloseWindow();
	return 0;
}