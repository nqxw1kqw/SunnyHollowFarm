#include "core/Game.h"
#include <iostream>

Game::~Game() {
    cleanUp();
}

bool Game::init(const char* title, int width, int height, bool smokeTestMode) {
    m_smokeTestMode = smokeTestMode;

    // Initialize SDL video subsystem
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        std::cerr << "SDL_Init error: " << SDL_GetError() << std::endl;
        return false;
    }

    // Create game window (640x480 native 20x15 tiles)
    Uint32 windowFlags = SDL_WINDOW_SHOWN;
    m_window = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        windowFlags
    );

    if (!m_window) {
        std::cerr << "SDL_CreateWindow error: " << SDL_GetError() << std::endl;
        return false;
    }

    // Initialize renderer
    if (!m_renderer.init(m_window)) {
        std::cerr << "Renderer initialization failed." << std::endl;
        return false;
    }

    // Initialize asset manager
    if (!m_assets.init()) {
        std::cerr << "AssetManager initialization warning." << std::endl;
    }

    m_isRunning = true;
    std::cout << "Sunny Hollow Farm initialized successfully (" << width << "x" << height << ")" << std::endl;
    if (m_smokeTestMode) {
        std::cout << "[Smoke Test Mode active: running 120 fixed ticks then cleanly exiting]" << std::endl;
    }

    return true;
}

void Game::cleanUp() {
    m_assets.cleanUp();
    m_renderer.cleanUp();

    if (m_window) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }

    SDL_Quit();
}

void Game::run() {
    while (m_isRunning) {
        // Measure frame elapsed time and feed accumulator
        m_time.beginFrame();

        // 1. Process OS window events and key transitions
        m_input.pollEvents();
        if (m_input.quitRequested()) {
            m_isRunning = false;
        }

        handleInput();

        // 2. Fixed-timestep update ticks (steady 60 updates/second)
        while (m_time.consumeFixedStep()) {
            update(Time::FIXED_DT);
            m_input.clearFramePressed();

            if (m_smokeTestMode) {
                m_smokeTestFrames++;
                if (m_smokeTestFrames >= 120) {
                    std::cout << "[Smoke Test Completed: 120 ticks passed successfully]" << std::endl;
                    m_isRunning = false;
                    break;
                }
            }
        }

        // 3. Render the current game state
        render();

        // In dummy/headless mode, SDL_RenderPresent doesn't block on vsync,
        // so yield a millisecond to prevent spinning 100% CPU when testing.
        SDL_Delay(1);
    }
}

void Game::handleInput() {
    // Tool hotbar selection keys [1], [2], [3]
    if (m_input.isKeyPressed(SDL_SCANCODE_1)) {
        m_inventory.activeTool = ToolType::Hoe;
        std::cout << "[Tool Selected] Hoe" << std::endl;
    } else if (m_input.isKeyPressed(SDL_SCANCODE_2)) {
        m_inventory.activeTool = ToolType::WateringCan;
        std::cout << "[Tool Selected] Watering Can" << std::endl;
    } else if (m_input.isKeyPressed(SDL_SCANCODE_3)) {
        m_inventory.activeTool = ToolType::TomatoSeeds;
        std::cout << "[Tool Selected] Tomato Seeds (Stock: " << m_inventory.tomatoSeeds << ")" << std::endl;
    }

    // Use active tool on the targeted tile (Space, E, or Return)
    if (m_input.isKeyPressed(SDL_SCANCODE_SPACE) ||
        m_input.isKeyPressed(SDL_SCANCODE_E) ||
        m_input.isKeyPressed(SDL_SCANCODE_RETURN)) {
        auto [tx, ty] = m_player.getTargetTile();
        m_farmSystem.interact(tx, ty, m_map, m_inventory);
    }
}

void Game::update(float dt) {
    m_player.update(dt, m_input, m_map);
    m_farmSystem.update(dt, m_map);

    // In smoke test mode, simulate farming sequence to verify functionality automatically
    if (m_smokeTestMode) {
        auto [tx, ty] = m_player.getTargetTile();
        if (m_smokeTestFrames == 10) {
            // Till soil
            m_inventory.activeTool = ToolType::Hoe;
            m_farmSystem.interact(tx, ty, m_map, m_inventory);
        } else if (m_smokeTestFrames == 20) {
            // Water soil
            m_inventory.activeTool = ToolType::WateringCan;
            m_farmSystem.interact(tx, ty, m_map, m_inventory);
        } else if (m_smokeTestFrames == 30) {
            // Plant tomato seed
            m_inventory.activeTool = ToolType::TomatoSeeds;
            m_farmSystem.interact(tx, ty, m_map, m_inventory);
        } else if (m_smokeTestFrames >= 31 && m_smokeTestFrames <= 100) {
            // Simulate time passage so tomato reaches mature Harvest stage
            m_farmSystem.update(1.0f, m_map);
        } else if (m_smokeTestFrames == 105) {
            // Harvest mature tomato
            m_farmSystem.interact(tx, ty, m_map, m_inventory);
            if (m_inventory.harvestedTomatoes > 0) {
                std::cout << "[Smoke Test] Farming loop verified: tilled -> watered -> planted -> matured -> harvested!" << std::endl;
            }
        }
    }
}

void Game::render() {
    // Clear screen with a neutral dark tone
    m_renderer.clear(SDL_Color{20, 20, 20, 255});

    // 1. Render terrain tile map
    m_map.render(m_renderer);

    // 2. Render planted crops
    m_farmSystem.render(m_renderer);

    // 3. Render farmer player
    m_player.render(m_renderer);

    // 4. Render HUD (hotbar, seed count, harvested crop counter)
    m_hud.render(m_renderer, m_inventory);

    // Present final frame buffer
    m_renderer.present();
}
