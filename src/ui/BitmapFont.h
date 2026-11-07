#pragma once

#include "core/Renderer.h"
#include <string>

// Lightweight 3x5 retro pixel font renderer.
// Allows drawing crisp in-game text, alerts, and numbers without external TTF dependencies.
class BitmapFont {
public:
    // Draws a single character at (x, y) with the specified scale and color.
    static void drawChar(Renderer& renderer, char c, int x, int y, SDL_Color color, int scale = 2);

    // Draws a text string at (x, y) with the specified scale and color.
    static void drawText(Renderer& renderer, const std::string& text, int x, int y, SDL_Color color, int scale = 2);

    // Draws an integer number at (x, y).
    static void drawNumber(Renderer& renderer, int number, int x, int y, SDL_Color color, int scale = 2);

    // Calculates the rendered pixel width of the given string.
    static int getTextWidth(const std::string& text, int scale = 2);
};
