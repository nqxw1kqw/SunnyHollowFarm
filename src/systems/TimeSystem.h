#pragma once

#include "entities/Player.h"
#include "systems/FarmSystem.h"
#include "world/Map.h"
#include "ui/HUD.h"
#include "systems/Weather.h"
#include <string>
#include <functional>

// Manages the in-game calendar day counter, sleeping mechanics, and overnight simulation.
class TimeSystem {
public:
    TimeSystem() {
        m_weather = calculateWeatherForDay(m_day);
    }

    int getDay() const { return m_day; }
    void setDay(int day, bool updateWeather = true) {
        m_day = std::max(1, day);
        if (updateWeather) {
            m_weather = calculateWeatherForDay(m_day);
        }
    }

    Weather getWeather() const { return m_weather; }
    void setWeather(Weather weather) { m_weather = weather; }

    void setOnSleepCallback(std::function<void()> callback) { m_onSleepCallback = callback; }

    // Advances the day cycle through sleep:
    // 1. Day counter increments.
    // 2. Weather rolls deterministically from the new day number.
    // 3. Player energy fully restores to max stamina.
    // 4. Crops advance overnight growth stage.
    // 5. Watered soil dries back to tilled soil, or is watered if new day is Rainy.
    // 6. Displays clear on-screen HUD feedback.
    void sleep(Player& player, FarmSystem& farmSystem, Map& map, HUD* hud = nullptr);

    // Checks if a tile corresponds to the Farmhouse bed / rest area.
    static bool isBedTile(int tileX, int tileY);

private:
    int m_day = 1;
    Weather m_weather = Weather::Sunny;
    std::function<void()> m_onSleepCallback = nullptr;
};
