#pragma once

#include "entities/Player.h"
#include "systems/FarmSystem.h"
#include "world/Map.h"
#include "ui/HUD.h"
#include "systems/Weather.h"
#include <string>
#include <functional>
#include <algorithm>
#include <SDL.h>

// Manages the in-game calendar day counter, clock time of day (Stardew Valley 6:00 AM - 2:00 AM system),
// sleeping mechanics, nightfall/dusk ambient lighting, and overnight simulation.
class TimeSystem {
public:
    static constexpr int START_TIME = 600;       // 06:00 AM morning start
    static constexpr int NOON_TIME = 1200;       // 12:00 PM noon
    static constexpr int SUNSET_TIME = 1800;     // 06:00 PM sunset start
    static constexpr int NIGHT_TIME = 2000;      // 08:00 PM nightfall
    static constexpr int PASS_OUT_TIME = 2600;   // 02:00 AM exhaustion limit

    // Stardew Valley standard: 700ms per in-game minute = 7.0s per 10 in-game minutes
    static constexpr float SECONDS_PER_TEN_MINUTES = 7.0f;

    TimeSystem() {
        m_weather = calculateWeatherForDay(m_day);
        m_timeOfDay = START_TIME;
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

    int getTimeOfDay() const { return m_timeOfDay; }
    void setTimeOfDay(int time) {
        m_timeOfDay = std::clamp(time, START_TIME, PASS_OUT_TIME);
        m_minuteTimer = 0.0f;
    }

    // Updates live in-game clock time during gameplay (paused during menus/dialogue)
    void update(float dt, bool isPaused, Player& player, FarmSystem& farmSystem, Map& map, HUD* hud = nullptr);

    // Formatted time string, e.g. "06:00 AM", "12:10 PM", "08:30 PM", "01:50 AM"
    std::string getTimeString() const;

    bool isNight() const { return m_timeOfDay >= NIGHT_TIME; }
    bool isSunset() const { return m_timeOfDay >= SUNSET_TIME && m_timeOfDay < NIGHT_TIME; }

    // Calculates ambient world lighting tint overlay (Stardew Valley ambient lighting model)
    SDL_Color getAmbientLighting(bool isOutdoors) const;

    void setOnSleepCallback(std::function<void()> callback) { m_onSleepCallback = callback; }
    void setOnDayAdvanceCallback(std::function<void(int)> callback) { m_onDayAdvanceCallback = callback; }
    void setOnPassOutCallback(std::function<void()> callback) { m_onPassOutCallback = callback; }

    // Advances the day cycle through sleep:
    // 1. Day counter increments.
    // 2. Weather rolls deterministically from the new day number.
    // 3. Player energy fully restores to max stamina.
    // 4. Crops advance overnight growth stage.
    // 5. Watered soil dries back to tilled soil, or is watered if new day is Rainy.
    // 6. Clock time resets back to 06:00 AM.
    // 7. Displays clear on-screen HUD feedback.
    void sleep(Player& player, FarmSystem& farmSystem, Map& map, HUD* hud = nullptr);

    // Exhaustion pass-out when player stays awake past 2:00 AM (02:00)
    void passOut(Player& player, FarmSystem& farmSystem, Map& map, HUD* hud = nullptr);

    // Checks if a tile corresponds to the Farmhouse bed / rest area.
    static bool isBedTile(int tileX, int tileY);

private:
    int m_day = 1;
    int m_timeOfDay = START_TIME;
    float m_minuteTimer = 0.0f;
    Weather m_weather = Weather::Sunny;

    std::function<void()> m_onSleepCallback = nullptr;
    std::function<void(int)> m_onDayAdvanceCallback = nullptr;
    std::function<void()> m_onPassOutCallback = nullptr;
};
