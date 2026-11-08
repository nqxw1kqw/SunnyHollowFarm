#include "systems/TimeSystem.h"
#include <iostream>

bool TimeSystem::isBedTile(int tileX, int tileY) {
    // Farmhouse occupies x=1..3, y=1..2. Tile (2, 2) is the farmhouse entrance & bed.
    return (tileX >= 1 && tileX <= 3 && tileY >= 1 && tileY <= 2);
}

void TimeSystem::sleep(Player& player, FarmSystem& farmSystem, Map& map, HUD* hud) {
    m_day++;

    // 1. Fully restore player stamina
    int restoredAmount = player.getMaxEnergy() - player.getEnergy();
    player.setEnergy(player.getMaxEnergy());

    // 2. Overnight crop growth and soil drying
    farmSystem.advanceDay(map);

    std::cout << "[DayCycle] Zzz... Player slept overnight. Welcome to Day " << m_day
              << "! Restored " << restoredAmount << " energy (now "
              << player.getEnergy() << "/" << player.getMaxEnergy() << ")." << std::endl;

    // 3. On-screen HUD notification
    if (hud) {
        std::string msg = "DAY " + std::to_string(m_day) + " - FULLY RESTED!";
        hud->showFeedback(msg, 2.5f);
    }
}
