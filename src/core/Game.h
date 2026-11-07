#pragma once

#include <SDL.h>
#include "core/Time.h"
#include "core/Input.h"
#include "core/Renderer.h"
#include "core/AssetManager.h"
#include "world/Map.h"
#include "entities/Player.h"
#include "entities/Inventory.h"
#include "systems/FarmSystem.h"
#include "ui/HUD.h"

// Top-level application class orchestrating game setup, fixed-timestep loop, and teardown.
class Game {
public:
    Game() = default;
    ~Game();

    // Initializes SDL, window, renderer, and subsystems.
    bool init(const char* title, int width, int height, bool smokeTestMode = false);

    // Runs the main fixed-timestep game loop until quit is requested.
    void run();

    // Cleans up resources on shutdown.
    void cleanUp();

    bool isRunning() const { return m_isRunning; }
    const Inventory& getInventory() const { return m_inventory; }

private:
    void handleInput();
    void update(float dt);
    void render();

    SDL_Window* m_window = nullptr;
    Renderer m_renderer;
    AssetManager m_assets;
    Time m_time;
    Input m_input;

    Map m_map;
    Player m_player;
    Inventory m_inventory;
    FarmSystem m_farmSystem;
    HUD m_hud;

    bool m_isRunning = false;
    bool m_smokeTestMode = false;
    int m_smokeTestFrames = 0;
};
