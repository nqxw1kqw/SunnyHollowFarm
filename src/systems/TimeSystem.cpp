#include "systems/TimeSystem.h"
#include <iostream>

bool TimeSystem::isBedTile(int tileX, int tileY) {
    // Farmhouse occupies x=1..3, y=1..2. Tile (2, 2) is the farmhouse entrance & bed.
    return (tileX >= 1 && tileX <= 3 && tileY >= 1 && tileY <= 2);
}

void TimeSystem::sleep(Player& player, FarmSystem& farmSystem, Map& map, HUD* hud) {
    m_day++;
    m_weather = calculateWeatherForDay(m_day);

    // 1. Fully restore player stamina
    int restoredAmount = player.getMaxEnergy() - player.getEnergy();
    player.setEnergy(player.getMaxEnergy());

    // 2. Overnight crop growth and soil drying/watering per weather
    farmSystem.advanceDay(map, m_weather);

    std::cout << "[DayCycle] Zzz... Player slept overnight. Welcome to Day " << m_day
              << "! Weather: " << weatherToString(m_weather)
              << ". Restored " << restoredAmount << " energy (now "
              << player.getEnergy() << "/" << player.getMaxEnergy() << ")." << std::endl;

    // 3. On-screen HUD notification
    if (hud) {
        std::string weatherNote = (m_weather == Weather::Rainy) ? " (RAINY)" : "";
        std::string msg = "DAY " + std::to_string(m_day) + weatherNote + " - FULLY RESTED!";
        hud->showFeedback(msg, 2.5f);
    }

    // 4. Invoke auto-save whenever the player sleeps
    if (m_onSleepCallback) {
        m_onSleepCallback();
    }
}
