#include "core/Game.h"
#include <iostream>
#include <cassert>

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

    // Initialize cooking recipes from assets/data/recipes.txt
    m_cookingSystem.loadRecipes("assets/data/recipes.txt");

    m_isRunning = true;
    std::cout << "Sunny Hollow Farm initialized successfully (" << width << "x" << height << ")" << std::endl;
    if (m_smokeTestMode) {
        std::cout << "[Smoke Test Mode active: running test sequence then cleanly exiting]" << std::endl;
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
                if (m_smokeTestFrames >= 135) {
                    std::cout << "[Smoke Test Completed: all ticks passed successfully]" << std::endl;
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
    // Inventory toggle key (Tab or I)
    if (m_input.isKeyPressed(SDL_SCANCODE_TAB) || m_input.isKeyPressed(SDL_SCANCODE_I)) {
        m_inventoryUI.toggle();
        std::cout << "[UI] Inventory modal " << (m_inventoryUI.isOpen() ? "OPENED" : "CLOSED") << std::endl;
    }

    // When the inventory grid is open, pass input to the modal and pause world interaction
    if (m_inventoryUI.isOpen()) {
        m_inventoryUI.handleInput(m_input, m_inventory);
        return;
    }

    // When the cooking UI modal is open, pass input to it and pause world interaction
    if (m_cookingUI.isOpen()) {
        m_cookingUI.handleInput(m_input, m_inventory, m_cookingSystem, &m_hud);
        return;
    }

    // Tool hotbar selection keys [1], [2], [3], [4]
    if (m_input.isKeyPressed(SDL_SCANCODE_1)) {
        m_inventory.activeTool = ToolType::Hoe;
        std::cout << "[Tool Selected] Hoe" << std::endl;
    } else if (m_input.isKeyPressed(SDL_SCANCODE_2)) {
        m_inventory.activeTool = ToolType::WateringCan;
        std::cout << "[Tool Selected] Watering Can" << std::endl;
    } else if (m_input.isKeyPressed(SDL_SCANCODE_3)) {
        m_inventory.activeTool = ToolType::TomatoSeeds;
        std::cout << "[Tool Selected] Tomato Seeds (Stock: "
                  << m_inventory.getItemCount(ItemType::TomatoSeeds) << ")" << std::endl;
    } else if (m_input.isKeyPressed(SDL_SCANCODE_4)) {
        m_inventory.activeTool = ToolType::WheatSeeds;
        std::cout << "[Tool Selected] Wheat Seeds (Stock: "
                  << m_inventory.getItemCount(ItemType::WheatSeeds) << ")" << std::endl;
    }

    // Action key (Space, E, Return, or C)
    if (m_input.isKeyPressed(SDL_SCANCODE_SPACE) ||
        m_input.isKeyPressed(SDL_SCANCODE_E) ||
        m_input.isKeyPressed(SDL_SCANCODE_RETURN) ||
        m_input.isKeyPressed(SDL_SCANCODE_C)) {
        auto [tx, ty] = m_player.getTargetTile();

        // 1. Stove kitchen interaction point check
        if (m_map.inBounds(tx, ty) && m_map.getTile(tx, ty).getType() == TileType::Stove) {
            m_cookingUI.open();
            std::cout << "[Stove] Opened cooking interface at (" << tx << ", " << ty << ")." << std::endl;
            return;
        }

        // 2. Farming interaction
        m_farmSystem.interact(tx, ty, m_map, m_inventory, m_player, &m_hud);
    }
}

void Game::update(float dt) {
    // Update on-screen feedback timer
    m_hud.update(dt);

    // Player movements and world physics only progress if modals are closed
    if (!m_inventoryUI.isOpen() && !m_cookingUI.isOpen()) {
        m_player.update(dt, m_input, m_map);
    }
    m_farmSystem.update(dt, m_map);

    // Headless smoke test verification sequence
    if (m_smokeTestMode) {
        auto [tx, ty] = m_player.getTargetTile();

        if (m_smokeTestFrames == 10) {
            // Verify initial state
            assert(m_player.getEnergy() == Config::INITIAL_ENERGY);
            assert(m_inventory.getMoney() == Config::INITIAL_MONEY);
            assert(m_inventory.getItemCount(ItemType::TomatoSeeds) == Config::INITIAL_TOMATO_SEEDS);
            assert(m_inventory.getItemCount(ItemType::Tomato) == 0);

            // Till soil
            m_inventory.activeTool = ToolType::Hoe;
            bool ok = m_farmSystem.interact(tx, ty, m_map, m_inventory, m_player, &m_hud);
            assert(ok && "Tilling should succeed");
            assert(m_player.getEnergy() == Config::INITIAL_ENERGY - Config::ENERGY_COST_TILL);
            std::cout << "[Smoke Test] Tilled: energy decreased to " << m_player.getEnergy() << std::endl;
        } else if (m_smokeTestFrames == 20) {
            // Water soil
            int energyBefore = m_player.getEnergy();
            m_inventory.activeTool = ToolType::WateringCan;
            bool ok = m_farmSystem.interact(tx, ty, m_map, m_inventory, m_player, &m_hud);
            assert(ok && "Watering should succeed");
            assert(m_player.getEnergy() == energyBefore - Config::ENERGY_COST_WATER);
            std::cout << "[Smoke Test] Watered: energy decreased to " << m_player.getEnergy() << std::endl;
        } else if (m_smokeTestFrames == 30) {
            // Plant tomato seed
            int energyBefore = m_player.getEnergy();
            int seedsBefore = m_inventory.getItemCount(ItemType::TomatoSeeds);
            m_inventory.activeTool = ToolType::TomatoSeeds;
            bool ok = m_farmSystem.interact(tx, ty, m_map, m_inventory, m_player, &m_hud);
            assert(ok && "Planting should succeed");
            assert(m_player.getEnergy() == energyBefore - Config::ENERGY_COST_PLANT);
            assert(m_inventory.getItemCount(ItemType::TomatoSeeds) == seedsBefore - 1);
            std::cout << "[Smoke Test] Planted: seeds decreased from " << seedsBefore
                      << " to " << m_inventory.getItemCount(ItemType::TomatoSeeds)
                      << ", energy to " << m_player.getEnergy() << std::endl;
        } else if (m_smokeTestFrames >= 31 && m_smokeTestFrames <= 100) {
            // Accelerate time to grow crop to maturity
            m_farmSystem.update(1.0f, m_map);
        } else if (m_smokeTestFrames == 105) {
            // Harvest mature tomato
            int energyBefore = m_player.getEnergy();
            int tomatoesBefore = m_inventory.getItemCount(ItemType::Tomato);
            bool ok = m_farmSystem.interact(tx, ty, m_map, m_inventory, m_player, &m_hud);
            assert(ok && "Harvesting mature crop should succeed");
            assert(m_player.getEnergy() == energyBefore - Config::ENERGY_COST_HARVEST);
            assert(m_inventory.getItemCount(ItemType::Tomato) == tomatoesBefore + 1);
            std::cout << "[Smoke Test] Harvested: tomatoes increased from " << tomatoesBefore
                      << " to " << m_inventory.getItemCount(ItemType::Tomato)
                      << ", energy to " << m_player.getEnergy() << std::endl;
            std::cout << "[Smoke Test] Farming loop verified: tilled -> watered -> planted -> matured -> harvested!" << std::endl;
        } else if (m_smokeTestFrames == 110) {
            // Verify action refusal when energy is insufficient
            m_player.setEnergy(1); // Set energy below any action cost (cost is 5 for till)
            int energyBefore = m_player.getEnergy();
            m_inventory.activeTool = ToolType::Hoe;
            // Attempt to till adjacent tile (tx + 1, ty)
            bool ok = m_farmSystem.interact(tx + 1, ty, m_map, m_inventory, m_player, &m_hud);
            assert(!ok && "Action must be refused when energy is too low");
            assert(m_player.getEnergy() == energyBefore && "Energy must not change or go negative when refused");
            assert(m_hud.hasActiveFeedback() && "On-screen feedback must be active when energy refused");
            assert(m_map.getTile(tx + 1, ty).getType() != TileType::TilledSoil && "Tile must remain untilled");
            std::cout << "[Smoke Test] Energy exhaustion verified: action refused, energy remained "
                      << m_player.getEnergy() << " (never negative), feedback banner triggered!" << std::endl;

            // Restore energy
            m_player.setEnergy(Config::INITIAL_ENERGY);
        } else if (m_smokeTestFrames == 115) {
            // Verify inventory UI toggle
            assert(!m_inventoryUI.isOpen());
            m_inventoryUI.toggle();
            assert(m_inventoryUI.isOpen() && "Inventory UI should be open after toggle");
            m_inventoryUI.toggle();
            assert(!m_inventoryUI.isOpen() && "Inventory UI should be closed after second toggle");
            std::cout << "[Smoke Test] Inventory UI toggle verified!" << std::endl;
        } else if (m_smokeTestFrames == 120) {
            // Milestone 4: Verify recipe parsing and missing ingredients refusal
            assert(m_cookingSystem.getRecipeCount() >= 2 && "At least Salad and Bread recipes must be loaded");
            assert(m_cookingSystem.findRecipeByDish(ItemType::Salad) != nullptr);
            assert(m_cookingSystem.findRecipeByDish(ItemType::Bread) != nullptr);

            // Attempt to cook Salad when only 1 tomato is available (Salad recipe requires 2)
            assert(m_inventory.getItemCount(ItemType::Tomato) == 1);
            assert(m_inventory.getItemCount(ItemType::Salad) == 0);
            bool cookFail = m_cookingSystem.cook(ItemType::Salad, m_inventory, &m_hud);
            assert(!cookFail && "Cooking Salad must fail when ingredients are missing");
            assert(m_inventory.getItemCount(ItemType::Tomato) == 1 && "Ingredients must not be consumed on failure");
            assert(m_inventory.getItemCount(ItemType::Salad) == 0);
            assert(m_hud.hasActiveFeedback() && "HUD must show missing ingredient feedback");
            std::cout << "[Smoke Test] Missing ingredients refusal verified: Salad requires 2 tomatoes (have 1) -> refused with feedback!" << std::endl;

            // Fulfill recipe with 2nd tomato and cook
            m_inventory.addItem(ItemType::Tomato, 1);
            assert(m_inventory.getItemCount(ItemType::Tomato) == 2);
            bool cookOk = m_cookingSystem.cook(ItemType::Salad, m_inventory, &m_hud);
            assert(cookOk && "Cooking Salad must succeed with required ingredients");
            assert(m_inventory.getItemCount(ItemType::Tomato) == 0 && "Tomatoes must be consumed");
            assert(m_inventory.getItemCount(ItemType::Salad) == 1 && "Salad dish must be added to inventory");
            std::cout << "[Smoke Test] Cooking Salad verified: 2x Tomato consumed -> 1x Salad added to inventory!" << std::endl;
        } else if (m_smokeTestFrames == 125) {
            // Milestone 4: Verify Bread cooking and stove interaction point
            assert(m_inventory.getItemCount(ItemType::Wheat) == 0);
            bool breadFail = m_cookingSystem.cook(ItemType::Bread, m_inventory, &m_hud);
            assert(!breadFail && "Cooking Bread must fail with 0 wheat");
            assert(m_hud.hasActiveFeedback());

            // Add 3 wheat and cook Bread
            m_inventory.addItem(ItemType::Wheat, 3);
            bool breadOk = m_cookingSystem.cook(ItemType::Bread, m_inventory, &m_hud);
            assert(breadOk && "Cooking Bread must succeed with 3 wheat");
            assert(m_inventory.getItemCount(ItemType::Wheat) == 0 && "Wheat must be consumed");
            assert(m_inventory.getItemCount(ItemType::Bread) == 1 && "Bread must be added to inventory");
            std::cout << "[Smoke Test] Cooking Bread verified: 3x Wheat consumed -> 1x Bread added to inventory!" << std::endl;

            // Verify kitchen stove interaction point on map and CookingUI toggle
            assert(m_map.getTile(4, 2).getType() == TileType::Stove && "Tile (4, 2) must be the kitchen Stove");
            assert(!m_cookingUI.isOpen());
            m_cookingUI.open();
            assert(m_cookingUI.isOpen() && "CookingUI should open");
            m_cookingUI.close();
            assert(!m_cookingUI.isOpen() && "CookingUI should close");
            std::cout << "[Smoke Test] Stove interaction tile & CookingUI toggle verified!" << std::endl;
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

    // 4. Render HUD (hotbar, money counter, energy bar, inventory hint, tomato counter, feedback banner)
    m_hud.render(m_renderer, m_inventory, m_player);

    // 5. Render Inventory Grid modal when open
    if (m_inventoryUI.isOpen()) {
        m_inventoryUI.render(m_renderer, m_inventory);
    }

    // 6. Render Cooking UI modal when open
    if (m_cookingUI.isOpen()) {
        m_cookingUI.render(m_renderer, m_inventory, m_cookingSystem);
    }

    // Present final frame buffer
    m_renderer.present();
}
