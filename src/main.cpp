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
	
	InitWindow(screenWidth, screenHeight, "EvolutionWORLD");
	// InitWindow(800, 600, "EvolutionWORLD");

	SetTargetFPS(FRAMES_PER_SECOND);
	InitAudioDevice();
	Music musicMenu = LoadMusicStream("resources/mus_menu.ogg");
	defaultFont = LoadFont("resources/romulus.png");
	SetMusicVolume(musicMenu, 0.5f);
	SetMusicPitch(musicMenu, 0.0f);

	Camera2D camera = { 0 };
	camera.target = { 0, 0 };
	camera.offset = { (float)screenWidth / 2.0f, (float)screenHeight / 2.0f };
	camera.rotation = 0.0f;
	camera.zoom = 1.0f;

	numberOfThreads = std::thread::hardware_concurrency();
	if (numberOfThreads == 0) {
		numberOfThreads = 1;
	}
	std::cout << "Processors: " << numberOfThreads << std::endl;
	std::unique_ptr<World> world;
	PlayMusicStream(musicMenu);
	while (!WindowShouldClose()) {

		screenWidth = GetScreenWidth();
		screenHeight = GetScreenHeight();

		screenWidthRatio = float(screenWidth) / float(absoluteWidth);
		screenHeightRatio = float(screenHeight) / float(absoluteHeight);

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
		World previewWorld = World(); // preview world
		BeginDrawing();

		switch (currentState) {
		case STATE_MENU_INIT:
			//set up menu buttons
			mainMenuButtons.clear();

			mainMenuButtons.push_back(Button(0, absoluteWidth / 2, absoluteHeight / 2 - 64, 120, 24, "Create WORLD"));
			mainMenuButtons.push_back(Button(1, absoluteWidth / 2, absoluteHeight / 2 , 120, 24, "Load WORLD"));
			mainMenuButtons.push_back(Button(2, absoluteWidth / 2, absoluteHeight / 2 + 64, 120, 24, "Settings"));
			mainMenuButtons.push_back(Button(3, absoluteWidth / 2, absoluteHeight / 2 + 96, 120, 24, "About"));
			mainMenuButtons.push_back(Button(4, absoluteWidth / 2, absoluteHeight / 2 + 128, 120, 24, "Exit"));

			// set up new game input fields and buttons
			newGameButtons.clear();
			newGameInputFields.clear();
			newGameSliders.clear();

			newGameInputFields.push_back(InputField(0, 160, 112, 130, 20, "World Name"));
			newGameInputFields.push_back(InputField(1, 160, 192, 130, 20, "World Seed"));
			newGameInputFields.push_back(InputField(2, 90, 272, 60, 20, "BG Color", "222222", 6));
			newGameInputFields.push_back(InputField(3, 230, 272, 60, 20, "Grnd Color", "222222", 6));

			newGameButtons.push_back(Button(0, absoluteWidth / 2, 450, 100, 25, "Create WORLD"));
			newGameButtons.push_back(Button(1, 60, 450, 50, 25, "Back"));

			newGameSliders.push_back(Slider(0, absoluteWidth - 160, 112, 130, 20, "Creature Count", 100, 2500, numOfCreatures));
			newGameSliders.push_back(Slider(1, absoluteWidth - 160, 192, 130, 20, "Ticks per Second", 50, 150, ticksPerSecond));
			newGameSliders.push_back(Slider(2, absoluteWidth - 160, 272, 130, 20, "Seconds per Sim", 5, 30, secondsPerSimulation));
			newGameSliders.push_back(Slider(3, absoluteWidth - 160, 352, 130, 20, "Muscle Speed", 50, 200, ticksToSwitchMuscleStage));


			currentState = STATE_MENU;
			break;
		case STATE_MENU:
			
			ClearBackground( PinkWORLD ); // very light pink
			DrawTextCentered("EvolutionWORLD " + std::string(versionString), absoluteWidth / 2, 32, 2, BLACK);	
			for (int i = 0; i < mainMenuButtons.size(); i++) {

				mainMenuButtons[i].draw();
				auto clicked = mainMenuButtons[i].checkCollision();
				if (clicked) {
					switch (mainMenuButtons[i].buttonID) {
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
			DrawTextCentered("Press M to toggle music", absoluteWidth / 2, 64, 1, BLACK);
			DrawTextCentered("Press B to go back", absoluteWidth / 2, 96, 1, BLACK);
			if (IsKeyPressed(KEY_B)) {
				currentState = STATE_MENU;
			}
			break;
		case STATE_CREATE:
			ClearBackground(LightBlueWORLD);
			DrawTextCentered("Create WORLD", absoluteWidth / 2, 32, 2, BLACK);

			if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
				for (int i = 0; i < newGameInputFields.size(); i++) {
					newGameInputFields[i].active = false;
				}
			}

			for (int i = 0; i < newGameInputFields.size(); i++) {
				if (newGameInputFields[i].active) {
					newGameInputFields[i].tick();
				}
				newGameInputFields[i].checkCollision();
				newGameInputFields[i].draw();
			}

			for (int i = 0; i < newGameButtons.size(); i++) {
				newGameButtons[i].draw();
				auto clicked = newGameButtons[i].checkCollision();
				if (clicked) {
					switch (newGameButtons[i].buttonID) {
					case 0: // Create World
					{
						int bgColor = std::stoi(newGameInputFields[2].text, nullptr, 16);
						int grColor = std::stoi(newGameInputFields[3].text, nullptr, 16);
						Color backgroundColor = { (unsigned char)((bgColor >> 16) & 0xFF), (unsigned char)((bgColor >> 8) & 0xFF), (unsigned char)(bgColor & 0xFF), 255 };
						Color groundColor = { (unsigned char)((grColor >> 16) & 0xFF), (unsigned char)((grColor >> 8) & 0xFF), (unsigned char)(grColor & 0xFF), 255 };
						world = std::make_unique<World>(
							newGameInputFields[0].text, // worldName
							newGameInputFields[1].text, // worldSeed
							newGameSliders[0].curVal, // numOfCreatures
							newGameSliders[1].curVal, // ticksPerSecond
							newGameSliders[2].curVal, // secondsPerSimulation
							newGameSliders[3].curVal, // ticksToSwitchMuscleStage
							backgroundColor,
							groundColor
						);
						Creature::world = world.get();
						currentState = STATE_GAME;
						break;
					}
					case 1: // Back
					{
						currentState = STATE_MENU_INIT;
						break;
						}
					}
				}
			}

			for (int i = 0; i < newGameSliders.size(); i++) {
				if (newGameSliders[i].active || newGameSliders[i].precise) {
					newGameSliders[i].tick();
				}
				newGameSliders[i].checkCollision();
				newGameSliders[i].draw();
			}

			// Draw preview world
			DrawTextCentered("Preview WORLD", absoluteWidth / 2, 100, 1, BLACK);
			previewWorld.DrawCentered(absoluteWidth / 2, absoluteHeight / 2, 110, 110);
			DrawRectangleCenteredLines(absoluteWidth / 2, absoluteHeight / 2, 110, 110, 2, BLACK);
			break;
		case STATE_GAME:

			ClearBackground(LIGHTGRAY);
			
			DrawTextB("World " + world->worldName + " ,Seed " + std::to_string(world->worldSeed) + " ,Generation " + std::to_string(world->generation), 24, 24, 1, BLACK);
			BeginMode2D(camera);
			if (IsKeyPressed(KEY_B)) {
				currentState = STATE_MENU_INIT;
			}
			
			//world->DrawCentered(absoluteWidth / 2, absoluteHeight / 2, absoluteWidth, absoluteHeight);

			EndMode2D();
			
			break;
		}
		if (IsKeyPressed(KEY_SPACE)) {
			guiScale += 1;
		}
		if (IsKeyPressed(KEY_R)) {
			guiScale -= 1;
		}
		EndDrawing();

		
	}
	CloseWindow();
	return 0;
}