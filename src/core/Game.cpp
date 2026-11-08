#include "core/Game.h"
#include <iostream>
#include <cassert>

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
                if (m_smokeTestFrames >= 150) {
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

        // 4. Farmhouse / Bed interaction check
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

    // Player movements and world physics only progress if modals are closed
    if (!m_inventoryUI.isOpen() && !m_cookingUI.isOpen() && !m_shopUI.isOpen()) {
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

    // 3. Render merchant NPCs (Seller and Buyer)
    m_sellerNPC.render(m_renderer);
    m_buyerNPC.render(m_renderer);

    // 4. Render farmer player
    m_player.render(m_renderer);

    // 5. Render HUD with Day Counter (hotbar, money counter, energy bar, day counter, inventory hint, tomato counter, feedback banner)
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
        m_player.setPosition(14.0f * Map::TILE_SIZE, 3.0f * Map::TILE_SIZE);
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

        m_player.setPosition(2.0f * Map::TILE_SIZE, 3.0f * Map::TILE_SIZE);
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
        m_player.setPosition(4.0f * Map::TILE_SIZE, 3.0f * Map::TILE_SIZE);
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
        m_player.setPosition(17.0f * Map::TILE_SIZE, 3.0f * Map::TILE_SIZE);
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
        m_player.setPosition(14.0f * Map::TILE_SIZE, 3.0f * Map::TILE_SIZE);
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
    } else if (m_playtestFrames >= 105) {
        std::cout << "[PASS] Full playthrough playtest completed successfully across "
                  << m_timeSystem.getDay() << " in-game days!" << std::endl;
        m_isRunning = false;
    }
}
