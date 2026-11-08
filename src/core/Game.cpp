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

        // 2. Seller NPC interaction check
        if (tx == m_sellerNPC.getTileX() && ty == m_sellerNPC.getTileY()) {
            m_shopUI.open(ShopMode::Buy);
            std::cout << "[Shop] Opened Seller NPC shop at (" << tx << ", " << ty << ")." << std::endl;
            return;
        }

        // 3. Buyer NPC interaction check
        if (tx == m_buyerNPC.getTileX() && ty == m_buyerNPC.getTileY()) {
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
