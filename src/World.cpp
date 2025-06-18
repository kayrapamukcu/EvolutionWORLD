#include "World.h"
#include "Helper.h"
#include "Creature.h"

void World::Draw(int x, int y, int width, int height) {
	DrawRectangleB(x, y, width, height, backgroundColor);
	DrawRectangleB(x, y + height - 10, width, 10, groundColor);
}

void World::DrawCentered(int x, int y, int width, int height) {
	DrawRectangleCentered(x, y, width, height, backgroundColor);
	DrawRectangleCentered(x, y + 4*height/5 / screenHeightRatio, width, height/5, groundColor);
}
