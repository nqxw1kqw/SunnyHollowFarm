#include "systems/TimeSystem.h"
#include <iostream>
#include <cstdio>
#include <cmath>

bool TimeSystem::isBedTile(int tileX, int tileY) {
    // Farmhouse occupies x=1..3, y=1..2. Tile (2, 2) is the farmhouse entrance & bed.
    return (tileX >= 1 && tileX <= 3 && tileY >= 1 && tileY <= 2);
}

void TimeSystem::update(float dt, bool isPaused, Player& player, FarmSystem& farmSystem, Map& map, HUD* hud) {
    if (isPaused) return;

    m_minuteTimer += dt;
    while (m_minuteTimer >= SECONDS_PER_TEN_MINUTES) {
        m_minuteTimer -= SECONDS_PER_TEN_MINUTES;
        m_timeOfDay += 10;
        if (m_timeOfDay % 100 >= 60) {
            m_timeOfDay = (m_timeOfDay / 100 + 1) * 100;
        }

        // Exhaustion check at 2:00 AM (Stardew Valley pass-out mechanic)
        if (m_timeOfDay >= PASS_OUT_TIME) {
            passOut(player, farmSystem, map, hud);
            break;
        }
    }
}

std::string TimeSystem::getTimeString() const {
    int rawHours = (m_timeOfDay / 100);
    int normHours = rawHours % 24;
    int displayHours = normHours % 12;
    if (displayHours == 0) displayHours = 12;
    bool isPM = (normHours >= 12);
    int mins = m_timeOfDay % 100;

    char buf[16];
    std::snprintf(buf, sizeof(buf), "%02d:%02d %s", displayHours, mins, isPM ? "PM" : "AM");
    return std::string(buf);
}

SDL_Color TimeSystem::getAmbientLighting(bool isOutdoors) const {
    if (!isOutdoors) {
        // Farmhouse Interior: Warm cozy lamplight
        return SDL_Color{255, 220, 150, 14};
    }

    if (m_timeOfDay < 800) {
        // Morning sunrise (06:00 - 08:00): Soft golden warmth fading into bright day
        float progress = std::clamp((800 - m_timeOfDay) / 200.0f, 0.0f, 1.0f);
        Uint8 alpha = static_cast<Uint8>(progress * 28.0f);
        return SDL_Color{255, 225, 170, alpha};
    } else if (m_timeOfDay < SUNSET_TIME) {
        // Midday (08:00 - 18:00): Clear sunlight, or cool slate overcast if raining
        if (m_weather == Weather::Rainy) {
            return SDL_Color{65, 90, 120, 32};
        }
        return SDL_Color{0, 0, 0, 0};
    } else if (m_timeOfDay < NIGHT_TIME) {
        // Sunset / Dusk (18:00 - 20:00): Rich amber golden hour
        float progress = std::clamp((m_timeOfDay - SUNSET_TIME) / 200.0f, 0.0f, 1.0f);
        Uint8 alpha = static_cast<Uint8>(progress * 62.0f);
        if (m_weather == Weather::Rainy) {
            return SDL_Color{160, 90, 110, static_cast<Uint8>(35 + alpha * 0.6f)};
        }
        return SDL_Color{240, 120, 35, alpha};
    } else {
        // Night (20:00 - 02:00): Deep starry indigo moonlight
        float progress = std::clamp((m_timeOfDay - NIGHT_TIME) / 200.0f, 0.0f, 1.0f);
        Uint8 alpha = static_cast<Uint8>(62.0f + progress * 55.0f);
        return SDL_Color{15, 20, 60, alpha};
    }
}

void TimeSystem::sleep(Player& player, FarmSystem& farmSystem, Map& map, HUD* hud) {
    m_day++;
    m_weather = calculateWeatherForDay(m_day);
    m_timeOfDay = START_TIME;
    m_minuteTimer = 0.0f;

    if (m_onDayAdvanceCallback) {
        m_onDayAdvanceCallback(m_day);
    }

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

void TimeSystem::passOut(Player& player, FarmSystem& farmSystem, Map& map, HUD* hud) {
    m_day++;
    m_weather = calculateWeatherForDay(m_day);
    m_timeOfDay = START_TIME;
    m_minuteTimer = 0.0f;

    if (m_onDayAdvanceCallback) {
        m_onDayAdvanceCallback(m_day);
    }

    // Penalized energy recovery on pass-out (50% max energy)
    player.setEnergy(player.getMaxEnergy() / 2);

    // Overnight crop growth
    farmSystem.advanceDay(map, m_weather);

    std::cout << "[DayCycle] 2:00 AM! Player passed out from exhaustion. Woke up in bed on Day "
              << m_day << ". Energy restored to " << player.getEnergy() << "/" << player.getMaxEnergy() << std::endl;

    if (hud) {
        hud->showFeedback("PASSED OUT AT 2:00 AM! WOKE UP TIRED.", 3.5f);
    }

    if (m_onPassOutCallback) {
        m_onPassOutCallback();
    }

    if (m_onSleepCallback) {
        m_onSleepCallback();
    }
}
