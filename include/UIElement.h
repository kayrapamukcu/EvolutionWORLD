#pragma once
#include "raylib.h"
#include <string>

class UIElement {
public:
    int x, y, width, height, elementID;
    Rectangle drawRect;
    std::string name;
    bool active = false;

    virtual void draw() = 0;
    virtual void tick() = 0;
    virtual std::string getContent() = 0;
};