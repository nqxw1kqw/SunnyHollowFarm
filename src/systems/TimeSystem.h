#pragma once

#include "entities/Player.h"
#include "systems/FarmSystem.h"
#include "world/Map.h"
#include "ui/HUD.h"
#include <string>

// Manages the in-game calendar day counter, sleeping mechanics, and overnight simulation.
class TimeSystem {
public:
    TimeSystem() = default;

    int getDay() const { return m_day; }
    void setDay(int day) { m_day = std::max(1, day); }

    // Advances the day cycle through sleep:
    // 1. Day counter increments.
    // 2. Player energy fully restores to max stamina.
    // 3. Crops advance overnight growth stage.
    // 4. Watered soil dries back to tilled soil under the overnight drying rule.
    // 5. Displays clear on-screen HUD feedback.
    void sleep(Player& player, FarmSystem& farmSystem, Map& map, HUD* hud = nullptr);

    // Checks if a tile corresponds to the Farmhouse bed / rest area.
    static bool isBedTile(int tileX, int tileY);

private:
    int m_day = 1;
};
