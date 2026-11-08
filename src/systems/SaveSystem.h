#pragma once

#include <string>

class Game;

// Handles serialization and deserialization of Sunny Hollow Farm game state to a documented JSON save file.
class SaveSystem {
public:
    static constexpr const char* DEFAULT_SAVE_FILE = "savegame.json";

    // Saves current game state (day, wallet, energy, inventory, crop field, player position/location) to JSON.
    static bool save(const Game& game, const std::string& filepath = DEFAULT_SAVE_FILE);

    // Loads and restores game state from the specified JSON file.
    static bool load(Game& game, const std::string& filepath = DEFAULT_SAVE_FILE);

    // Checks whether a save file exists at the given path.
    static bool exists(const std::string& filepath = DEFAULT_SAVE_FILE);

    // Removes the save file from disk.
    static bool removeSave(const std::string& filepath = DEFAULT_SAVE_FILE);
};
