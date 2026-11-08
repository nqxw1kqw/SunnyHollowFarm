#pragma once

#include "core/Renderer.h"
#include "entities/Inventory.h"
#include "entities/Player.h"
#include <string>

// Heads-Up Display showing the active tool hotbar, money wallet, energy bar,
// inventory toggle hint, harvested produce counter, and feedback banners.
class HUD {
public:
    HUD() = default;

    // Updates feedback notification timers.
    void update(float dt);

    // Displays an on-screen warning or notification banner.
    void showFeedback(const std::string& message, float duration = 2.0f);

    // Renders all HUD components including day counter.
    void render(Renderer& renderer, const Inventory& inventory, const Player& player, int currentDay = 1) const;

    // Checks if a feedback banner is currently visible.
    bool hasActiveFeedback() const { return m_feedbackTimer > 0.0f; }
    const std::string& getActiveFeedback() const { return m_feedbackMessage; }

private:
    void drawToolIcon(Renderer& renderer, ToolType type, int x, int y) const;
    void drawCoinIcon(Renderer& renderer, int x, int y) const;
    void drawEnergyIcon(Renderer& renderer, int x, int y) const;
    void drawSunIcon(Renderer& renderer, int x, int y) const;

    std::string m_feedbackMessage;
    float m_feedbackTimer = 0.0f;
    float m_feedbackDuration = 2.0f;
};
