#pragma once

#include "core/Renderer.h"
#include "entities/Inventory.h"
#include "entities/Player.h"
#include "systems/Weather.h"
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

    // Hotbar layout constants (located in bottom region of screen)
    static constexpr int HOTBAR_SLOT_COUNT = 8;
    static constexpr int HOTBAR_SLOT_SIZE = 34;
    static constexpr int HOTBAR_SLOT_SPACING = 4;
    static constexpr int HOTBAR_TOTAL_WIDTH = HOTBAR_SLOT_COUNT * HOTBAR_SLOT_SIZE + (HOTBAR_SLOT_COUNT - 1) * HOTBAR_SLOT_SPACING; // 300
    static constexpr int HOTBAR_X = (Config::SCREEN_WIDTH - HOTBAR_TOTAL_WIDTH) / 2; // (bottom-center)
    static constexpr int HOTBAR_Y = Config::SCREEN_HEIGHT - 48; // Bottom region

    int getHotbarSlotAt(int mouseX, int mouseY) const {
        if (mouseY < HOTBAR_Y - 5 || mouseY > HOTBAR_Y + HOTBAR_SLOT_SIZE + 5) return -1;
        for (int i = 0; i < HOTBAR_SLOT_COUNT; ++i) {
            int slotX = HOTBAR_X + i * (HOTBAR_SLOT_SIZE + HOTBAR_SLOT_SPACING);
            if (mouseX >= slotX && mouseX <= slotX + HOTBAR_SLOT_SIZE) {
                return i;
            }
        }
        return -1;
    }

    // Renders all HUD components including clock, day counter, weather indicator, and bottom hotbar.
    void render(Renderer& renderer, const Inventory& inventory, const Player& player,
                int currentDay = 1, Weather weather = Weather::Sunny,
                const std::string& timeString = "06:00 AM", bool isNight = false) const;

    // Renders only the top feedback banner (if active)
    void renderFeedbackBanner(Renderer& renderer) const;

    // Checks if a feedback banner is currently visible.
    bool hasActiveFeedback() const { return m_feedbackTimer > 0.0f; }
    const std::string& getActiveFeedback() const { return m_feedbackMessage; }

private:
    void drawToolIcon(Renderer& renderer, ToolType type, int x, int y) const;
    void drawCoinIcon(Renderer& renderer, int x, int y) const;
    void drawEnergyIcon(Renderer& renderer, int x, int y) const;
    void drawSunIcon(Renderer& renderer, int x, int y) const;
    void drawRainIcon(Renderer& renderer, int x, int y) const;
    void drawMoonIcon(Renderer& renderer, int x, int y) const;

    std::string m_feedbackMessage;
    float m_feedbackTimer = 0.0f;
    float m_feedbackDuration = 2.0f;
};
