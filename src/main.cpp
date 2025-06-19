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

std::array<Creature, 1000> creatures;

void RunSimulation(int times) {
	for (int j = 0; j < times; j++) {
		for (int i = 0; i < ticksPerSecond * secondsPerSimulation * numOfCreatures; i++) {
			creatures[i % numOfCreatures].tick();
		}
		for (int i = 0; i < 1000; i++) {
			creatures[i].getCenterX();
		}
		std::sort(creatures.begin(), creatures.end(), [](Creature& a, Creature& b) {
			return a.getCenterX() < b.getCenterX();
			});
		if (times - 1 == j) {
			for (int i = 0; i < 10; i++) {
				std::cout << "Creature " << creatures[999 - i].id << ": " << creatures[999 - i].getCenterX() << "\n";
			}
		}
		for (int i = 0; i < 500; i++) {
			creatures[999 - i].reset();
		}
		for (int i = 0; i < 500; i++) {
			creatures[i] = creatures[999 - i].reproduce();
		}
	}
}

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
 
enum State {
	STATE_MENU_INIT,
	STATE_MENU,
	STATE_OPTIONS,
	STATE_ABOUT,
	STATE_CREATE,
	STATE_GAME,
};


int main() {
	State currentState = STATE_MENU_INIT;
	std::vector<Button> mainMenuButtons;

	std::vector<InputField> newGameInputFields;
	std::vector<Button> newGameButtons;
	std::vector<Slider> newGameSliders;

	std::vector<std::unique_ptr<UIElement>> UIElements;
	
	InitWindow(screenWidth, screenHeight, "EvolutionWORLD");

	SetTargetFPS(FRAMES_PER_SECOND);
	InitAudioDevice();
	Music musicMenu = LoadMusicStream("resources/mus_menu.ogg");
	defaultFont = LoadFont("resources/romulus.png");
	SetMusicVolume(musicMenu, 0.5f);
	SetMusicPitch(musicMenu, 0.0f);

	Camera2D camera = { 0 };
	camera.target = { 0, 0 };
	camera.offset = { 0, 0 };
	camera.rotation = 0.0f;
	camera.zoom = 1.0f;

	numberOfThreads = std::thread::hardware_concurrency();
	if (numberOfThreads == 0) {
		numberOfThreads = 1;
	}
	std::cout << "Processors: " << numberOfThreads << std::endl;
	std::unique_ptr<World> world;
	std::unique_ptr<World> previewWorld = std::make_unique<World>();

	PlayMusicStream(musicMenu);
	while (!WindowShouldClose()) {

		ClampWindowSize();

		if (IsKeyPressed(KEY_M)) {
			if (IsMusicStreamPlaying(musicMenu)) {
				PauseMusicStream(musicMenu);
			}
			else {
				PlayMusicStream(musicMenu);
			}
		}

		if (IsMusicStreamPlaying(musicMenu)) {
			UpdateMusicStream(musicMenu);
		}
		
		BeginDrawing();

		switch (currentState) {
		case STATE_MENU_INIT:
			//set up menu buttons
			mainMenuButtons.clear();

			mainMenuButtons.push_back(Button(0, absoluteWidth / 2, absoluteHeight / 2 - 64, 120, 24, "Create WORLD"));
			mainMenuButtons.push_back(Button(1, absoluteWidth / 2, absoluteHeight / 2 , 120, 24, "Load WORLD"));
			mainMenuButtons.push_back(Button(2, absoluteWidth / 2, absoluteHeight / 2 + 64, 120, 24, "Settings"));
			mainMenuButtons.push_back(Button(3, absoluteWidth / 2, absoluteHeight / 2 + 128, 120, 24, "About"));
			mainMenuButtons.push_back(Button(4, absoluteWidth / 2, absoluteHeight / 2 + 192, 120, 24, "Exit"));

			// set up new game input fields and buttons

			UIElements.clear();
			
			UIElements.push_back(std::make_unique<InputField>(100, 160, 112, 130, 20, "World Name"));
			UIElements.push_back(std::make_unique<InputField>(101, 160, 192, 130, 20, "World Seed"));
			UIElements.push_back(std::make_unique<InputField>(102, 90, 272, 60, 20, "BG Color", "222222", 6));
			UIElements.push_back(std::make_unique<InputField>(103, 230, 272, 60, 20, "Grnd Color", "222222", 6));
			
			UIElements.push_back(std::make_unique<Button>(0, absoluteWidth / 2, 450, 100, 25, "Create WORLD"));
			UIElements.push_back(std::make_unique<Button>(1, 60, 450, 50, 25, "Back"));
			
			UIElements.push_back(std::make_unique<Slider>(200, absoluteWidth - 160, 112, 130, 20, "Creature Count", 100, 2500, numOfCreatures));
			UIElements.push_back(std::make_unique<Slider>(201, absoluteWidth - 160, 192, 130, 20, "Ticks per Second", 50, 150, ticksPerSecond));
			UIElements.push_back(std::make_unique<Slider>(202, absoluteWidth - 160, 272, 130, 20, "Seconds per Sim", 5, 30, secondsPerSimulation));
			UIElements.push_back(std::make_unique<Slider>(203, absoluteWidth - 160, 352, 130, 20, "Muscle Speed", 20, 200, ticksToSwitchMuscleStage));

			currentState = STATE_MENU;
			break;
		case STATE_MENU:
			
			ClearBackground( PinkWORLD ); // very light pink
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
						break;
					case 2: // Settings
						currentState = STATE_OPTIONS;
						break;
					case 3: // About
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

			for (int i = 0; i < UIElements.size(); i++) {
				UIElements[i]->tick();
				UIElements[i]->draw();

				if (UIElements[i]->active) {
					switch (UIElements[i]->elementID) {
					case 0: // Create World
					{
						int bgColor = std::stoi(UIElements[2]->getContent(), nullptr, 16);
						int grColor = std::stoi(UIElements[3]->getContent(), nullptr, 16);
						Color backgroundColor = { (unsigned char)((bgColor >> 16) & 0xFF), (unsigned char)((bgColor >> 8) & 0xFF), (unsigned char)(bgColor & 0xFF), 255 };
						Color groundColor = { (unsigned char)((grColor >> 16) & 0xFF), (unsigned char)((grColor >> 8) & 0xFF), (unsigned char)(grColor & 0xFF), 255 };

						world = std::make_unique<World>(
							UIElements[0]->getContent(), // worldName
							UIElements[1]->getContent(), // worldSeed
							std::stoi(UIElements[6]->getContent()), // numOfCreatures
							std::stoi(UIElements[7]->getContent()), // ticksPerSecond
							std::stoi(UIElements[8]->getContent()), // secondsPerSimulation
							std::stoi(UIElements[9]->getContent()), // ticksToSwitchMuscleStage
							backgroundColor,
							groundColor
						);
						currentState = STATE_GAME;
						break;
					}
					case 1: // Back
					{
						currentState = STATE_MENU_INIT;
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

			ClearBackground(LIGHTGRAY);
			
			DrawTextB("World " + world->worldName + " ,Seed " + std::to_string(world->worldSeed) + " ,Generation " + std::to_string(world->generation), 24, 24, 1, BLACK);
			
			if (IsKeyPressed(KEY_B)) {
				currentState = STATE_MENU_INIT;
			}
			if (IsKeyPressed(KEY_R)) {
				world->DoGeneration();
			}
			if (IsKeyDown(KEY_L)) {
				BeginMode2D(camera);
				world->DrawCreature();
				EndMode2D();
			}

			if (IsKeyPressed(KEY_C)) {
				auto time = std::chrono::high_resolution_clock::now();
				for (int i = 0; i < 10; i++) {
					world->DoGeneration();
				}
				auto endTime = std::chrono::high_resolution_clock::now();
				auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - time);
				std::cout << "10 generations took " << duration.count() << " ms\n";
			}
			//world->DrawCentered(absoluteWidth / 2, absoluteHeight / 2, absoluteWidth, absoluteHeight);
			
			break;
		}
		EndDrawing();
	}
	CloseWindow();
	return 0;
}