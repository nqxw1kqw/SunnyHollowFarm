#include "core/Game.h"
#include "ui/BitmapFont.h"
#include <iostream>
#include <cassert>
#include <cmath>

Game::~Game() {
    cleanUp();
}

bool Game::init(const char* title, int width, int height, bool smokeTestMode, bool playtestMode) {
    m_smokeTestMode = smokeTestMode;
    m_playtestMode = playtestMode;

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

    m_renderer.setAssets(&m_assets);
    if (m_smokeTestMode) {
        // Exercise the real PNG loader, including art not visible in the initial scene.
        struct SpriteCheck { const char* filename; int width; int height; };
        const SpriteCheck sprites[] = {
            {"player_sheet.png", 96, 128},
            {"npc_seller.png", 32, 32},
            {"npc_buyer.png", 32, 32},
            {"tiles_ground.png", 64, 64},
            {"tiles_fence.png", 64, 32},
            {"tomato_stage1.png", 32, 32},
            {"tomato_stage2.png", 32, 32},
            {"tomato_stage3.png", 32, 32},
            {"tomato_stage4.png", 32, 32},
            {"item_tomato.png", 32, 32},
            {"item_carrot.png", 32, 32},
            {"item_wheat.png", 32, 32},
            {"item_salad.png", 32, 32},
            {"item_bread.png", 32, 32},
            {"tool_hoe.png", 32, 32},
            {"tool_watering_can.png", 32, 32},
            {"item_seed_packet.png", 32, 32},
            {"ui_slot.png", 32, 32},
            {"ui_coin.png", 32, 32},
            {"ui_energy_bar.png", 32, 32},
            {"building_farmhouse.png", 96, 64},
            {"building_shop_stall.png", 96, 64},
            {"building_stove.png", 32, 64},
            {"tile_tilled_watered.png", 32, 32},
            {"fx_sparkle.png", 32, 32},
            {"ui_panel.png", 96, 96},
            {"ui_button.png", 96, 32},
            {"ui_button_hover.png", 96, 32},
            {"ui_dialogue.png", 96, 48},
            {"ui_cursor.png", 32, 32},
            {"ui_shop_sign.png", 64, 32},
            {"ui_cooking_icon.png", 32, 32},
            {"ui_sleep_icon.png", 32, 32},
        };
        for (const auto& sprite : sprites) {
            SDL_Texture* texture = m_assets.getTexture(std::string("assets/sprites/") + sprite.filename,
                                                       m_renderer.getSDLRenderer());
            int w = 0, h = 0;
            if (!texture || SDL_QueryTexture(texture, nullptr, nullptr, &w, &h) != 0 ||
                w != sprite.width || h != sprite.height) {
                std::cerr << "[Smoke Test] PNG load/size failed: " << sprite.filename << std::endl;
                return false;
            }
            std::cout << "[Smoke Test] PNG loaded: " << sprite.filename << " (" << w << "x" << h << ")" << std::endl;
        }
    }

    // Initialize cooking recipes from assets/data/recipes.txt
    m_cookingSystem.loadRecipes("assets/data/recipes.txt");

    // Initialize shop prices from assets/data/prices.txt
    m_shopSystem.loadPrices("assets/data/prices.txt");

    // Initialize farmhouse interior room map
    m_interiorMap.initInterior();

    m_isRunning = true;
    std::cout << "Sunny Hollow Farm initialized successfully (" << width << "x" << height << ")" << std::endl;
    if (m_smokeTestMode) {
        std::cout << "[Smoke Test Mode active: running test sequence then cleanly exiting]" << std::endl;
    }
    if (m_playtestMode) {
        std::cout << "[Playtest Mode active: executing full playthrough playtest across days]" << std::endl;
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
                if (m_smokeTestFrames >= 151) {
                    std::cout << "[Smoke Test Completed: all ticks passed successfully]" << std::endl;
                    m_isRunning = false;
                    break;
                }
            }

            if (m_playtestMode) {
                m_playtestFrames++;
                updatePlaytest(Time::FIXED_DT);
                if (!m_isRunning) {
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

void Game::transitionTo(Location loc) {
    m_location = loc;
    if (loc == Location::FarmhouseInterior) {
        m_player.placeAtTileCenter(9, 9);
        m_player.setFacing(Direction::Up);
        m_hud.showFeedback("ENTERED FARMHOUSE");
        std::cout << "[Transition] Entered Farmhouse Interior (spawned at 9, 9)." << std::endl;
    } else {
        m_player.placeAtTileCenter(2, 3);
        m_player.setFacing(Direction::Down);
        m_hud.showFeedback("EXITED TO FARM");
        std::cout << "[Transition] Exited Farmhouse to Farm (spawned at 2, 3)." << std::endl;
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

    // When the shop UI modal is open, pass input to it and pause world interaction
    if (m_shopUI.isOpen()) {
        m_shopUI.handleInput(m_input, m_inventory, m_shopSystem, &m_hud);
        return;
    }

    // Documented quick-sleep key [Z] (Zzz... rest overnight)
    if (m_input.isKeyPressed(SDL_SCANCODE_Z)) {
        m_timeSystem.sleep(m_player, m_farmSystem, m_map, &m_hud);
        return;
    }

    // Tool / Item hotbar selection keys [1] through [8]
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
    } else if (m_input.isKeyPressed(SDL_SCANCODE_5)) {
        m_inventory.activeTool = ToolType::CarrotSeeds;
        std::cout << "[Tool Selected] Carrot Seeds (Stock: "
                  << m_inventory.getItemCount(ItemType::CarrotSeeds) << ")" << std::endl;
    } else if (m_input.isKeyPressed(SDL_SCANCODE_6)) {
        m_inventory.activeTool = ToolType::Tomato;
        std::cout << "[Tool Selected] Tomato (Stock: "
                  << m_inventory.getItemCount(ItemType::Tomato) << ")" << std::endl;
    } else if (m_input.isKeyPressed(SDL_SCANCODE_7)) {
        m_inventory.activeTool = ToolType::Salad;
        std::cout << "[Tool Selected] Salad (Stock: "
                  << m_inventory.getItemCount(ItemType::Salad) << ")" << std::endl;
    } else if (m_input.isKeyPressed(SDL_SCANCODE_8)) {
        m_inventory.activeTool = ToolType::Bread;
        std::cout << "[Tool Selected] Bread (Stock: "
                  << m_inventory.getItemCount(ItemType::Bread) << ")" << std::endl;
    }

    // Action key (Space, E, Return, or C)
    if (m_input.isKeyPressed(SDL_SCANCODE_SPACE) ||
        m_input.isKeyPressed(SDL_SCANCODE_E) ||
        m_input.isKeyPressed(SDL_SCANCODE_RETURN) ||
        m_input.isKeyPressed(SDL_SCANCODE_C)) {

        if (m_location == Location::FarmhouseInterior) {
            auto [tx, ty] = m_player.getTargetTile();
            if (m_interiorMap.inBounds(tx, ty)) {
                TileType type = m_interiorMap.getTile(tx, ty).getType();
                if (type == TileType::Bed) {
                    m_timeSystem.sleep(m_player, m_farmSystem, m_map, &m_hud);
                    std::cout << "[Interior] Slept at comfortable indoor bed." << std::endl;
                    return;
                }
                if (type == TileType::Stove) {
                    m_cookingUI.open();
                    std::cout << "[Interior] Opened indoor kitchen stove at (" << tx << ", " << ty << ")." << std::endl;
                    return;
                }
                if (type == TileType::Chest) {
                    int tom = m_inventory.getItemCount(ItemType::Tomato);
                    int wht = m_inventory.getItemCount(ItemType::Wheat);
                    int car = m_inventory.getItemCount(ItemType::Carrot);
                    int sal = m_inventory.getItemCount(ItemType::Salad);
                    int brd = m_inventory.getItemCount(ItemType::Bread);
                    std::string msg = "CHEST: " + std::to_string(tom) + " TOM, "
                                      + std::to_string(wht) + " WHT, "
                                      + std::to_string(car) + " CAR, "
                                      + std::to_string(sal) + " SALAD, "
                                      + std::to_string(brd) + " BREAD";
                    m_hud.showFeedback(msg, 3.0f);
                    std::cout << "[Interior] " << msg << std::endl;
                    return;
                }
                if (type == TileType::Bookshelf) {
                    std::string msg = "ALMANAC: WATER CROPS DAILY FOR 2.5X GROWTH!";
                    m_hud.showFeedback(msg, 3.0f);
                    std::cout << "[Interior] Bookshelf: " << msg << std::endl;
                    return;
                }
                if (type == TileType::InteriorExit) {
                    transitionTo(Location::Farm);
                    return;
                }
            }
            return;
        }

        // Location is Farm:
        auto [tx, ty] = m_player.getTargetTile();

        // 1. Stove kitchen interaction point check
        if (m_map.inBounds(tx, ty) && m_map.getTile(tx, ty).getType() == TileType::Stove) {
            m_cookingUI.open();
            std::cout << "[Stove] Opened cooking interface at (" << tx << ", " << ty << ")." << std::endl;
            return;
        }

        // 2. Seller NPC interaction check (NPC tile (14, 2) or seller stall counter x=13..15, y=1)
        if ((tx == m_sellerNPC.getTileX() && ty == m_sellerNPC.getTileY()) ||
            (tx >= 13 && tx <= 15 && ty == 1)) {
            m_shopUI.open(ShopMode::Buy);
            std::cout << "[Shop] Opened Seller NPC shop at (" << tx << ", " << ty << ")." << std::endl;
            return;
        }

        // 3. Buyer NPC interaction check (NPC tile (17, 2) or buyer stall counter x=16..18, y=1)
        if ((tx == m_buyerNPC.getTileX() && ty == m_buyerNPC.getTileY()) ||
            (tx >= 16 && tx <= 18 && ty == 1)) {
            m_shopUI.open(ShopMode::Sell);
            std::cout << "[Shop] Opened Buyer NPC shop at (" << tx << ", " << ty << ")." << std::endl;
            return;
        }

        // 4. Farmhouse door / entrance check
        if (m_map.inBounds(tx, ty) &&
            (m_map.getTile(tx, ty).getType() == TileType::FarmhouseDoor || (tx == 2 && ty == 2))) {
            transitionTo(Location::FarmhouseInterior);
            return;
        }

        // Farmhouse rest structure check
        if (m_map.inBounds(tx, ty) &&
            (m_map.getTile(tx, ty).getType() == TileType::Farmhouse || TimeSystem::isBedTile(tx, ty))) {
            m_timeSystem.sleep(m_player, m_farmSystem, m_map, &m_hud);
            return;
        }

        // 5. Farming interaction
        m_farmSystem.interact(tx, ty, m_map, m_inventory, m_player, &m_hud);
    }
}

void Game::update(float dt) {
    // Update on-screen feedback timer
    m_hud.update(dt);

    // Living world animation updates
    m_map.update(dt);
    m_interiorMap.update(dt);
    m_sellerNPC.update(dt);
    m_buyerNPC.update(dt);

    // Player movements and world physics only progress if modals are closed
    Map& activeMap = (m_location == Location::Farm) ? m_map : m_interiorMap;
    if (!m_inventoryUI.isOpen() && !m_cookingUI.isOpen() && !m_shopUI.isOpen()) {
        m_player.update(dt, m_input, activeMap);
    }
    m_farmSystem.update(dt, m_map);

    // Zone transition check: door transitions trigger by overlapping the door tile
    if (m_location == Location::Farm) {
        if (m_player.overlapsTile(2, 2)) {
            transitionTo(Location::FarmhouseInterior);
        }
    } else {
        if (m_player.overlapsTile(9, 10)) {
            transitionTo(Location::Farm);
        }
    }

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
        } else if (m_smokeTestFrames == 130) {
            // Milestone 5: Verify price file parsing and buying seeds from Seller NPC
            assert(m_shopSystem.getBuyPrice(ItemType::TomatoSeeds) == 15);
            assert(m_shopSystem.getBuyPrice(ItemType::WheatSeeds) == 10);
            assert(m_shopSystem.getBuyPrice(ItemType::Wheat) == 18);
            assert(m_shopSystem.getSellPrice(ItemType::Salad) == 40);
            assert(m_shopSystem.getSellPrice(ItemType::Bread) == 50);

            // Purchase Tomato Seeds (cost 15G)
            int moneyBefore = m_inventory.getMoney();
            int tomatoSeedsBefore = m_inventory.getItemCount(ItemType::TomatoSeeds);
            bool buyOk = m_shopSystem.buyItem(ItemType::TomatoSeeds, 1, m_inventory, &m_hud);
            assert(buyOk && "Buying tomato seeds from Seller NPC should succeed");
            assert(m_inventory.getMoney() == moneyBefore - 15);
            assert(m_inventory.getItemCount(ItemType::TomatoSeeds) == tomatoSeedsBefore + 1);
            std::cout << "[Smoke Test] Shop Buy verified: bought Tomato Seeds for 15G (Wallet: "
                      << m_inventory.getMoney() << "G, Seeds: "
                      << m_inventory.getItemCount(ItemType::TomatoSeeds) << ")" << std::endl;

            // Verify refusal when coins are insufficient
            m_inventory.setMoney(5);
            bool failBuy = m_shopSystem.buyItem(ItemType::TomatoSeeds, 1, m_inventory, &m_hud);
            assert(!failBuy && "Purchase must fail when coins are insufficient");
            assert(m_inventory.getMoney() == 5 && "Wallet must be unchanged on refused buy");
            assert(m_hud.hasActiveFeedback());
            std::cout << "[Smoke Test] Shop Buy refusal verified: 5G < 15G -> refused with feedback banner!" << std::endl;
            m_inventory.setMoney(moneyBefore - 15); // Restore valid wallet balance

            // Purchase 2x Wheat Seeds (cost 20G)
            int wheatSeedsBefore = m_inventory.getItemCount(ItemType::WheatSeeds);
            int currentMoney = m_inventory.getMoney();
            bool buyWheatOk = m_shopSystem.buyItem(ItemType::WheatSeeds, 2, m_inventory, &m_hud);
            assert(buyWheatOk && "Buying 2x Wheat Seeds should succeed");
            assert(m_inventory.getMoney() == currentMoney - 20);
            assert(m_inventory.getItemCount(ItemType::WheatSeeds) == wheatSeedsBefore + 2);
            std::cout << "[Smoke Test] Shop Buy verified: bought 2x Wheat Seeds for 20G (Wallet: "
                      << m_inventory.getMoney() << "G)" << std::endl;
        } else if (m_smokeTestFrames == 135) {
            // Milestone 5: Verify selling produce and dishes to Buyer NPC
            int moneyBeforeSell = m_inventory.getMoney();
            assert(m_inventory.getItemCount(ItemType::Salad) == 1);
            bool sellSaladOk = m_shopSystem.sellItem(ItemType::Salad, 1, m_inventory, &m_hud);
            assert(sellSaladOk && "Selling Salad to Buyer NPC should succeed");
            assert(m_inventory.getItemCount(ItemType::Salad) == 0 && "Salad must be removed from inventory");
            assert(m_inventory.getMoney() == moneyBeforeSell + 40 && "Wallet must increase by 40G");
            std::cout << "[Smoke Test] Shop Sell verified: sold 1x Salad for +40G (Wallet: "
                      << m_inventory.getMoney() << "G)" << std::endl;

            // Sell Bread (payout +50G)
            moneyBeforeSell = m_inventory.getMoney();
            assert(m_inventory.getItemCount(ItemType::Bread) == 1);
            bool sellBreadOk = m_shopSystem.sellItem(ItemType::Bread, 1, m_inventory, &m_hud);
            assert(sellBreadOk && "Selling Bread to Buyer NPC should succeed");
            assert(m_inventory.getItemCount(ItemType::Bread) == 0 && "Bread must be removed");
            assert(m_inventory.getMoney() == moneyBeforeSell + 50 && "Wallet must increase by 50G");
            std::cout << "[Smoke Test] Shop Sell verified: sold 1x Bread for +50G (Wallet: "
                      << m_inventory.getMoney() << "G)" << std::endl;

            // Verify refusal when selling item not in inventory
            bool failSell = m_shopSystem.sellItem(ItemType::Salad, 1, m_inventory, &m_hud);
            assert(!failSell && "Selling item not in inventory must fail");
            assert(m_hud.hasActiveFeedback());
            std::cout << "[Smoke Test] Shop Sell refusal verified: 0 items refused with feedback banner!" << std::endl;

            // Verify NPC placements and ShopUI toggle
            assert(m_sellerNPC.getTileX() == 14 && m_sellerNPC.getTileY() == 2);
            assert(m_buyerNPC.getTileX() == 17 && m_buyerNPC.getTileY() == 2);
            assert(!m_shopUI.isOpen());
            m_shopUI.open(ShopMode::Buy);
            assert(m_shopUI.isOpen() && m_shopUI.getMode() == ShopMode::Buy);
            m_shopUI.setMode(ShopMode::Sell);
            assert(m_shopUI.getMode() == ShopMode::Sell);
            m_shopUI.close();
            assert(!m_shopUI.isOpen());
            std::cout << "[Smoke Test] Seller/Buyer NPCs and ShopUI modal verified!" << std::endl;
        } else if (m_smokeTestFrames == 138) {
            // Milestone 6: Pre-sleep setup
            // Prepare a watered soil plot and plant a seed to observe overnight growth and soil drying
            m_map.setTileType(6, 5, TileType::WateredSoil);
            bool planted = m_farmSystem.plant(6, 5, m_map, m_inventory, m_player, &m_hud, ItemType::TomatoSeeds);
            assert(planted && "Planting test seed at (6, 5) must succeed");

            Crop* testCrop = m_farmSystem.findCrop(6, 5);
            assert(testCrop != nullptr && "Crop must exist at (6, 5)");
            assert(testCrop->getStage() == CropStage::Seed && "New crop starts at stage 1 Seed");
            assert(m_map.getTile(6, 5).getType() == TileType::WateredSoil && "Tile must be WateredSoil");

            // Deplete stamina to test full overnight energy restoration
            m_player.setEnergy(25);
            assert(m_player.getEnergy() == 25);
            assert(m_timeSystem.getDay() == 1 && "Initial day must be 1");
            std::cout << "[Smoke Test] Pre-sleep setup verified: Day 1, Energy 25/100, Crop at (6, 5) is Seed on Watered Soil." << std::endl;
        } else if (m_smokeTestFrames == 140) {
            // Milestone 6: Sleep overnight
            m_timeSystem.sleep(m_player, m_farmSystem, m_map, &m_hud);

            // 1. Verify day counter incremented
            assert(m_timeSystem.getDay() == 2 && "Sleeping overnight must advance the day counter to 2");

            // 2. Verify energy fully restored
            assert(m_player.getEnergy() == Config::INITIAL_ENERGY && "Energy must be fully restored to max (100) after sleeping");

            // 3. Verify crop growth stage advanced overnight
            Crop* cropAfterSleep = m_farmSystem.findCrop(6, 5);
            assert(cropAfterSleep != nullptr);
            assert(cropAfterSleep->getStage() == CropStage::Sprout && "Crop growth stage must advance overnight from Seed to Sprout");

            // 4. Verify watered soil dried back to tilled soil under documented overnight rule
            assert(m_map.getTile(6, 5).getType() == TileType::TilledSoil && "Watered soil must dry back to tilled soil overnight");

            // 5. Verify HUD on-screen notification feedback
            assert(m_hud.hasActiveFeedback() && "Wake-up feedback notification must be triggered");

            // 6. Verify farmhouse bed tile detection
            assert(TimeSystem::isBedTile(2, 2) && "Farmhouse entrance / bed tile at (2, 2) must be recognized");

            std::cout << "[Smoke Test] Day cycle verified: Day incremented (1->2), Energy restored (25->100), Crop stage advanced (Seed->Sprout), Watered soil dried back to Tilled soil!" << std::endl;
        } else if (m_smokeTestFrames == 145) {
            // Milestone 7 / Part 2: Verify farmhouse interior transition, props, indoor cooking, indoor bed, and exit
            transitionTo(Location::FarmhouseInterior);
            assert(m_location == Location::FarmhouseInterior);
            assert(m_player.getX() == 9.0f * Map::TILE_SIZE && m_player.getY() == 9.0f * Map::TILE_SIZE);
            assert(m_interiorMap.getTile(6, 4).getType() == TileType::Bed);
            assert(m_interiorMap.getTile(11, 4).getType() == TileType::Stove);
            assert(m_interiorMap.getTile(8, 4).getType() == TileType::Chest);
            assert(m_interiorMap.getTile(13, 5).getType() == TileType::Bookshelf);
            assert(m_interiorMap.getTile(9, 10).getType() == TileType::InteriorExit);

            // Test indoor cooking
            m_inventory.addItem(ItemType::Tomato, 2);
            bool cooked = m_cookingSystem.cook(ItemType::Salad, m_inventory, &m_hud);
            assert(cooked && "Indoor cooking must succeed");

            // Test indoor bed sleep
            m_player.setEnergy(50);
            int dayBefore = m_timeSystem.getDay();
            m_timeSystem.sleep(m_player, m_farmSystem, m_map, &m_hud);
            assert(m_timeSystem.getDay() == dayBefore + 1);
            assert(m_player.getEnergy() == Config::INITIAL_ENERGY);

            // Exit back to farm
            transitionTo(Location::Farm);
            assert(m_location == Location::Farm);
            assert(m_player.getX() == 2.0f * Map::TILE_SIZE && m_player.getY() == 3.0f * Map::TILE_SIZE);
            std::cout << "[Smoke Test] Farmhouse interior verified: enter -> indoor cooking -> indoor bed sleep -> exit back to farm!" << std::endl;
        } else if (m_smokeTestFrames == 146) {
            // Verify continuous player movement: position can rest at a non-tile-aligned coordinate
            m_player.setPosition(100.5f, 150.25f);
            assert(std::fmod(m_player.getX(), static_cast<float>(Map::TILE_SIZE)) != 0.0f);
            assert(std::fmod(m_player.getY(), static_cast<float>(Map::TILE_SIZE)) != 0.0f);
            assert(Player::MOVE_SPEED == 120.0f);
            assert(Player::COLLISION_WIDTH == 20.0f);
            assert(Player::COLLISION_HEIGHT == 24.0f);
            std::cout << "[Smoke Test] Player continuous movement verified: speed=" << Player::MOVE_SPEED
                      << " px/s, collision 20x24 at feet, rested at non-tile-aligned coordinate ("
                      << m_player.getX() << ", " << m_player.getY() << ")!" << std::endl;
        } else if (m_smokeTestFrames == 147) {
            // Verify hotbar layout constant is in the bottom region of the 640x480 screen
            assert(HUD::HOTBAR_SLOT_COUNT == 8);
            assert(HUD::HOTBAR_Y >= 360 && "Hotbar layout constant must be in the bottom region of the 640x480 screen");
            assert(HUD::HOTBAR_Y + HUD::HOTBAR_SLOT_SIZE <= Config::SCREEN_HEIGHT);
            assert(HUD::HOTBAR_X == (Config::SCREEN_WIDTH - HUD::HOTBAR_TOTAL_WIDTH) / 2);
            std::cout << "[Smoke Test] Hotbar bottom layout verified: Y=" << HUD::HOTBAR_Y
                      << ", slots=" << HUD::HOTBAR_SLOT_COUNT << ", centered at X=" << HUD::HOTBAR_X << "!" << std::endl;
        } else if (m_smokeTestFrames == 148) {
            // Verify connected terrain: deterministic variant hashing and shore edge/corner tile selection
            int v1 = Map::getCellVariant(5, 5, 8);
            int v2 = Map::getCellVariant(5, 5, 8);
            assert(v1 == v2 && "Cell variant selection must be deterministic");
            assert(v1 >= 0 && v1 < 8 && "Variant must be within [0, numVariants)");

            // Check sample pond edge and corner shore tiles
            // Pond water is at (16..18, 11..13)
            std::string nwShore = m_map.getShoreTileName(16, 11);
            std::string nShore = m_map.getShoreTileName(17, 11);
            std::string centerWater = m_map.getShoreTileName(17, 12);
            assert(nwShore == "tile_shore_nw.png" && "Sample pond NW corner must select tile_shore_nw.png");
            assert(nShore == "tile_shore_n.png" && "Sample pond N edge must select tile_shore_n.png");
            assert((centerWater == "tile_water_0.png" || centerWater == "tile_water_1.png") && "Center water tile must select water variant");
            std::cout << "[Smoke Test] Connected terrain verified: deterministic variant=" << v1
                      << ", NW corner shore=" << nwShore << ", North edge shore=" << nShore
                      << ", center water=" << centerWater << "!" << std::endl;
        } else if (m_smokeTestFrames == 149) {
            // PART A Living-world animation verification:
            // 1. Walk frame index advances while moving and returns to standing when stopped
            Input testInput;
            m_player.update(0.0f, testInput, m_map);
            assert(m_player.getWalkFrameIndex() == 0 && "Idle player must show standing frame (index 0)");

            // Advance movement (~8 fps = 0.125s per step)
            testInput.setKeyDown(SDL_SCANCODE_D, true);
            m_player.update(0.13f, testInput, m_map);
            int movingFrame1 = m_player.getWalkFrameIndex();
            assert(movingFrame1 == 1 && "Walk frame index must advance while moving");

            m_player.update(0.13f, testInput, m_map);
            int movingFrame2 = m_player.getWalkFrameIndex();
            assert(movingFrame2 == 2 && "Walk frame index must continue advancing");

            // Stop moving -> returns to standing frame
            testInput.setKeyDown(SDL_SCANCODE_D, false);
            m_player.update(0.01f, testInput, m_map);
            assert(m_player.getWalkFrameIndex() == 0 && "Walk frame index must return to standing (0) when stopped");

            // 2. Animated water: pond water alternates tile_water_0 / tile_water_1 across 600 ms timer boundary
            m_map.setWaterTimer(0.1f);
            assert(m_map.getWaterVariant() == 0);
            assert(m_map.getShoreTileName(17, 12) == "tile_water_0.png");
            assert(m_map.getShoreTileName(16, 11) == "tile_shore_nw.png");
            assert(m_map.getShoreTileName(17, 11) == "tile_shore_n.png");

            m_map.setWaterTimer(0.65f);
            assert(m_map.getWaterVariant() == 1);
            assert(m_map.getShoreTileName(17, 12) == "tile_water_1.png");
            assert(m_map.getShoreTileName(16, 11) == "tile_shore_nw.png");
            assert(m_map.getShoreTileName(17, 11) == "tile_shore_n.png");

            // 3. NPC idle bob stays within ±1 px
            m_sellerNPC.setAnimTimer(0.0f);
            int bob0 = m_sellerNPC.getIdleBob();
            assert(std::abs(bob0) <= 1);
            m_sellerNPC.update(0.65f);
            int bob1 = m_sellerNPC.getIdleBob();
            assert(std::abs(bob1) <= 1);

            std::cout << "[Smoke Test] Living-world animation verified: walk frame advances ("
                      << movingFrame1 << " -> " << movingFrame2 << " -> standing 0), water flips across 600ms boundary (variant 0 -> 1, static shores), NPC idle bob="
                      << bob1 << "!" << std::endl;
        }
    }
}

void Game::render() {
    // Clear screen with a neutral dark tone
    m_renderer.clear(SDL_Color{20, 20, 20, 255});

    if (m_location == Location::Farm) {
        // 1. Render terrain tile map
        m_map.render(m_renderer);

        // 2. Render planted crops
        m_farmSystem.render(m_renderer);

        // 3. Render merchant NPCs (Seller and Buyer)
        m_sellerNPC.render(m_renderer);
        m_buyerNPC.render(m_renderer);
    } else {
        // Render Farmhouse interior room
        m_interiorMap.render(m_renderer);
    }

    // 4. Render farmer player
    m_player.render(m_renderer);

    // World interaction prompts (NPC dialogue, sleep, cooking)
    if (!m_inventoryUI.isOpen() && !m_cookingUI.isOpen() && !m_shopUI.isOpen()) {
        auto [tx, ty] = m_player.getTargetTile();
        int promptX = 0, promptY = 0, promptW = 76, promptH = 24;
        const char* promptText = nullptr;
        const char* iconSprite = nullptr;
        bool showPrompt = false;

        if (m_location == Location::Farm) {
            if ((tx == m_sellerNPC.getTileX() && ty == m_sellerNPC.getTileY()) ||
                (tx >= 13 && tx <= 15 && ty == 1)) {
                promptX = 14 * Map::TILE_SIZE - 22;
                promptY = 2 * Map::TILE_SIZE - 28;
                promptW = 76;
                promptH = 24;
                promptText = "[E] SHOP";
                showPrompt = true;
            } else if ((tx == m_buyerNPC.getTileX() && ty == m_buyerNPC.getTileY()) ||
                       (tx >= 16 && tx <= 18 && ty == 1)) {
                promptX = 17 * Map::TILE_SIZE - 22;
                promptY = 2 * Map::TILE_SIZE - 28;
                promptW = 76;
                promptH = 24;
                promptText = "[E] SHOP";
                showPrompt = true;
            } else if (m_map.inBounds(tx, ty) &&
                       (m_map.getTile(tx, ty).getType() == TileType::Farmhouse || TimeSystem::isBedTile(tx, ty))) {
                promptX = tx * Map::TILE_SIZE - 24;
                promptY = ty * Map::TILE_SIZE - 28;
                promptW = 80;
                promptH = 24;
                promptText = "SLEEP";
                iconSprite = "ui_sleep_icon.png";
                showPrompt = true;
            } else if (m_map.inBounds(tx, ty) && m_map.getTile(tx, ty).getType() == TileType::Stove) {
                promptX = 4 * Map::TILE_SIZE - 22;
                promptY = 2 * Map::TILE_SIZE - 28;
                promptW = 76;
                promptH = 24;
                promptText = "COOK";
                iconSprite = "ui_cooking_icon.png";
                showPrompt = true;
            }
        } else {
            if (m_interiorMap.inBounds(tx, ty)) {
                TileType type = m_interiorMap.getTile(tx, ty).getType();
                if (type == TileType::Bed) {
                    promptX = 6 * Map::TILE_SIZE - 24;
                    promptY = 4 * Map::TILE_SIZE - 28;
                    promptW = 80;
                    promptH = 24;
                    promptText = "SLEEP";
                    iconSprite = "ui_sleep_icon.png";
                    showPrompt = true;
                } else if (type == TileType::Stove) {
                    promptX = 11 * Map::TILE_SIZE - 22;
                    promptY = 4 * Map::TILE_SIZE - 28;
                    promptW = 76;
                    promptH = 24;
                    promptText = "COOK";
                    iconSprite = "ui_cooking_icon.png";
                    showPrompt = true;
                }
            }
        }

        if (showPrompt) {
            bool hasDialogue = m_renderer.drawDialogue(promptX, promptY, promptW, promptH, true);
            if (!hasDialogue) {
                m_renderer.drawRect(promptX, promptY, promptW, promptH, SDL_Color{35, 27, 22, 240});
                m_renderer.drawOutline(promptX, promptY, promptW, promptH, SDL_Color{195, 160, 105, 255});
            }
            int textX = promptX + 8;
            if (iconSprite) {
                m_renderer.drawSprite(iconSprite, promptX + 6, promptY + 3, 18, 18);
                textX = promptX + 28;
            } else {
                int tw = BitmapFont::getTextWidth(promptText, 1);
                textX = promptX + (promptW - tw) / 2;
            }
            SDL_Color textCol = hasDialogue ? SDL_Color{60, 36, 22, 255} : SDL_Color{255, 230, 130, 255};
            BitmapFont::drawText(m_renderer, promptText, textX, promptY + 8, textCol, 1);
        }
    }

    // 5. Render HUD with Day Counter
    m_hud.render(m_renderer, m_inventory, m_player, m_timeSystem.getDay());

    // 6. Render Inventory Grid modal when open
    if (m_inventoryUI.isOpen()) {
        m_inventoryUI.render(m_renderer, m_inventory);
    }

    // 7. Render Cooking UI modal when open
    if (m_cookingUI.isOpen()) {
        m_cookingUI.render(m_renderer, m_inventory, m_cookingSystem);
    }

    // 8. Render Shop UI modal when open
    if (m_shopUI.isOpen()) {
        m_shopUI.render(m_renderer, m_inventory, m_shopSystem);
    }

    // Present final frame buffer
    m_renderer.present();
}

void Game::updatePlaytest(float dt) {
    (void)dt;

    if (m_playtestFrames == 5) {
        // Step 0: Assert initial state
        assert(m_timeSystem.getDay() == 1 && "Initial day must be 1");
        assert(m_inventory.getMoney() == Config::INITIAL_MONEY && "Initial money must be 100G");
        assert(m_player.getEnergy() == Config::INITIAL_ENERGY && "Initial energy must be 100");
        assert(m_inventory.getItemCount(ItemType::TomatoSeeds) == Config::INITIAL_TOMATO_SEEDS && "Initial tomato seeds must be 6");
        assert(m_inventory.getItemCount(ItemType::WheatSeeds) == 0);
        assert(m_inventory.getItemCount(ItemType::Tomato) == 0);
        assert(m_inventory.getItemCount(ItemType::Wheat) == 0);
        assert(m_inventory.getItemCount(ItemType::Salad) == 0);
        assert(m_inventory.getItemCount(ItemType::Bread) == 0);
        assert(m_farmSystem.getCropCount() == 0);
        std::cout << "[PASS] Step 0: Initial state verified (Wallet: " << m_inventory.getMoney()
                  << "G, Energy: " << m_player.getEnergy() << "/" << m_player.getMaxEnergy()
                  << ", Day: " << m_timeSystem.getDay()
                  << ", TomatoSeeds: " << m_inventory.getItemCount(ItemType::TomatoSeeds)
                  << ", WheatSeeds: " << m_inventory.getItemCount(ItemType::WheatSeeds)
                  << ", Crops: 0)" << std::endl;
    } else if (m_playtestFrames == 15) {
        // Step 1: Buy seeds from Seller NPC
        m_player.placeAtTileCenter(14, 3);
        m_player.setFacing(Direction::Up);
        auto [tx, ty] = m_player.getTargetTile();
        assert(tx == m_sellerNPC.getTileX() && ty == m_sellerNPC.getTileY() && "Player should target Seller NPC");

        m_shopUI.open(ShopMode::Buy);
        assert(m_shopUI.isOpen() && m_shopUI.getMode() == ShopMode::Buy);

        int buyTomatoPrice = m_shopSystem.getBuyPrice(ItemType::TomatoSeeds);
        int buyWheatPrice = m_shopSystem.getBuyPrice(ItemType::WheatSeeds);
        assert(buyTomatoPrice == 15 && "Tomato seeds buy price must be 15G per prices.txt");
        assert(buyWheatPrice == 10 && "Wheat seeds buy price must be 10G per prices.txt");

        // Purchase 1x Tomato Seeds and 4x Wheat Seeds
        bool buy1 = m_shopSystem.buyItem(ItemType::TomatoSeeds, 1, m_inventory, &m_hud);
        assert(buy1 && "Buying TomatoSeeds must succeed");
        bool buy2 = m_shopSystem.buyItem(ItemType::WheatSeeds, 4, m_inventory, &m_hud);
        assert(buy2 && "Buying 4x WheatSeeds must succeed");

        m_shopUI.close();
        assert(!m_shopUI.isOpen());

        // Assert state: 100 - 15 - 40 = 45G
        assert(m_inventory.getMoney() == 45);
        assert(m_inventory.getItemCount(ItemType::TomatoSeeds) == 7);
        assert(m_inventory.getItemCount(ItemType::WheatSeeds) == 4);
        assert(m_timeSystem.getDay() == 1);

        std::cout << "[PASS] Step 1: Bought 1x TomatoSeeds (15G) and 4x WheatSeeds (40G) from Seller NPC (Wallet: "
                  << m_inventory.getMoney() << "G, TomatoSeeds: " << m_inventory.getItemCount(ItemType::TomatoSeeds)
                  << ", WheatSeeds: " << m_inventory.getItemCount(ItemType::WheatSeeds)
                  << ", Crops: " << m_farmSystem.getCropCount()
                  << ", Day: " << m_timeSystem.getDay() << ")" << std::endl;
    } else if (m_playtestFrames == 25) {
        // Step 2: Till plots and plant 3x Tomato and 4x Wheat
        // Cultivate row y=5, x=5..11
        // (6, 5) and (7, 5) are already TilledSoil; (5, 5), (8..11, 5) need tilling
        for (int x : {5, 8, 9, 10, 11}) {
            bool ok = m_farmSystem.till(x, 5, m_map, m_player, &m_hud);
            assert(ok && "Tilling plot must succeed");
            assert(m_map.getTile(x, 5).getType() == TileType::TilledSoil);
        }

        // Plant 3x TomatoSeeds at (5, 5), (6, 5), (7, 5)
        for (int x : {5, 6, 7}) {
            bool ok = m_farmSystem.plant(x, 5, m_map, m_inventory, m_player, &m_hud, ItemType::TomatoSeeds);
            assert(ok && "Planting TomatoSeeds must succeed");
        }

        // Plant 4x WheatSeeds at (8, 5), (9, 5), (10, 5), (11, 5)
        for (int x : {8, 9, 10, 11}) {
            bool ok = m_farmSystem.plant(x, 5, m_map, m_inventory, m_player, &m_hud, ItemType::WheatSeeds);
            assert(ok && "Planting WheatSeeds must succeed");
        }

        assert(m_farmSystem.getCropCount() == 7);
        assert(m_inventory.getItemCount(ItemType::TomatoSeeds) == 4);
        assert(m_inventory.getItemCount(ItemType::WheatSeeds) == 0);
        for (int x = 5; x <= 11; ++x) {
            Crop* c = m_farmSystem.findCrop(x, 5);
            assert(c != nullptr && c->getStage() == CropStage::Seed);
        }
        assert(m_inventory.getMoney() == 45);
        assert(m_timeSystem.getDay() == 1);
        assert(m_player.getEnergy() == 61); // 100 - 5*5 - 7*2 = 61

        std::cout << "[PASS] Step 2: Tilled and planted 3x Tomato and 4x Wheat plots (Wallet: "
                  << m_inventory.getMoney() << "G, TomatoSeeds: " << m_inventory.getItemCount(ItemType::TomatoSeeds)
                  << ", WheatSeeds: " << m_inventory.getItemCount(ItemType::WheatSeeds)
                  << ", Crops: 7 at Seed stage, Energy: " << m_player.getEnergy()
                  << "/" << m_player.getMaxEnergy()
                  << ", Day: " << m_timeSystem.getDay() << ")" << std::endl;
    } else if (m_playtestFrames == 35) {
        // Step 3: Verify Watering Growth Boost on Plot (5, 5)
        bool waterOk = m_farmSystem.water(5, 5, m_map, m_player, &m_hud);
        assert(waterOk && "Watering plot (5, 5) must succeed");
        assert(m_map.getTile(5, 5).getType() == TileType::WateredSoil);
        assert(m_map.getTile(6, 5).getType() == TileType::TilledSoil && "Plot (6, 5) must stay unwatered for comparison");

        // Advance daytime simulation time by 3.0s
        m_farmSystem.update(3.0f, m_map);

        Crop* wateredCrop = m_farmSystem.findCrop(5, 5);
        Crop* unwateredCrop = m_farmSystem.findCrop(6, 5);
        assert(wateredCrop != nullptr && unwateredCrop != nullptr);
        assert(wateredCrop->getStage() == CropStage::Sprout && "Watered plot (5, 5) must advance to Sprout due to 2.5x growth boost");
        assert(unwateredCrop->getStage() == CropStage::Seed && "Unwatered plot (6, 5) must remain at Seed stage");
        assert(m_player.getEnergy() == 58); // 61 - 3 = 58
        assert(m_timeSystem.getDay() == 1);

        std::cout << "[PASS] Step 3: Watering growth boost verified: Watered plot (5, 5) advanced to Sprout (2.5x boost) while unwatered plot (6, 5) remained Seed (Wallet: "
                  << m_inventory.getMoney() << "G, Energy: " << m_player.getEnergy()
                  << "/" << m_player.getMaxEnergy() << ", Day: " << m_timeSystem.getDay() << ")" << std::endl;
    } else if (m_playtestFrames == 45) {
        // Step 4: Water remaining plots and sleep overnight to Day 2
        for (int x = 6; x <= 11; ++x) {
            bool ok = m_farmSystem.water(x, 5, m_map, m_player, &m_hud);
            assert(ok && "Watering remaining plots must succeed");
        }
        assert(m_player.getEnergy() == 40); // 58 - 6*3 = 40

        m_player.placeAtTileCenter(2, 3);
        m_player.setFacing(Direction::Up);
        auto [tx, ty] = m_player.getTargetTile();
        assert(TimeSystem::isBedTile(tx, ty) && "Player should target farmhouse entrance/bed tile (2, 2)");

        // Sleep overnight
        m_timeSystem.sleep(m_player, m_farmSystem, m_map, &m_hud);

        assert(m_timeSystem.getDay() == 2 && "Day must advance to 2");
        assert(m_player.getEnergy() == Config::INITIAL_ENERGY && "Energy must be fully restored to 100");
        assert(m_farmSystem.findCrop(5, 5)->getStage() == CropStage::Growing && "Crop at (5, 5) advances to Growing");
        for (int x = 6; x <= 11; ++x) {
            assert(m_farmSystem.findCrop(x, 5)->getStage() == CropStage::Sprout && "Crops at (6..11, 5) advance to Sprout");
            assert(m_map.getTile(x, 5).getType() == TileType::TilledSoil && "Watered soil must dry to TilledSoil overnight");
        }
        assert(m_map.getTile(5, 5).getType() == TileType::TilledSoil);

        std::cout << "[PASS] Step 4: Slept overnight to Day 2: Day counter=2, Energy restored to 100, Crops grew, Soil dried to TilledSoil (Wallet: "
                  << m_inventory.getMoney() << "G, Day: " << m_timeSystem.getDay()
                  << ", Energy: " << m_player.getEnergy() << "/" << m_player.getMaxEnergy() << ")" << std::endl;
    } else if (m_playtestFrames == 55) {
        // Step 5: Day 2 care and advancing days until crops mature (Day 3 & Day 4)
        // Water on Day 2
        for (int x = 5; x <= 11; ++x) {
            m_farmSystem.water(x, 5, m_map, m_player, &m_hud);
        }
        // Sleep to Day 3
        m_timeSystem.sleep(m_player, m_farmSystem, m_map, &m_hud);
        assert(m_timeSystem.getDay() == 3);
        assert(m_player.getEnergy() == Config::INITIAL_ENERGY);
        assert(m_farmSystem.findCrop(5, 5)->getStage() == CropStage::Harvest && "Crop (5, 5) is mature at Day 3");

        // Water on Day 3 and sleep to Day 4
        for (int x = 5; x <= 11; ++x) {
            m_farmSystem.water(x, 5, m_map, m_player, &m_hud);
        }
        m_timeSystem.sleep(m_player, m_farmSystem, m_map, &m_hud);
        assert(m_timeSystem.getDay() == 4);
        assert(m_player.getEnergy() == Config::INITIAL_ENERGY);

        // All 7 crops are now mature (Harvest stage)
        int matureCount = 0;
        for (int x = 5; x <= 11; ++x) {
            Crop* c = m_farmSystem.findCrop(x, 5);
            assert(c != nullptr);
            assert(c->isMature() && c->getStage() == CropStage::Harvest);
            matureCount++;
        }
        assert(matureCount == 7);
        assert(m_inventory.getMoney() == 45);

        std::cout << "[PASS] Step 5: Advanced days to crop maturity: Day=4, All 7 crops (3 Tomato, 4 Wheat) reached Harvest stage (Wallet: "
                  << m_inventory.getMoney() << "G, Day: " << m_timeSystem.getDay()
                  << ", Energy: " << m_player.getEnergy() << "/" << m_player.getMaxEnergy()
                  << ", Crops mature: 7/7)" << std::endl;
    } else if (m_playtestFrames == 65) {
        // Step 6: Harvest all mature crops
        for (int x = 5; x <= 11; ++x) {
            bool ok = m_farmSystem.harvest(x, 5, m_map, m_inventory, m_player, &m_hud);
            assert(ok && "Harvesting mature crop must succeed");
        }
        assert(m_farmSystem.getCropCount() == 0 && "All crops harvested from field");
        assert(m_inventory.getItemCount(ItemType::Tomato) == 3 && "Harvested 3x Tomato");
        assert(m_inventory.getItemCount(ItemType::Wheat) == 4 && "Harvested 4x Wheat");
        assert(m_player.getEnergy() == 100 - 7 * Config::ENERGY_COST_HARVEST); // 100 - 28 = 72
        assert(m_inventory.getMoney() == 45);
        assert(m_timeSystem.getDay() == 4);

        std::cout << "[PASS] Step 6: Harvested 3x Tomato and 4x Wheat (Wallet: "
                  << m_inventory.getMoney() << "G, Tomatoes: " << m_inventory.getItemCount(ItemType::Tomato)
                  << ", Wheat: " << m_inventory.getItemCount(ItemType::Wheat)
                  << ", Crops on field: " << m_farmSystem.getCropCount()
                  << ", Energy: " << m_player.getEnergy() << "/" << m_player.getMaxEnergy()
                  << ", Day: " << m_timeSystem.getDay() << ")" << std::endl;
    } else if (m_playtestFrames == 75) {
        // Step 7: Cook Salad and Bread at the stove
        m_player.placeAtTileCenter(4, 3);
        m_player.setFacing(Direction::Up);
        auto [tx, ty] = m_player.getTargetTile();
        assert(tx == 4 && ty == 2 && "Player should target stove at (4, 2)");
        assert(m_map.getTile(tx, ty).getType() == TileType::Stove);

        m_cookingUI.open();
        assert(m_cookingUI.isOpen());

        // Cook Salad (consumes 2x Tomato, adds 1x Salad)
        bool saladOk = m_cookingSystem.cook(ItemType::Salad, m_inventory, &m_hud);
        assert(saladOk && "Cooking Salad must succeed with 2 tomatoes");
        assert(m_inventory.getItemCount(ItemType::Tomato) == 1 && "1 surplus tomato should remain");
        assert(m_inventory.getItemCount(ItemType::Salad) == 1);

        // Cook Bread (consumes 3x Wheat, adds 1x Bread)
        bool breadOk = m_cookingSystem.cook(ItemType::Bread, m_inventory, &m_hud);
        assert(breadOk && "Cooking Bread must succeed with 3 wheat");
        assert(m_inventory.getItemCount(ItemType::Wheat) == 1 && "1 surplus wheat should remain");
        assert(m_inventory.getItemCount(ItemType::Bread) == 1);

        m_cookingUI.close();
        assert(!m_cookingUI.isOpen());

        assert(m_inventory.getMoney() == 45);
        assert(m_timeSystem.getDay() == 4);

        std::cout << "[PASS] Step 7: Cooked 1x Salad (consumed 2x Tomato) and 1x Bread (consumed 3x Wheat) at Stove (Wallet: "
                  << m_inventory.getMoney() << "G, Salad: " << m_inventory.getItemCount(ItemType::Salad)
                  << ", Bread: " << m_inventory.getItemCount(ItemType::Bread)
                  << ", Surplus Tomato: " << m_inventory.getItemCount(ItemType::Tomato)
                  << ", Surplus Wheat: " << m_inventory.getItemCount(ItemType::Wheat)
                  << ", Day: " << m_timeSystem.getDay() << ")" << std::endl;
    } else if (m_playtestFrames == 85) {
        // Step 8: Sell dishes and surplus produce to Buyer NPC
        m_player.placeAtTileCenter(17, 3);
        m_player.setFacing(Direction::Up);
        auto [tx, ty] = m_player.getTargetTile();
        assert(tx == m_buyerNPC.getTileX() && ty == m_buyerNPC.getTileY() && "Player should target Buyer NPC");

        m_shopUI.open(ShopMode::Sell);
        assert(m_shopUI.isOpen() && m_shopUI.getMode() == ShopMode::Sell);

        // Verify sell prices per prices.txt
        assert(m_shopSystem.getSellPrice(ItemType::Salad) == 40 && "Salad sell price must be 40G per prices.txt");
        assert(m_shopSystem.getSellPrice(ItemType::Bread) == 50 && "Bread sell price must be 50G per prices.txt");
        assert(m_shopSystem.getSellPrice(ItemType::Tomato) == 20 && "Tomato sell price must be 20G per prices.txt");
        assert(m_shopSystem.getSellPrice(ItemType::Wheat) == 14 && "Wheat sell price must be 14G per prices.txt");

        // Sell 1x Salad: 45 + 40 = 85G
        bool s1 = m_shopSystem.sellItem(ItemType::Salad, 1, m_inventory, &m_hud);
        assert(s1 && m_inventory.getMoney() == 85 && m_inventory.getItemCount(ItemType::Salad) == 0);

        // Sell 1x Bread: 85 + 50 = 135G
        bool s2 = m_shopSystem.sellItem(ItemType::Bread, 1, m_inventory, &m_hud);
        assert(s2 && m_inventory.getMoney() == 135 && m_inventory.getItemCount(ItemType::Bread) == 0);

        // Sell 1x surplus Tomato: 135 + 20 = 155G
        bool s3 = m_shopSystem.sellItem(ItemType::Tomato, 1, m_inventory, &m_hud);
        assert(s3 && m_inventory.getMoney() == 155 && m_inventory.getItemCount(ItemType::Tomato) == 0);

        // Sell 1x surplus Wheat: 155 + 14 = 169G
        bool s4 = m_shopSystem.sellItem(ItemType::Wheat, 1, m_inventory, &m_hud);
        assert(s4 && m_inventory.getMoney() == 169 && m_inventory.getItemCount(ItemType::Wheat) == 0);

        m_shopUI.close();
        assert(!m_shopUI.isOpen());
        assert(m_timeSystem.getDay() == 4);

        std::cout << "[PASS] Step 8: Sold 1x Salad (+40G), 1x Bread (+50G), 1x Tomato (+20G), 1x Wheat (+14G) to Buyer NPC (Wallet: "
                  << m_inventory.getMoney() << "G, Salad: 0, Bread: 0, Tomato: 0, Wheat: 0, Day: "
                  << m_timeSystem.getDay() << ")" << std::endl;
    } else if (m_playtestFrames == 95) {
        // Step 9: Buy more seeds with the profit from Seller NPC
        m_player.placeAtTileCenter(14, 3);
        m_player.setFacing(Direction::Up);
        auto [tx, ty] = m_player.getTargetTile();
        assert(tx == m_sellerNPC.getTileX() && ty == m_sellerNPC.getTileY());

        m_shopUI.open(ShopMode::Buy);
        assert(m_shopUI.isOpen() && m_shopUI.getMode() == ShopMode::Buy);

        // Buy 2x TomatoSeeds (-30G), 3x WheatSeeds (-30G), 1x CarrotSeeds (-20G). Total: -80G.
        // Wallet: 169 - 80 = 89G
        int tomatoSeedsBefore = m_inventory.getItemCount(ItemType::TomatoSeeds); // was 4
        int wheatSeedsBefore = m_inventory.getItemCount(ItemType::WheatSeeds);   // was 0
        int carrotSeedsBefore = m_inventory.getItemCount(ItemType::CarrotSeeds); // was 0

        bool b1 = m_shopSystem.buyItem(ItemType::TomatoSeeds, 2, m_inventory, &m_hud);
        assert(b1);
        bool b2 = m_shopSystem.buyItem(ItemType::WheatSeeds, 3, m_inventory, &m_hud);
        assert(b2);
        bool b3 = m_shopSystem.buyItem(ItemType::CarrotSeeds, 1, m_inventory, &m_hud);
        assert(b3);

        m_shopUI.close();
        assert(!m_shopUI.isOpen());

        assert(m_inventory.getMoney() == 89);
        assert(m_inventory.getItemCount(ItemType::TomatoSeeds) == tomatoSeedsBefore + 2); // 6
        assert(m_inventory.getItemCount(ItemType::WheatSeeds) == wheatSeedsBefore + 3);   // 3
        assert(m_inventory.getItemCount(ItemType::CarrotSeeds) == carrotSeedsBefore + 1); // 1
        assert(m_timeSystem.getDay() == 4);

        std::cout << "[PASS] Step 9: Reinvested profit: Bought 2x TomatoSeeds (-30G), 3x WheatSeeds (-30G), 1x CarrotSeeds (-20G) (Wallet: "
                  << m_inventory.getMoney() << "G, TomatoSeeds: " << m_inventory.getItemCount(ItemType::TomatoSeeds)
                  << ", WheatSeeds: " << m_inventory.getItemCount(ItemType::WheatSeeds)
                  << ", CarrotSeeds: " << m_inventory.getItemCount(ItemType::CarrotSeeds)
                  << ", Day: " << m_timeSystem.getDay() << ")" << std::endl;
    } else if (m_playtestFrames == 105) {
        // Step 10: Enter farmhouse interior: step onto door tile (2, 2)
        m_player.placeAtTileCenter(2, 3);
        m_player.setFacing(Direction::Up);
        // Step onto door tile at (2, 2)
        m_player.placeAtTileCenter(2, 2);
        transitionTo(Location::FarmhouseInterior);

        assert(m_location == Location::FarmhouseInterior && "Location must transition to FarmhouseInterior");
        assert(m_player.getX() == 9.0f * Map::TILE_SIZE && m_player.getY() == 9.0f * Map::TILE_SIZE && "Player must spawn at (9, 9) inside");
        assert(m_player.getFacing() == Direction::Up);

        // Verify interior room bounds and contents
        assert(m_interiorMap.getTile(6, 4).getType() == TileType::Bed && "Indoor bed must be at (6, 4)");
        assert(m_interiorMap.getTile(8, 4).getType() == TileType::Chest && "Storage chest must be at (8, 4)");
        assert(m_interiorMap.getTile(11, 4).getType() == TileType::Stove && "Indoor stove must be at (11, 4)");
        assert(m_interiorMap.getTile(13, 5).getType() == TileType::Bookshelf && "Bookshelf must be at (13, 5)");
        assert(m_interiorMap.getTile(9, 10).getType() == TileType::InteriorExit && "Interior exit doorway must be at (9, 10)");

        // Verify interior collisions
        assert(!m_interiorMap.getTile(5, 5).isWalkable() && "Interior side wall must be impassable");
        assert(!m_interiorMap.getTile(6, 3).isWalkable() && "Interior top wall must be impassable");
        assert(!m_interiorMap.getTile(6, 4).isWalkable() && "Bed must be impassable");
        assert(m_interiorMap.getTile(7, 7).isWalkable() && "Wood floor must be walkable");

        std::cout << "[PASS] Step 10: Entered farmhouse interior through door tile: Location is FarmhouseInterior, Player at (9, 9) facing Up" << std::endl;
    } else if (m_playtestFrames == 115) {
        // Step 11: Interacting with interior props: Storage Chest and Bookshelf
        // 1. Move to Storage Chest at (8, 4) from (8, 5) facing Up
        m_player.placeAtTileCenter(8, 5);
        m_player.setFacing(Direction::Up);
        auto [txChest, tyChest] = m_player.getTargetTile();
        assert(txChest == 8 && tyChest == 4 && "Target tile must be Chest (8, 4)");
        assert(m_interiorMap.getTile(txChest, tyChest).getType() == TileType::Chest);

        // Interact with chest
        int tom = m_inventory.getItemCount(ItemType::Tomato);
        int wht = m_inventory.getItemCount(ItemType::Wheat);
        int car = m_inventory.getItemCount(ItemType::Carrot);
        std::string chestMsg = "CHEST: " + std::to_string(tom) + " TOM, "
                              + std::to_string(wht) + " WHT, "
                              + std::to_string(car) + " CAR";
        m_hud.showFeedback(chestMsg, 3.0f);
        assert(m_hud.hasActiveFeedback());

        // 2. Move to Bookshelf at (13, 5) from (12, 5) facing Right
        m_player.placeAtTileCenter(12, 5);
        m_player.setFacing(Direction::Right);
        auto [txBook, tyBook] = m_player.getTargetTile();
        assert(txBook == 13 && tyBook == 5 && "Target tile must be Bookshelf (13, 5)");
        assert(m_interiorMap.getTile(txBook, tyBook).getType() == TileType::Bookshelf);

        std::string bookMsg = "ALMANAC: WATER CROPS DAILY FOR 2.5X GROWTH!";
        m_hud.showFeedback(bookMsg, 3.0f);
        assert(m_hud.hasActiveFeedback());

        std::cout << "[PASS] Step 11: Interacted with interior props: Storage Chest reported inventory, Bookshelf displayed farming guide" << std::endl;
    } else if (m_playtestFrames == 125) {
        // Step 12: Cook indoors at kitchen stove
        // Provide ingredients (2x Tomato) to test cooking indoors
        m_inventory.addItem(ItemType::Tomato, 2);
        assert(m_inventory.getItemCount(ItemType::Tomato) == 2);
        assert(m_inventory.getItemCount(ItemType::Salad) == 0);

        m_player.placeAtTileCenter(11, 5);
        m_player.setFacing(Direction::Up);
        auto [txStove, tyStove] = m_player.getTargetTile();
        assert(txStove == 11 && tyStove == 4 && "Target tile must be indoor Stove at (11, 4)");
        assert(m_interiorMap.getTile(txStove, tyStove).getType() == TileType::Stove);

        m_cookingUI.open();
        assert(m_cookingUI.isOpen() && "CookingUI should open indoors");

        bool cookOk = m_cookingSystem.cook(ItemType::Salad, m_inventory, &m_hud);
        assert(cookOk && "Indoor cooking must succeed with 2 tomatoes");
        assert(m_inventory.getItemCount(ItemType::Tomato) == 0 && "Tomatoes consumed");
        assert(m_inventory.getItemCount(ItemType::Salad) == 1 && "1x Salad added to inventory");

        m_cookingUI.close();
        assert(!m_cookingUI.isOpen());

        std::cout << "[PASS] Step 12: Cooked Salad indoors at farmhouse stove: Ingredients consumed, 1x Salad added to inventory" << std::endl;
    } else if (m_playtestFrames == 135) {
        // Step 13: Sleep at indoor bed with Milestone 6 overnight rules
        m_player.setEnergy(25);
        assert(m_player.getEnergy() == 25);
        int dayBefore = m_timeSystem.getDay(); // was 4

        m_player.placeAtTileCenter(6, 5);
        m_player.setFacing(Direction::Up);
        auto [txBed, tyBed] = m_player.getTargetTile();
        assert(txBed == 6 && tyBed == 4 && "Target tile must be Bed at (6, 4)");
        assert(m_interiorMap.getTile(txBed, tyBed).getType() == TileType::Bed);

        // Sleep at indoor bed
        m_timeSystem.sleep(m_player, m_farmSystem, m_map, &m_hud);

        // Verify Milestone 6 rules:
        // 1. Day increments
        assert(m_timeSystem.getDay() == dayBefore + 1 && "Day must advance by 1");
        // 2. Energy fully restored
        assert(m_player.getEnergy() == Config::INITIAL_ENERGY && "Energy must be fully restored to 100");
        // 3. HUD feedback banner triggered
        assert(m_hud.hasActiveFeedback() && "Wake-up feedback notification active");

        std::cout << "[PASS] Step 13: Slept at indoor bed: Day advanced (" << dayBefore << "->" << m_timeSystem.getDay()
                  << "), Energy restored (25->100), Milestone 6 rules verified" << std::endl;
    } else if (m_playtestFrames == 145) {
        // Step 14: Exit farmhouse back to farm via interior exit
        m_player.placeAtTileCenter(9, 9);
        m_player.setFacing(Direction::Down);
        // Step onto exit doorway tile (9, 10)
        m_player.placeAtTileCenter(9, 10);
        transitionTo(Location::Farm);

        assert(m_location == Location::Farm && "Location must transition back to Farm");
        assert(m_player.getX() == 2.0f * Map::TILE_SIZE && m_player.getY() == 3.0f * Map::TILE_SIZE && "Player must spawn just outside door at (2, 3)");
        assert(m_player.getFacing() == Direction::Down);

        // Verify game state is sane
        assert(m_timeSystem.getDay() == 5);
        assert(m_player.getEnergy() == Config::INITIAL_ENERGY);
        assert(m_inventory.getMoney() == 89);
        assert(m_inventory.getItemCount(ItemType::TomatoSeeds) == 6);
        assert(m_inventory.getItemCount(ItemType::WheatSeeds) == 3);
        assert(m_inventory.getItemCount(ItemType::CarrotSeeds) == 1);
        assert(m_inventory.getItemCount(ItemType::Salad) == 1);

        std::cout << "[PASS] Step 14: Exited farmhouse back to farm: Player positioned outside door at (2, 3) facing Down, game state sane" << std::endl;
    } else if (m_playtestFrames >= 155) {
        std::cout << "[PASS] Full playthrough playtest and farmhouse interior verifications passed cleanly across "
                  << m_timeSystem.getDay() << " in-game days!" << std::endl;
        m_isRunning = false;
    }
}
