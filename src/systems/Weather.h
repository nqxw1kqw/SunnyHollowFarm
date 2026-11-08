#pragma once

#include <string>
#include <cstdint>

// Weather conditions in Sunny Hollow Farm.
enum class Weather {
    Sunny,
    Rainy
};

inline const char* weatherToString(Weather weather) {
    switch (weather) {
        case Weather::Sunny: return "Sunny";
        case Weather::Rainy: return "Rainy";
    }
    return "Sunny";
}

inline Weather stringToWeather(const std::string& str) {
    if (str == "Rainy") return Weather::Rainy;
    return Weather::Sunny;
}

// Deterministic seeded weather formula from calendar day number.
// Seed 73 yields ~31% rainy days (roughly 1 day in 3, Sunny most days).
// Specifically: Days 1-5 are Sunny, Day 6 is Rainy, Day 7 is Sunny, Day 8 is Rainy.
inline Weather calculateWeatherForDay(int day) {
    uint32_t h = (static_cast<uint32_t>(day) + 73U) & 0xffffffff;
    h = (((h >> 16) ^ h) * 0x45d9f3b) & 0xffffffff;
    h = (((h >> 16) ^ h) * 0x45d9f3b) & 0xffffffff;
    h = ((h >> 16) ^ h) & 0xffffffff;
    return (h % 3 == 0) ? Weather::Rainy : Weather::Sunny;
}
