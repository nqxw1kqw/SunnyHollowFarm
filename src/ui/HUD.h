#pragma once

#include "core/Renderer.h"
#include "entities/Inventory.h"

// Heads-Up Display showing the active tool hotbar, seed count, and harvested produce.
class HUD {
public:
    HUD() = default;

    // Renders the hotbar and inventory counters at the top of the screen.
    void render(Renderer& renderer, const Inventory& inventory) const;

private:
    void drawToolIcon(Renderer& renderer, ToolType type, int x, int y) const;
    void drawNumber(Renderer& renderer, int number, int x, int y, SDL_Color color) const;
};
