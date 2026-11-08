#include "core/Game.h"
#include "ui/BitmapFont.h"
#include "systems/SaveSystem.h"
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
    m_shopSystem.setChickenHandler(
        [this]() { return m_chickenSystem.getChickenCount(); },
        [this]() { return m_chickenSystem.addChicken(); }
    );

    // Initialize farmhouse interior room map
    m_interiorMap.initInterior();

    // Initialize audio system (SDL2_mixer)
    m_audio.init();

    // Wire day advance (egg rolls) and auto-save whenever player sleeps (Milestone 6 / Part C)
    m_timeSystem.setOnDayAdvanceCallback([this](int newDay) {
        m_chickenSystem.onNewMorning(newDay);
    });
    m_timeSystem.setOnSleepCallback([this]() {
        saveGame();
    });

    // Wire lifetime statistics increment callbacks
    m_shopSystem.setOnSellCallback([this](int goldEarned) {
        m_statsSystem.recordSale(goldEarned, &m_hud);
    });
    m_farmSystem.setOnHarvestCallback([this](int count) {
        m_statsSystem.recordHarvest(count);
    });
    m_fishingSystem.setOnCatchFishCallback([this](int count) {
        m_statsSystem.recordCatchFish(count);
    });
    m_chickenSystem.setOnEggCollectedCallback([this](int count) {
        m_statsSystem.recordEggCollected(count);
    });
    m_cookingSystem.setOnCookSuccessCallback([this](int count) {
        m_statsSystem.recordDishCooked(count);
    });

    initRainParticles();

    // In --smoke-test / --playtest modes the title is bypassed (headless runs unchanged).
    // In normal gameplay, the game boots into the title screen.
    if (m_smokeTestMode || m_playtestMode) {
        m_gameState = GameState::Playing;
    } else {
        m_gameState = GameState::Title;
        m_titleUI.init(SaveSystem::exists());
    }

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
    m_audio.cleanUp();
    m_assets.cleanUp();
    m_renderer.cleanUp();

    if (m_window) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }

    // Clean up temporary test save files if smoke test or playtest ran
    if (m_smokeTestMode || m_playtestMode) {
        SaveSystem::removeSave("smoke_save.json");
        SaveSystem::removeSave("savegame.json");
        SaveSystem::removeSave("test_weather_save.json");
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
                if (m_smokeTestFrames >= 158) {
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

void Game::startTransition(TransitionAction action) {
    if (m_fadePhase != FadePhase::None) return;
    m_fishingSystem.cancel();
    m_transitionAction = action;
    m_fadePhase = FadePhase::FadeOut;
    m_fadeTimer = 0.0f;
    if (action == TransitionAction::EnterHouse || action == TransitionAction::ExitHouse) {
        m_audio.playSFX(SFX::Door);
    } else if (action == TransitionAction::Sleep) {
        m_audio.playSFX(SFX::Sleep);
    }
}

void Game::updateTransition(float dt) {
    if (m_fadePhase == FadePhase::FadeOut) {
        m_fadeTimer += dt;
        if (m_fadeTimer >= FADE_OUT_DURATION) {
            m_fadeTimer = 0.0f;
            executeTransitionMidpoint();
            m_fadePhase = FadePhase::FadeIn;
        }
    } else if (m_fadePhase == FadePhase::FadeIn) {
        m_fadeTimer += dt;
        if (m_fadeTimer >= FADE_IN_DURATION) {
            m_fadeTimer = 0.0f;
            m_fadePhase = FadePhase::None;
            m_transitionAction = TransitionAction::None;
        }
    }
}

void Game::executeTransitionMidpoint() {
    switch (m_transitionAction) {
        case TransitionAction::EnterHouse:
            transitionTo(Location::FarmhouseInterior);
            break;
        case TransitionAction::ExitHouse:
            transitionTo(Location::Farm);
            break;
        case TransitionAction::Sleep:
            m_timeSystem.sleep(m_player, m_farmSystem, m_map, &m_hud);
            break;
        case TransitionAction::None:
            break;
    }
}

void Game::fastForwardTransition() {
    if (m_fadePhase == FadePhase::FadeOut) {
        executeTransitionMidpoint();
        m_fadePhase = FadePhase::None;
        m_transitionAction = TransitionAction::None;
        m_fadeTimer = 0.0f;
    } else if (m_fadePhase == FadePhase::FadeIn) {
        m_fadePhase = FadePhase::None;
        m_transitionAction = TransitionAction::None;
        m_fadeTimer = 0.0f;
    }
}

bool Game::saveGame(const std::string& filepath) const {
    return SaveSystem::save(*this, filepath);
}

bool Game::loadGame(const std::string& filepath) {
    return SaveSystem::load(*this, filepath);
}

void Game::resetGame() {
    SaveSystem::removeSave();
    m_timeSystem.setDay(1);
    m_inventory = Inventory();
    m_player = Player();
    m_location = Location::Farm;
    m_map.initDefault();
    m_interiorMap.initInterior();
    m_farmSystem.clearCrops();
    m_chickenSystem.reset();
    m_statsSystem.reset();
    m_audio.setMuted(false);
    m_fadePhase = FadePhase::None;
    m_transitionAction = TransitionAction::None;
    m_fadeTimer = 0.0f;
    m_fishingSystem.cancel();
    m_hud.showFeedback("RESET TO FRESH DAY 1", 3.0f);
    std::cout << "[Reset] Reset to fresh Day 1 (save removed, stats zeroed)." << std::endl;
}

void Game::pauseSaveNow() {
    saveGame();
    m_hud.showFeedback("Saved!", 2.0f);
    std::cout << "[Pause] Save Now: wrote savegame.json immediately." << std::endl;
}

bool Game::eatSelectedItem() {
    ItemType selectedDish = ItemType::Empty;

    if (m_inventoryUI.isOpen()) {
        int slotIdx = m_inventoryUI.getSelectedSlot();
        if (slotIdx >= 0 && slotIdx < m_inventory.getSlotCount()) {
            ItemType t = m_inventory.getSlot(slotIdx).type;
            if (t == ItemType::Salad || t == ItemType::Bread || t == ItemType::Omelette) {
                selectedDish = t;
            }
        }
    } else {
        if (m_inventory.activeTool == ToolType::Salad) {
            selectedDish = ItemType::Salad;
        } else if (m_inventory.activeTool == ToolType::Bread) {
            selectedDish = ItemType::Bread;
        }
    }

    if (selectedDish == ItemType::Empty) {
        return false;
    }

    if (m_inventory.getItemCount(selectedDish) <= 0) {
        m_hud.showFeedback("NO " + itemTypeToString(selectedDish) + " IN INVENTORY!");
        return false;
    }

    // Refusal banner at full energy (do not consume)
    if (m_player.getEnergy() >= m_player.getMaxEnergy()) {
        m_hud.showFeedback("ENERGY ALREADY FULL!");
        std::cout << "[Eat] Refused: Farmer energy is already full ("
                  << m_player.getEnergy() << "/" << m_player.getMaxEnergy() << ")!" << std::endl;
        return false;
    }

    int energyBoost = 25;
    if (selectedDish == ItemType::Bread) {
        energyBoost = 35;
    } else if (selectedDish == ItemType::Omelette) {
        energyBoost = 40;
    }
    m_inventory.removeItem(selectedDish, 1);
    m_player.restoreEnergy(energyBoost);
    std::string banner = "Ate " + itemTypeToString(selectedDish) + ": +" + std::to_string(energyBoost) + " Energy";
    m_hud.showFeedback(banner);
    m_audio.playSFX(SFX::UIConfirm);

    std::cout << "[Eat] " << banner << " [Energy: " << m_player.getEnergy()
              << "/" << m_player.getMaxEnergy() << "]" << std::endl;
    return true;
}

bool Game::tryStartFishing() {
    if (m_location != Location::Farm || isInputLocked() ||
        m_inventoryUI.isOpen() || m_cookingUI.isOpen() || m_shopUI.isOpen()) {
        return false;
    }
    auto [tx, ty] = m_player.getTargetTile();
    if (m_map.inBounds(tx, ty) && m_map.getTile(tx, ty).getType() == TileType::Water) {
        return m_fishingSystem.startFishing(tx, ty, m_player, &m_hud, &m_audio);
    }
    return false;
}

bool Game::refillWateringCan() {
    m_audio.playSFX(SFX::Water);
    m_hud.showFeedback("FILLED WATERING CAN!", 2.0f);
    std::cout << "[Pond] Refilled watering can with fresh water from pond." << std::endl;
    return true;
}

bool Game::interactAt(int tx, int ty) {
    if (m_fishingSystem.isFishing()) {
        m_fishingSystem.handleActionInput(m_inventory, &m_hud, &m_audio);
        return true;
    }

    if (m_location == Location::FarmhouseInterior) {
        if (!m_interiorMap.inBounds(tx, ty)) return false;
        TileType type = m_interiorMap.getTile(tx, ty).getType();
        if (type == TileType::Bed) {
            startTransition(TransitionAction::Sleep);
            std::cout << "[Interior] Slept at comfortable indoor bed." << std::endl;
            return true;
        }
        if (type == TileType::Stove) {
            m_cookingUI.open();
            m_audio.playSFX(SFX::UIOpen);
            std::cout << "[Interior] Opened indoor kitchen stove at (" << tx << ", " << ty << ")." << std::endl;
            return true;
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
            return true;
        }
        if (type == TileType::Bookshelf) {
            std::string msg = "ALMANAC: WATER CROPS DAILY FOR 2.5X GROWTH!";
            m_hud.showFeedback(msg, 3.0f);
            std::cout << "[Interior] Bookshelf: " << msg << std::endl;
            return true;
        }
        if (type == TileType::InteriorExit) {
            startTransition(TransitionAction::ExitHouse);
            return true;
        }
        return false;
    }

    // Location is Farm:
    if (!m_map.inBounds(tx, ty)) return false;

    // 1. Stove kitchen interaction point check
    if (m_map.getTile(tx, ty).getType() == TileType::Stove) {
        m_cookingUI.open();
        m_audio.playSFX(SFX::UIOpen);
        std::cout << "[Stove] Opened cooking interface at (" << tx << ", " << ty << ")." << std::endl;
        return true;
    }

    // 2. Seller NPC interaction check
    if ((tx == m_sellerNPC.getTileX() && ty == m_sellerNPC.getTileY()) ||
        (tx >= 13 && tx <= 15 && ty == 1)) {
        m_shopUI.open(ShopMode::Buy);
        m_audio.playSFX(SFX::UIOpen);
        std::cout << "[Shop] Opened Seller NPC shop at (" << tx << ", " << ty << ")." << std::endl;
        return true;
    }

    // 3. Buyer NPC interaction check
    if ((tx == m_buyerNPC.getTileX() && ty == m_buyerNPC.getTileY()) ||
        (tx >= 16 && tx <= 18 && ty == 1)) {
        m_shopUI.open(ShopMode::Sell);
        m_audio.playSFX(SFX::UIOpen);
        std::cout << "[Shop] Opened Buyer NPC shop at (" << tx << ", " << ty << ")." << std::endl;
        return true;
    }

    // 4. Farmhouse door / entrance check
    if (m_map.getTile(tx, ty).getType() == TileType::FarmhouseDoor || (tx == 2 && ty == 2)) {
        startTransition(TransitionAction::EnterHouse);
        return true;
    }

    // Farmhouse rest structure check
    if (m_map.getTile(tx, ty).getType() == TileType::Farmhouse || TimeSystem::isBedTile(tx, ty)) {
        startTransition(TransitionAction::Sleep);
        return true;
    }

    // 5. Fishing / Pond interaction check
    if (m_map.getTile(tx, ty).getType() == TileType::Water) {
        if (m_inventory.activeTool == ToolType::WateringCan) {
            refillWateringCan();
            return true;
        }
        return tryStartFishing();
    }

    // 5b. Egg collection interaction check
    int currTx = static_cast<int>(std::round(m_player.getX() / static_cast<float>(Map::TILE_SIZE)));
    int currTy = static_cast<int>(std::round(m_player.getY() / static_cast<float>(Map::TILE_SIZE)));
    if (m_chickenSystem.hasEggAt(tx, ty)) {
        m_chickenSystem.collectEgg(tx, ty, m_inventory, &m_hud, &m_audio);
        return true;
    } else if (m_chickenSystem.hasEggAt(currTx, currTy)) {
        m_chickenSystem.collectEgg(currTx, currTy, m_inventory, &m_hud, &m_audio);
        return true;
    }

    // 6. Farming interaction
    // If player uses watering can on untilled soil, inform user
    TileType curType = m_map.getTile(tx, ty).getType();
    if (m_inventory.activeTool == ToolType::WateringCan &&
        curType != TileType::TilledSoil && curType != TileType::WateredSoil &&
        !m_farmSystem.findCrop(tx, ty)) {
        m_hud.showFeedback("SOIL MUST BE TILLED FIRST!");
        return false;
    }

    return m_farmSystem.interact(tx, ty, m_map, m_inventory, m_player, &m_hud, &m_audio);
}

void Game::handleInput() {
    if (m_gameState == GameState::Title) {
        TitleAction action = m_titleUI.handleInput(m_input, &m_audio);
        if (action == TitleAction::Continue) {
            if (loadGame()) {
                std::cout << "[Title] Continued existing game from savegame.json." << std::endl;
            } else {
                std::cout << "[Title] Failed to load savegame.json; starting fresh game." << std::endl;
                resetGame();
            }
            m_gameState = GameState::Playing;
        } else if (action == TitleAction::NewGame) {
            resetGame();
            m_gameState = GameState::Playing;
            std::cout << "[Title] Started fresh new game." << std::endl;
        } else if (action == TitleAction::Quit) {
            m_isRunning = false;
            std::cout << "[Title] Quit requested from title screen." << std::endl;
        }
        return;
    }

    // Input is locked during screen transition fades
    if (isInputLocked()) {
        return;
    }

    // When pause overlay is open:
    if (m_pauseUI.isOpen()) {
        if (m_statsUI.isOpen()) {
            m_statsUI.handleInput(m_input, &m_audio);
            return;
        }

        PauseAction action = m_pauseUI.handleInput(m_input, &m_audio);
        if (action == PauseAction::Resume) {
            // Already closed inside PauseUI
        } else if (action == PauseAction::SaveNow) {
            pauseSaveNow();
        } else if (action == PauseAction::ViewStats) {
            m_statsUI.open(true);
            m_audio.playSFX(SFX::UIOpen);
        } else if (action == PauseAction::QuitToTitle) {
            m_pauseUI.close();
            m_statsUI.close();
            m_fishingSystem.cancel();
            m_gameState = GameState::Title;
            m_titleUI.open(SaveSystem::exists());
            std::cout << "[Pause] Returned to title screen." << std::endl;
        } else if (action == PauseAction::QuitGame) {
            m_isRunning = false;
            std::cout << "[Pause] Quit game requested." << std::endl;
        }
        return;
    }

    // When Stats view is open during gameplay (opened via key T):
    if (m_statsUI.isOpen()) {
        m_statsUI.handleInput(m_input, &m_audio);
        return;
    }

    // In-game reset key [F9]: deletes save and restarts at fresh Day 1
    if (m_input.isKeyPressed(SDL_SCANCODE_F9)) {
        resetGame();
        return;
    }

    // Audio mute toggle key [M]
    if (m_input.isKeyPressed(SDL_SCANCODE_M)) {
        bool muted = m_audio.toggleMute();
        m_hud.showFeedback(muted ? "AUDIO MUTED" : "AUDIO UNMUTED", 1.5f);
        std::cout << "[Audio] Key M pressed: " << (muted ? "MUTED" : "UNMUTED") << std::endl;
    }

    // Eat food key [F] (Salad +25 energy, Bread +35 energy, clamp at max)
    if (m_input.isKeyPressed(SDL_SCANCODE_F)) {
        eatSelectedItem();
    }

    // Inventory toggle key (Tab or I)
    if (m_input.isKeyPressed(SDL_SCANCODE_TAB) || m_input.isKeyPressed(SDL_SCANCODE_I)) {
        if (m_fishingSystem.isFishing()) {
            m_fishingSystem.cancel();
        }
        m_inventoryUI.toggle();
        if (m_inventoryUI.isOpen()) {
            m_audio.playSFX(SFX::UIOpen);
        } else {
            m_audio.playSFX(SFX::UIConfirm);
        }
        std::cout << "[UI] Inventory modal " << (m_inventoryUI.isOpen() ? "OPENED" : "CLOSED") << std::endl;
    }

    // When the inventory grid is open, pass input to the modal and pause world interaction
    if (m_inventoryUI.isOpen()) {
        m_inventoryUI.handleInput(m_input, m_inventory, &m_audio);
        return;
    }

    // When the cooking UI modal is open, pass input to it and pause world interaction
    if (m_cookingUI.isOpen()) {
        m_cookingUI.handleInput(m_input, m_inventory, m_cookingSystem, &m_hud, &m_audio);
        return;
    }

    // When the shop UI modal is open, pass input to it and pause world interaction
    if (m_shopUI.isOpen()) {
        m_shopUI.handleInput(m_input, m_inventory, m_shopSystem, &m_hud, &m_audio);
        return;
    }

    // ESC opens pause overlay when no shop/cooking/inventory modal is open
    if (m_input.isKeyPressed(SDL_SCANCODE_ESCAPE)) {
        if (m_fishingSystem.isFishing()) {
            m_fishingSystem.cancel();
        }
        m_pauseUI.open();
        m_audio.playSFX(SFX::UIOpen);
        return;
    }

    // Key T opens Stats view during gameplay
    if (m_input.isKeyPressed(SDL_SCANCODE_T)) {
        if (m_fishingSystem.isFishing()) {
            m_fishingSystem.cancel();
        }
        m_statsUI.open(false);
        m_audio.playSFX(SFX::UIOpen);
        return;
    }

    // Documented quick-sleep key [Z] (Zzz... rest overnight through fade transition)
    if (m_input.isKeyPressed(SDL_SCANCODE_Z)) {
        startTransition(TransitionAction::Sleep);
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
        auto [tx, ty] = m_player.getTargetTile();
        interactAt(tx, ty);
        return;
    }

    // Mouse click interactions
    if (m_input.isMouseButtonPressed(SDL_BUTTON_LEFT)) {
        int mx = m_input.getMouseX();
        int my = m_input.getMouseY();

        // 1. Hotbar item slot selection via mouse
        int hotbarSlot = m_hud.getHotbarSlotAt(mx, my);
        if (hotbarSlot >= 0) {
            const ToolType hotbarTools[HUD::HOTBAR_SLOT_COUNT] = {
                ToolType::Hoe,
                ToolType::WateringCan,
                ToolType::TomatoSeeds,
                ToolType::WheatSeeds,
                ToolType::CarrotSeeds,
                ToolType::Tomato,
                ToolType::Salad,
                ToolType::Bread
            };
            m_inventory.activeTool = hotbarTools[hotbarSlot];
            m_audio.playSFX(SFX::UIConfirm);
            return;
        }

        // 2. Inventory BAG icon toggle via mouse
        constexpr int PROD_W = 146;
        constexpr int PROD_H = 38;
        constexpr int PROD_X = Config::SCREEN_WIDTH - PROD_W - 14;
        constexpr int PROD_Y = 8;
        if (mx >= PROD_X && mx <= PROD_X + PROD_W && my >= PROD_Y && my <= PROD_Y + PROD_H) {
            if (m_fishingSystem.isFishing()) {
                m_fishingSystem.cancel();
            }
            m_inventoryUI.toggle();
            m_audio.playSFX(m_inventoryUI.isOpen() ? SFX::UIOpen : SFX::UIConfirm);
            return;
        }

        // 3. Gameplay world tile interaction via mouse (NPCs, ground tiles, door, lake, etc.)
        int worldMx = mx - WORLD_OFFSET_X;
        int worldMy = my - WORLD_OFFSET_Y;
        int clickTx = worldMx / Map::TILE_SIZE;
        int clickTy = worldMy / Map::TILE_SIZE;

        Map& activeMap = (m_location == Location::Farm) ? m_map : m_interiorMap;
        if (worldMx >= 0 && worldMy >= 0 && activeMap.inBounds(clickTx, clickTy)) {
            int playerTx = static_cast<int>(std::round(m_player.getX() / static_cast<float>(Map::TILE_SIZE)));
            int playerTy = static_cast<int>(std::round(m_player.getY() / static_cast<float>(Map::TILE_SIZE)));
            int dist = std::max(std::abs(clickTx - playerTx), std::abs(clickTy - playerTy));

            if (dist <= 2) {
                // Turn player towards clicked tile
                if (std::abs(clickTx - playerTx) >= std::abs(clickTy - playerTy)) {
                    if (clickTx > playerTx) m_player.setFacing(Direction::Right);
                    else if (clickTx < playerTx) m_player.setFacing(Direction::Left);
                } else {
                    if (clickTy > playerTy) m_player.setFacing(Direction::Down);
                    else if (clickTy < playerTy) m_player.setFacing(Direction::Up);
                }
                interactAt(clickTx, clickTy);
            } else {
                m_hud.showFeedback("TOO FAR AWAY!", 1.0f);
            }
            return;
        }
    }
}

void Game::update(float dt) {
    if (m_gameState == GameState::Title) {
        m_map.update(dt);
        return;
    }

    // Update on-screen feedback timer
    m_hud.update(dt);

    // If pause overlay or stats view is open, freeze world simulation
    if (m_pauseUI.isOpen() || m_statsUI.isOpen()) {
        return;
    }

    // Screen transition fade update (~300 ms out + 300 ms in, through black)
    updateTransition(dt);

    // Living world animation updates
    m_map.update(dt);
    m_interiorMap.update(dt);
    m_sellerNPC.update(dt);
    m_buyerNPC.update(dt);

    if (m_location == Location::Farm) {
        m_chickenSystem.update(dt);
    }

    // Fishing system update & safety checks
    if (m_fishingSystem.isFishing()) {
        if (m_location != Location::Farm || isInputLocked() || m_inventoryUI.isOpen() || m_cookingUI.isOpen() || m_shopUI.isOpen()) {
            m_fishingSystem.cancel();
        } else {
            m_fishingSystem.update(dt, &m_hud);
        }
    }

    // Player movements and world physics only progress if input is unlocked and modals are closed
    Map& activeMap = (m_location == Location::Farm) ? m_map : m_interiorMap;
    if (!isInputLocked() && !m_inventoryUI.isOpen() && !m_cookingUI.isOpen() && !m_shopUI.isOpen()) {
        if (m_fishingSystem.isFishing()) {
            const bool moveInput = m_input.isKeyDown(SDL_SCANCODE_W) || m_input.isKeyDown(SDL_SCANCODE_UP) ||
                                   m_input.isKeyDown(SDL_SCANCODE_S) || m_input.isKeyDown(SDL_SCANCODE_DOWN) ||
                                   m_input.isKeyDown(SDL_SCANCODE_A) || m_input.isKeyDown(SDL_SCANCODE_LEFT) ||
                                   m_input.isKeyDown(SDL_SCANCODE_D) || m_input.isKeyDown(SDL_SCANCODE_RIGHT);
            if (moveInput) {
                m_fishingSystem.cancel();
                m_hud.showFeedback("FISHING CANCELLED");
            }
        }
        m_player.update(dt, m_input, activeMap);
    }
    if (m_location == Location::Farm && m_timeSystem.getWeather() == Weather::Rainy) {
        updateRainParticles(dt);
    }
    Weather outdoorWeather = (m_location == Location::Farm) ? m_timeSystem.getWeather() : Weather::Sunny;
    m_farmSystem.update(dt, m_map, outdoorWeather);

    // Zone transition check: door transitions trigger by overlapping the door tile
    if (m_location == Location::Farm) {
        if (m_player.overlapsTile(2, 2) && m_fadePhase == FadePhase::None) {
            startTransition(TransitionAction::EnterHouse);
        }
    } else {
        if (m_player.overlapsTile(9, 10) && m_fadePhase == FadePhase::None) {
            startTransition(TransitionAction::ExitHouse);
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
        } else if (m_smokeTestFrames == 150) {
            // PART B Screen transitions verification:
            // 1. Verify fade phases progress in order and complete, locking input
            assert(m_fadePhase == FadePhase::None);
            assert(!isInputLocked());

            startTransition(TransitionAction::EnterHouse);
            assert(m_fadePhase == FadePhase::FadeOut);
            assert(isInputLocked());
            assert(m_location == Location::Farm && "Location must not change during FadeOut before midpoint");

            // Step through halfway of FadeOut (0.15s)
            updateTransition(0.15f);
            assert(m_fadePhase == FadePhase::FadeOut);
            assert(m_location == Location::Farm);

            // Step through midpoint boundary (0.16s, total > 0.30s)
            updateTransition(0.16f);
            assert(m_fadePhase == FadePhase::FadeIn && "FadeOut must transition to FadeIn at black midpoint");
            assert(m_location == Location::FarmhouseInterior && "State change must execute at midpoint");
            assert(m_player.getX() == 9 * Map::TILE_SIZE && m_player.getY() == 9 * Map::TILE_SIZE);
            assert(isInputLocked());

            // Complete FadeIn (0.31s)
            updateTransition(0.31f);
            assert(m_fadePhase == FadePhase::None && "Transition must complete to None");
            assert(!isInputLocked() && "Input must unlock after fade completes");

            // 2. Sleeping through a fade advances day exactly once with day-cycle assertions intact
            int dayBefore = m_timeSystem.getDay();
            m_player.setEnergy(25);
            // Prepare plot (6, 5) as watered soil
            m_map.setTileType(6, 5, TileType::WateredSoil);

            startTransition(TransitionAction::Sleep);
            assert(m_fadePhase == FadePhase::FadeOut);
            assert(m_timeSystem.getDay() == dayBefore && "Day must not advance before midpoint");

            // Advance through midpoint
            updateTransition(0.31f);
            assert(m_fadePhase == FadePhase::FadeIn);
            assert(m_timeSystem.getDay() == dayBefore + 1 && "Day must advance exactly once at midpoint");
            assert(m_player.getEnergy() == m_player.getMaxEnergy() && "Energy must restore fully to 100");
            assert(m_map.getTile(6, 5).getType() == TileType::TilledSoil && "Watered soil must dry to tilled soil");

            // Complete FadeIn
            updateTransition(0.31f);
            assert(m_fadePhase == FadePhase::None);
            assert(m_timeSystem.getDay() == dayBefore + 1 && "Day must remain dayBefore + 1 after fade completes");

            // Reset player location back to Farm for cleanliness
            transitionTo(Location::Farm);

            std::cout << "[Smoke Test] Screen transitions verified: fade phases progress in order (None -> FadeOut -> Midpoint -> FadeIn -> None), input locks, and sleeping through fade advances day exactly once!" << std::endl;
        } else if (m_smokeTestFrames == 151) {
            // PART C Save / load verification:
            // 1. Verify standard new-game state on no-save boot
            assert(!SaveSystem::exists("non_existent_save_file.json"));

            // 2. Set up distinctive known state to save
            const std::string testSaveFile = "smoke_save.json";
            m_timeSystem.setDay(6);
            m_inventory.setMoney(375);
            m_player.setEnergy(82);
            m_player.setPosition(128.0f, 160.0f);
            m_player.setFacing(Direction::Right);
            m_location = Location::Farm;
            m_inventory.activeTool = ToolType::WateringCan;
            m_inventory.getSlots().clear();
            m_inventory.getSlots().push_back(InventorySlot{ItemType::Tomato, 4});
            m_inventory.getSlots().push_back(InventorySlot{ItemType::Bread, 2});
            m_inventory.getSlots().push_back(InventorySlot{ItemType::WheatSeeds, 5});
            while (m_inventory.getSlots().size() < static_cast<size_t>(Config::INVENTORY_SLOT_COUNT)) {
                m_inventory.getSlots().push_back(InventorySlot{ItemType::Empty, 0});
            }

            // Set up a crop plot at (8, 6)
            m_map.setTileType(8, 6, TileType::WateredSoil);
            m_farmSystem.clearCrops();
            m_farmSystem.addCrop(Crop(8, 6, ItemType::Wheat, CropStage::Growing, 2.5f));

            // Save state
            bool saved = saveGame(testSaveFile);
            assert(saved && "SaveSystem::save must succeed");
            assert(SaveSystem::exists(testSaveFile) && "Saved file must exist on disk");

            // 3. Scramble the state completely
            m_timeSystem.setDay(99);
            m_inventory.setMoney(0);
            m_player.setEnergy(5);
            m_player.setPosition(0.0f, 0.0f);
            m_player.setFacing(Direction::Up);
            m_location = Location::FarmhouseInterior;
            m_inventory.activeTool = ToolType::Hoe;
            m_inventory.getSlots().clear();
            m_inventory.getSlots().resize(Config::INVENTORY_SLOT_COUNT);
            m_map.setTileType(8, 6, TileType::Dirt);
            m_farmSystem.clearCrops();

            // Verify state is indeed scrambled
            assert(m_timeSystem.getDay() == 99);
            assert(m_inventory.getMoney() == 0);
            assert(m_player.getEnergy() == 5);
            assert(m_farmSystem.getCropCount() == 0);

            // 4. Load state from save
            bool loaded = loadGame(testSaveFile);
            assert(loaded && "SaveSystem::load must succeed");

            // 5. Assert day, wallet, inventory, and at least one crop plot are restored exactly
            assert(m_timeSystem.getDay() == 6 && "Day must be restored to 6");
            assert(m_inventory.getMoney() == 375 && "Wallet must be restored to 375G");
            assert(m_player.getEnergy() == 82 && "Energy must be restored to 82");
            assert(m_player.getX() == 128.0f && m_player.getY() == 160.0f && "Player position must be restored");
            assert(m_player.getFacing() == Direction::Right && "Facing must be restored to Right");
            assert(m_location == Location::Farm && "Location must be restored to Farm");
            assert(m_inventory.activeTool == ToolType::WateringCan && "Active tool must be restored to WateringCan");
            assert(m_inventory.getItemCount(ItemType::Tomato) == 4 && "Tomato count must be 4");
            assert(m_inventory.getItemCount(ItemType::Bread) == 2 && "Bread count must be 2");
            assert(m_inventory.getItemCount(ItemType::WheatSeeds) == 5 && "WheatSeeds count must be 5");

            // Assert crop plot at (8, 6) is restored exactly
            Crop* loadedCrop = m_farmSystem.findCrop(8, 6);
            assert(loadedCrop != nullptr && "Crop at (8, 6) must be restored");
            assert(loadedCrop->getCropType() == ItemType::Wheat && "Crop type must be Wheat");
            assert(loadedCrop->getStage() == CropStage::Growing && "Crop stage must be Growing");
            assert(m_map.getTile(8, 6).getType() == TileType::WateredSoil && "Soil state must be WateredSoil");
            assert(m_map.getTile(8, 6).isWatered() && "Watered flag must be true");

            // Verify mute persistence in save file
            m_audio.setMuted(true);
            saveGame(testSaveFile);
            m_audio.setMuted(false);
            loadGame(testSaveFile);
            assert(m_audio.isMuted() && "Mute state must be restored as true");
            m_audio.setMuted(false);

            // Clean up test save file
            SaveSystem::removeSave(testSaveFile);
            assert(!SaveSystem::exists(testSaveFile) && "Test save file must be deleted");

            // Also test F9 in-game reset behavior
            saveGame(testSaveFile);
            assert(SaveSystem::exists(testSaveFile));
            SaveSystem::removeSave(testSaveFile);
            resetGame();
            assert(m_timeSystem.getDay() == 1 && "F9 Reset must return to Day 1");
            assert(m_inventory.getMoney() == Config::INITIAL_MONEY && "F9 Reset must restore initial money");
            assert(m_player.getEnergy() == Config::INITIAL_ENERGY && "F9 Reset must restore initial energy");
            assert(m_inventory.getItemCount(ItemType::TomatoSeeds) == Config::INITIAL_TOMATO_SEEDS);

            std::cout << "[Smoke Test] Save / Load verified: save -> scramble -> load restored day (6), wallet (375G), inventory (4 Tomato, 2 Bread, 5 WheatSeeds), crop plot (8, 6 Wheat Growing on WateredSoil), and F9 reset verified!" << std::endl;
        } else if (m_smokeTestFrames == 152) {
            // PART A AudioSystem verification:
            // 1. Audio init under dummy driver
            assert(m_audio.isAvailable() && "AudioSystem should be available under dummy driver");

            // 2. Mute toggle flips state
            assert(!m_audio.isMuted() && "Audio starts unmuted by default");
            bool muted1 = m_audio.toggleMute();
            assert(muted1 && m_audio.isMuted() && "Audio should be muted after first toggle");
            bool muted2 = m_audio.toggleMute();
            assert(!muted2 && !m_audio.isMuted() && "Audio should be unmuted after second toggle");

            // 3. Every SFX trigger path is safe with audio disabled
            m_audio.setAvailable(false);
            const SFX sfxList[] = {
                SFX::Till, SFX::Water, SFX::Plant, SFX::Harvest,
                SFX::CookSuccess, SFX::CookFail, SFX::Buy, SFX::Sell,
                SFX::Sleep, SFX::UIOpen, SFX::UIConfirm, SFX::Door
            };
            for (SFX sfx : sfxList) {
                m_audio.playSFX(sfx);
            }
            m_audio.playMusic();
            m_audio.stopMusic();
            m_audio.setAvailable(true);

            // 4. Safe with audio enabled under dummy sink
            for (SFX sfx : sfxList) {
                m_audio.playSFX(sfx);
            }

            std::cout << "[Smoke Test] AudioSystem verified: dummy init clean, mute toggle flips state, and every SFX trigger path safe with audio disabled!" << std::endl;
        } else if (m_smokeTestFrames == 153) {
            // PART B Eat food for energy verification:
            // Setup: add 1x Salad and 1x Bread to inventory, set energy to 50
            m_inventory.addItem(ItemType::Salad, 1);
            m_inventory.addItem(ItemType::Bread, 1);
            m_player.setEnergy(50);
            assert(m_player.getEnergy() == 50);

            // 1. Eat Salad with active tool in hotbar: +25 energy (50 -> 75)
            m_inventory.activeTool = ToolType::Salad;
            bool ateSalad = eatSelectedItem();
            assert(ateSalad && "Eating Salad should succeed");
            assert(m_player.getEnergy() == 75 && "Eating Salad must restore exactly +25 Energy (50 -> 75)");
            assert(m_inventory.getItemCount(ItemType::Salad) == 0 && "Salad must be consumed from inventory");
            assert(m_hud.getActiveFeedback() == "Ate Salad: +25 Energy");

            // 2. Eat Bread with active tool in hotbar: +35 energy with clamp at 100 (75 + 35 = 110 -> clamped to 100)
            m_inventory.activeTool = ToolType::Bread;
            bool ateBread = eatSelectedItem();
            assert(ateBread && "Eating Bread should succeed");
            assert(m_player.getEnergy() == 100 && "Energy must clamp at max 100 (75 + 35 -> 100)");
            assert(m_inventory.getItemCount(ItemType::Bread) == 0 && "Bread must be consumed from inventory");
            assert(m_hud.getActiveFeedback() == "Ate Bread: +35 Energy");

            // 3. Full-energy refusal: energy is at 100, item must NOT be consumed
            m_inventory.addItem(ItemType::Salad, 1);
            m_inventory.addItem(ItemType::Bread, 1);
            assert(m_player.getEnergy() == 100);

            m_inventory.activeTool = ToolType::Salad;
            bool refusedSalad = eatSelectedItem();
            assert(!refusedSalad && "Eating at full energy must be refused");
            assert(m_inventory.getItemCount(ItemType::Salad) == 1 && "Item must not be consumed when refused at full energy");
            assert(m_player.getEnergy() == 100 && "Energy must remain unchanged at 100");
            assert(m_hud.hasActiveFeedback());

            m_inventory.activeTool = ToolType::Bread;
            bool refusedBread = eatSelectedItem();
            assert(!refusedBread && "Eating Bread at full energy must be refused");
            assert(m_inventory.getItemCount(ItemType::Bread) == 1 && "Bread must not be consumed when refused at full energy");
            assert(m_player.getEnergy() == 100);

            // 4. Verify eating from open inventory UI modal
            m_player.setEnergy(60);
            m_inventoryUI.open();
            // Find slot with Salad in inventory
            int saladSlot = -1;
            for (int i = 0; i < m_inventory.getSlotCount(); ++i) {
                if (m_inventory.getSlot(i).type == ItemType::Salad) {
                    saladSlot = i;
                    break;
                }
            }
            assert(saladSlot >= 0 && "Salad slot must be found in inventory");
            m_inventoryUI.setSelectedSlot(saladSlot);

            bool ateFromInv = eatSelectedItem();
            assert(ateFromInv && "Eating Salad from open inventory modal should succeed");
            assert(m_player.getEnergy() == 85 && "Energy restored +25 (60 -> 85)");
            assert(m_inventory.getItemCount(ItemType::Salad) == 0 && "Salad consumed from inventory modal");
            assert(m_hud.getActiveFeedback() == "Ate Salad: +25 Energy");
            m_inventoryUI.close();

            std::cout << "[Smoke Test] Eating food verified: exact energy additions (Salad +25, Bread +35), clamping at 100, full-energy refusal retains items!" << std::endl;
        } else if (m_smokeTestFrames == 154) {
            // PART C Weather verification:
            // 1. Assert deterministic seeded weather roll across days:
            assert(calculateWeatherForDay(1) == Weather::Sunny);
            assert(calculateWeatherForDay(2) == Weather::Sunny);
            assert(calculateWeatherForDay(3) == Weather::Sunny);
            assert(calculateWeatherForDay(4) == Weather::Sunny);
            assert(calculateWeatherForDay(5) == Weather::Sunny);
            assert(calculateWeatherForDay(6) == Weather::Rainy);
            assert(calculateWeatherForDay(7) == Weather::Sunny);
            assert(calculateWeatherForDay(8) == Weather::Rainy);

            // 2. Weather roll on day set & sleep:
            m_timeSystem.setDay(6);
            assert(m_timeSystem.getWeather() == Weather::Rainy && "Day 6 must roll Rainy");
            m_timeSystem.setDay(7);
            assert(m_timeSystem.getWeather() == Weather::Sunny && "Day 7 must roll Sunny");

            // 3. Test outdoor planted crop watering on Rainy day (boost without manual watering)
            m_timeSystem.setDay(6); // Rainy
            assert(m_timeSystem.getWeather() == Weather::Rainy);

            // Clear any old crops
            m_farmSystem.clearCrops();
            // Till a plot at (10, 6)
            m_map.setTileType(10, 6, TileType::TilledSoil);
            m_inventory.addItem(ItemType::TomatoSeeds, 1);
            m_inventory.activeTool = ToolType::TomatoSeeds;
            m_player.placeAtTileCenter(10, 7);
            m_player.setFacing(Direction::Up);
            bool planted = m_farmSystem.interact(10, 6, m_map, m_inventory, m_player, &m_hud);
            assert(planted && "Planting on rainy day should succeed");

            // On rainy day, outdoor planted crop tile is watered by rain
            m_farmSystem.update(0.1f, m_map, m_timeSystem.getWeather());
            assert(m_map.getTile(10, 6).getType() == TileType::WateredSoil && "Soil must show WateredSoil under rain");
            assert(m_map.getTile(10, 6).isWatered());

            // Verify 2.5x growth boost applies without manual watering:
            Crop* crop = m_farmSystem.findCrop(10, 6);
            assert(crop != nullptr && "Crop must be planted");
            crop->setStageTimer(0.0f);
            m_farmSystem.update(1.0f, m_map, m_timeSystem.getWeather());
            // dt = 1.0f with 2.5x boost -> stageTimer must equal 2.5f
            assert(std::abs(crop->getStageTimer() - 2.5f) < 0.001f && "Rained crop must receive 2.5x boost without manual watering");

            // 4. Save/Load weather persistence
            m_timeSystem.setWeather(Weather::Rainy);
            bool saved = SaveSystem::save(*this, "test_weather_save.json");
            assert(saved && "Save with Rainy weather should succeed");

            m_timeSystem.setWeather(Weather::Sunny);
            assert(m_timeSystem.getWeather() == Weather::Sunny);

            bool loaded = SaveSystem::load(*this, "test_weather_save.json");
            assert(loaded && "Load from weather save should succeed");
            assert(m_timeSystem.getWeather() == Weather::Rainy && "Weather must be restored as Rainy from save file");
            SaveSystem::removeSave("test_weather_save.json");

            std::cout << "[Smoke Test] Weather verified: deterministic day roll (Days 1-5 Sunny, Day 6 Rainy), rain auto-watering with 2.5x growth boost, and save/load persistence!" << std::endl;
        } else if (m_smokeTestFrames == 155) {
            // PART D Pond Fishing System verification:
            // 1. State machine integration constraints
            m_player.placeAtTileCenter(15, 11);
            m_player.setFacing(Direction::Right);

            // Cannot fish indoors
            m_location = Location::FarmhouseInterior;
            bool fishIndoor = tryStartFishing();
            assert(!fishIndoor && !m_fishingSystem.isFishing() && "Fishing cannot start indoors");
            m_location = Location::Farm;

            // Cannot fish while modal is open
            m_inventoryUI.open();
            bool fishModal = tryStartFishing();
            assert(!fishModal && !m_fishingSystem.isFishing() && "Fishing cannot start while modal is open");
            m_inventoryUI.close();

            // Cannot fish while transitioning
            m_fadePhase = FadePhase::FadeOut;
            bool fishTransition = tryStartFishing();
            assert(!fishTransition && !m_fishingSystem.isFishing() && "Fishing cannot start while transitioning");
            m_fadePhase = FadePhase::None;

            // Active fishing is cancelled if modal opens
            m_player.setEnergy(50);
            bool cast1 = tryStartFishing();
            assert(cast1 && m_fishingSystem.isFishing());
            m_inventoryUI.open();
            if (m_inventoryUI.isOpen()) m_fishingSystem.cancel();
            assert(!m_fishingSystem.isFishing() && "Opening modal cancels active fishing");
            m_inventoryUI.close();

            // Moving cancels fishing
            m_player.setEnergy(50);
            bool cast2 = tryStartFishing();
            assert(cast2 && m_fishingSystem.isFishing());
            m_fishingSystem.cancel();
            assert(!m_fishingSystem.isFishing() && "Movement / cancel resets fishing state to None");

            // 2. Successful catch adds Fish and costs 3 energy (deterministic via injection)
            m_player.setEnergy(50);
            int initialFishCount = m_inventory.getItemCount(ItemType::Fish);
            m_fishingSystem.setNextWaitDuration(3.0f);
            bool castOk = m_fishingSystem.startFishing(16, 11, m_player, &m_hud, &m_audio);
            assert(castOk && "Starting fishing on pond water tile must succeed");
            assert(m_player.getEnergy() == 47 && "Fishing must cost exactly 3 energy (50 -> 47)");
            assert(m_fishingSystem.getState() == FishingState::Waiting);

            // Advance time before wait expires (1.5s < 3.0s)
            m_fishingSystem.update(1.5f, &m_hud);
            assert(m_fishingSystem.getState() == FishingState::Waiting && "Still waiting for bite");

            // Advance remaining time into bite window (total 3.1s >= 3.0s)
            m_fishingSystem.update(1.6f, &m_hud);
            assert(m_fishingSystem.getState() == FishingState::Bite && "Bite window must open after wait duration");

            // Press E inside ~800ms bite window
            bool catchOk = m_fishingSystem.handleActionInput(m_inventory, &m_hud, &m_audio);
            assert(catchOk && "Pressing E during bite window must catch fish");
            assert(m_inventory.getItemCount(ItemType::Fish) == initialFishCount + 1 && "Fish count must increment by exactly 1");
            assert(m_player.getEnergy() == 47 && "Reeling in must not cost extra energy");
            assert(m_fishingSystem.getState() == FishingState::None);
            assert(m_hud.getActiveFeedback() == "CAUGHT 1 FISH!");

            // 3. Early press cancels with no fish (no refund)
            m_fishingSystem.setNextWaitDuration(4.0f);
            m_fishingSystem.startFishing(16, 11, m_player, &m_hud, &m_audio);
            assert(m_player.getEnergy() == 44 && "Energy reduced by 3 (47 -> 44)");
            assert(m_fishingSystem.getState() == FishingState::Waiting);

            // Advance 1.0s (less than 4.0s) and press E early
            m_fishingSystem.update(1.0f, &m_hud);
            assert(m_fishingSystem.getState() == FishingState::Waiting);
            bool earlyHandled = m_fishingSystem.handleActionInput(m_inventory, &m_hud, &m_audio);
            assert(earlyHandled && "Early action handled");
            assert(m_fishingSystem.getState() == FishingState::None && "Early press cancels fishing session");
            assert(m_inventory.getItemCount(ItemType::Fish) == initialFishCount + 1 && "No fish awarded on early press");
            assert(m_player.getEnergy() == 44 && "No energy refund on early press");
            assert(m_hud.getActiveFeedback() == "TOO EARLY! FISH SCARED AWAY!");

            // 4. Expired window = fish escapes
            m_fishingSystem.setNextWaitDuration(2.0f);
            m_fishingSystem.startFishing(16, 11, m_player, &m_hud, &m_audio);
            assert(m_player.getEnergy() == 41 && "Energy reduced by 3 (44 -> 41)");
            m_fishingSystem.update(2.05f, &m_hud);
            assert(m_fishingSystem.getState() == FishingState::Bite && "Bite window opened");

            // Let bite window pass (> 0.8s) without pressing E
            m_fishingSystem.update(0.85f, &m_hud);
            assert(m_fishingSystem.getState() == FishingState::None && "Expired bite window returns state to None");
            assert(m_inventory.getItemCount(ItemType::Fish) == initialFishCount + 1 && "No fish awarded when window expires");
            assert(m_hud.getActiveFeedback() == "THE FISH ESCAPED!");

            // 5. Full-inventory escape path (full bag = fish escapes with banner)
            Inventory backup = m_inventory;
            for (auto& slot : m_inventory.getSlots()) {
                slot.type = ItemType::Wheat;
                slot.count = Config::MAX_STACK_SIZE;
            }
            assert(!m_inventory.hasSpaceFor(ItemType::Fish, 1) && "Bag must be completely full");
            m_fishingSystem.setNextWaitDuration(2.0f);
            m_fishingSystem.startFishing(16, 11, m_player, &m_hud, &m_audio);
            m_fishingSystem.update(2.05f, &m_hud);
            assert(m_fishingSystem.getState() == FishingState::Bite);

            bool fullHandled = m_fishingSystem.handleActionInput(m_inventory, &m_hud, &m_audio);
            assert(fullHandled);
            assert(m_fishingSystem.getState() == FishingState::None);
            assert(m_hud.getActiveFeedback() == "INVENTORY FULL! FISH ESCAPED!");
            m_inventory = backup;

            std::cout << "[Smoke Test] Fishing system verified: state machine clean, catch (+1 fish, -3 energy), early cancel, timeout escape, and full bag escape!" << std::endl;
        } else if (m_smokeTestFrames == 156) {
            // Chicken System verification:
            // 1. Pen fence survey & layout check
            assert(m_map.getTile(1, 10).getType() == TileType::Fence);
            assert(m_map.getTile(2, 10).getType() == TileType::Fence);
            assert(m_map.getTile(3, 10).getType() == TileType::Grass && "Gate gap at (3, 10) must be walkable Grass");
            assert(m_map.getTile(4, 10).getType() == TileType::Fence);
            assert(m_map.getTile(5, 10).getType() == TileType::Fence);
            assert(m_map.getTile(6, 10).getType() == TileType::Fence);
            assert(m_map.getTile(1, 14).getType() == TileType::Fence);
            assert(m_map.getTile(6, 14).getType() == TileType::Fence);

            // Fresh state starts with 0 chickens and 0 ground eggs
            m_chickenSystem.reset();
            assert(m_chickenSystem.getChickenCount() == 0);
            assert(m_chickenSystem.getGroundEggCount() == 0);

            // 2. Buy chicken (wallet -100, count +1, does not enter bag)
            assert(m_shopSystem.getBuyPrice(ItemType::Chicken) == 100);
            m_inventory.setMoney(500);
            bool buy1 = m_shopSystem.buyItem(ItemType::Chicken, 1, m_inventory, &m_hud, &m_audio);
            assert(buy1 && "Buying chicken for 100G succeeds");
            assert(m_inventory.getMoney() == 400 && "Wallet must be reduced by exactly 100G (500 -> 400)");
            assert(m_chickenSystem.getChickenCount() == 1 && "Chicken count increments to 1");
            assert(m_inventory.getItemCount(ItemType::Chicken) == 0 && "Chicken does not enter inventory slots");
            assert(m_chickenSystem.getChickens()[0].getX() >= 64.0f && m_chickenSystem.getChickens()[0].getX() <= 160.0f);
            assert(m_chickenSystem.getChickens()[0].getY() >= 352.0f && m_chickenSystem.getChickens()[0].getY() <= 416.0f);

            // 3. Cap refusal at 6 with no charge
            m_inventory.setMoney(1000);
            for (int i = 0; i < 5; ++i) {
                bool buyExtra = m_shopSystem.buyItem(ItemType::Chicken, 1, m_inventory, &m_hud, &m_audio);
                assert(buyExtra);
            }
            assert(m_chickenSystem.getChickenCount() == 6 && "Pen is now at capacity of 6 chickens");
            assert(m_inventory.getMoney() == 500);

            bool buy7 = m_shopSystem.buyItem(ItemType::Chicken, 1, m_inventory, &m_hud, &m_audio);
            assert(!buy7 && "Buying chicken beyond cap 6 must be refused");
            assert(m_chickenSystem.getChickenCount() == 6 && "Chicken count remains at 6");
            assert(m_inventory.getMoney() == 500 && "No charge on refused cap purchase");
            assert(m_hud.getActiveFeedback() == "PEN AT CAPACITY (6/6)!");

            // Selling chickens is not supported
            bool sellChicken = m_shopSystem.sellItem(ItemType::Chicken, 1, m_inventory, &m_hud, &m_audio);
            assert(!sellChicken && "Selling chickens must not be supported");

            // 4. Forced egg spawn overnight
            m_chickenSystem.clearGroundEggs();
            m_chickenSystem.forceNextEggSpawn(true);
            int dayBeforeSleep = m_timeSystem.getDay();
            m_timeSystem.sleep(m_player, m_farmSystem, m_map, &m_hud);
            assert(m_timeSystem.getDay() == dayBeforeSleep + 1);
            assert(m_chickenSystem.getGroundEggCount() == 6 && "Each of 6 chickens laid 1 egg under forced spawn");
            for (const auto& egg : m_chickenSystem.getGroundEggs()) {
                assert(egg.tileX >= ChickenSystem::PEN_MIN_TILE_X && egg.tileX <= ChickenSystem::PEN_MAX_TILE_X);
                assert(egg.tileY >= ChickenSystem::PEN_MIN_TILE_Y && egg.tileY <= ChickenSystem::PEN_MAX_TILE_Y);
            }

            // 5. Collect egg into inventory respecting inventory capacity rule
            int eggTx = m_chickenSystem.getGroundEggs()[0].tileX;
            int eggTy = m_chickenSystem.getGroundEggs()[0].tileY;
            int groundCountBefore = m_chickenSystem.getGroundEggCount();

            // Test full bag refusal
            Inventory fullInvBackup = m_inventory;
            for (auto& slot : m_inventory.getSlots()) {
                slot.type = ItemType::Wheat;
                slot.count = Config::MAX_STACK_SIZE;
            }
            bool fullCollect = m_chickenSystem.collectEgg(eggTx, eggTy, m_inventory, &m_hud, &m_audio);
            assert(!fullCollect && "Egg collection refused when inventory is full");
            assert(m_chickenSystem.getGroundEggCount() == groundCountBefore && "Egg remains on ground");
            assert(m_hud.getActiveFeedback() == "INVENTORY FULL!");
            m_inventory = fullInvBackup;

            // Successful collection
            m_inventory.clear();
            bool collected = m_chickenSystem.collectEgg(eggTx, eggTy, m_inventory, &m_hud, &m_audio);
            assert(collected && "Collecting egg with free inventory succeeds");
            assert(m_chickenSystem.getGroundEggCount() == groundCountBefore - 1);
            assert(m_inventory.getItemCount(ItemType::Egg) == 1 && "Egg added to inventory");
            assert(m_hud.getActiveFeedback() == "COLLECTED 1 EGG!");

            // 6. Sell egg at exactly 15G
            assert(m_shopSystem.getSellPrice(ItemType::Egg) == 15 && "Buyer price for Egg must be exactly 15G");
            m_inventory.setMoney(100);
            bool soldEgg = m_shopSystem.sellItem(ItemType::Egg, 1, m_inventory, &m_hud, &m_audio);
            assert(soldEgg && "Selling Egg to Buyer NPC succeeds");
            assert(m_inventory.getItemCount(ItemType::Egg) == 0);
            assert(m_inventory.getMoney() == 115 && "Wallet must be exactly 100 + 15 = 115G");

            // 7. Cook Omelette from 2 eggs
            assert(m_shopSystem.getSellPrice(ItemType::Omelette) == 50 && "Buyer price for Omelette must be exactly 50G");
            m_inventory.clear();
            m_inventory.addItem(ItemType::Egg, 1);
            bool cookMissing = m_cookingSystem.cook(ItemType::Omelette, m_inventory, &m_hud);
            assert(!cookMissing && "Cooking Omelette requires 2 eggs, refused with 1");
            m_inventory.addItem(ItemType::Egg, 1); // now 2 eggs
            bool cookOk = m_cookingSystem.cook(ItemType::Omelette, m_inventory, &m_hud);
            assert(cookOk && "Cooking Omelette succeeds with 2 eggs");
            assert(m_inventory.getItemCount(ItemType::Egg) == 0 && "2x eggs consumed");
            assert(m_inventory.getItemCount(ItemType::Omelette) == 1 && "1x Omelette created");

            // Verify eating Omelette restores +40 energy
            m_player.setEnergy(50);
            m_inventoryUI.open();
            m_inventoryUI.setSelectedSlot(0);
            bool ate = eatSelectedItem();
            assert(ate && "Eating Omelette succeeds");
            assert(m_player.getEnergy() == 90 && "Stamina restored by +40 (50 -> 90)");
            assert(m_hud.getActiveFeedback() == "Ate Omelette: +40 Energy");
            m_inventoryUI.close();

            // 8. Save/load restores chickens and eggs
            m_chickenSystem.setChickenCount(4);
            m_chickenSystem.clearGroundEggs();
            m_chickenSystem.addGroundEgg(2, 11);
            m_chickenSystem.addGroundEgg(3, 12);
            m_chickenSystem.addGroundEgg(5, 13);
            bool saveOk = SaveSystem::save(*this, "smoke_chicken_save.json");
            assert(saveOk);
            m_chickenSystem.reset();
            assert(m_chickenSystem.getChickenCount() == 0);
            assert(m_chickenSystem.getGroundEggCount() == 0);
            bool loadOk = SaveSystem::load(*this, "smoke_chicken_save.json");
            assert(loadOk);
            assert(m_chickenSystem.getChickenCount() == 4);
            assert(m_chickenSystem.getGroundEggCount() == 3);
            assert(m_chickenSystem.hasEggAt(2, 11));
            assert(m_chickenSystem.hasEggAt(3, 12));
            assert(m_chickenSystem.hasEggAt(5, 13));
            SaveSystem::removeSave("smoke_chicken_save.json");

            // 9. Chicken wandering containment inside pen
            for (int tick = 0; tick < 60; ++tick) {
                m_chickenSystem.update(0.05f);
            }
            for (const auto& ch : m_chickenSystem.getChickens()) {
                assert(ch.getX() >= 64.0f && ch.getX() <= 160.0f && "Chicken X within pen");
                assert(ch.getY() >= 352.0f && ch.getY() <= 416.0f && "Chicken Y within pen");
            }

            std::cout << "[Smoke Test] Chicken system verified: buy chicken (-100G, count +1), cap refusal at 6 with no charge, forced egg spawn overnight, collect egg with inventory capacity rule, sell egg at 15G, cook & eat Omelette, pen fence containment, and save/load persistence!" << std::endl;
        } else if (m_smokeTestFrames == 157) {
            // ROUND 6 VERIFICATIONS:
            // 1. Title state logic headless (save-exists detection)
            {
                TitleUI title;
                title.init(false);
                assert(title.getOptionCount() == 2 && "Title has 2 options when no save exists (New Game, Quit)");

                // Select option 0: New Game (when no save exists -> starts directly without overwrite prompt)
                title.setSelectedIndex(0);
                Input testInputEnter;
                testInputEnter.setKeyPressed(SDL_SCANCODE_RETURN, true);
                TitleAction actNewGameNoSave = title.handleInput(testInputEnter);
                assert(actNewGameNoSave == TitleAction::NewGame && "New Game triggers immediately without prompt when no save exists");
                assert(!title.isConfirmingOverwrite());

                // Select option 1: Quit
                title.setSelectedIndex(1);
                TitleAction actQuitNoSave = title.handleInput(testInputEnter);
                assert(actQuitNoSave == TitleAction::Quit);

                // Now simulate save existing
                title.refreshSaveExists(true);
                assert(title.getOptionCount() == 3 && "Title has 3 options when save exists (Continue, New Game, Quit)");

                // Select option 0: Continue
                title.setSelectedIndex(0);
                TitleAction actContinue = title.handleInput(testInputEnter);
                assert(actContinue == TitleAction::Continue && "Selecting Continue triggers Continue action");

                // Select option 1: New Game (when save exists -> asks overwrite confirmation)
                title.setSelectedIndex(1);
                TitleAction actNewGameWithSave = title.handleInput(testInputEnter);
                assert(actNewGameWithSave == TitleAction::None && "New Game with save must not launch immediately");
                assert(title.isConfirmingOverwrite() && "New Game with save prompts 'Overwrite save? Enter = yes, Esc = no'");

                // Esc cancels overwrite prompt
                Input testInputEsc;
                testInputEsc.setKeyPressed(SDL_SCANCODE_ESCAPE, true);
                TitleAction actCancel = title.handleInput(testInputEsc);
                assert(actCancel == TitleAction::None && "Esc cancels overwrite prompt");
                assert(!title.isConfirmingOverwrite() && "Prompt closed on Esc");

                // New Game again -> Enter confirms overwrite
                title.setSelectedIndex(1);
                title.handleInput(testInputEnter);
                assert(title.isConfirmingOverwrite());
                TitleAction actConfirmed = title.handleInput(testInputEnter);
                assert(actConfirmed == TitleAction::NewGame && "Enter confirms overwrite and starts New Game");
                assert(!title.isConfirmingOverwrite());

                std::cout << "[Smoke Test] Title screen state logic verified: save-exists detection (3 vs 2 options), New Game overwrite confirmation prompt, and Esc cancellation!" << std::endl;
            }

            // 2. New Game reset zeroes stats
            {
                m_statsSystem.setLifetimeEarnedGold(500);
                m_statsSystem.setCropsHarvested(10);
                m_statsSystem.setFishCaught(4);
                m_statsSystem.setEggsCollected(3);
                m_statsSystem.setDishesCooked(2);
                m_statsSystem.setGoalReached(true);

                resetGame();

                assert(m_statsSystem.getLifetimeEarnedGold() == 0 && "Reset zeroes lifetime_earned_gold");
                assert(m_statsSystem.getCropsHarvested() == 0 && "Reset zeroes crops_harvested");
                assert(m_statsSystem.getFishCaught() == 0 && "Reset zeroes fish_caught");
                assert(m_statsSystem.getEggsCollected() == 0 && "Reset zeroes eggs_collected");
                assert(m_statsSystem.getDishesCooked() == 0 && "Reset zeroes dishes_cooked");
                assert(!m_statsSystem.isGoalReached() && "Reset clears goal_reached");

                std::cout << "[Smoke Test] New Game reset verified: all lifetime counters zeroed and goal cleared!" << std::endl;
            }

            // 3. Each counter increments on its event
            {
                m_statsSystem.reset();

                // (a) Harvest event
                assert(m_statsSystem.getCropsHarvested() == 0);
                m_map.setTileType(5, 5, TileType::WateredSoil);
                m_farmSystem.clearCrops();
                m_farmSystem.addCrop(Crop(5, 5, ItemType::Tomato, CropStage::Harvest));
                m_inventory.clear();
                m_player.setEnergy(50);
                bool harvested = m_farmSystem.harvest(5, 5, m_map, m_inventory, m_player, &m_hud);
                assert(harvested && "Harvest mature crop succeeds");
                assert(m_statsSystem.getCropsHarvested() == 1 && "crops_harvested incremented on harvest event");

                // (b) Fish caught event
                assert(m_statsSystem.getFishCaught() == 0);
                m_fishingSystem.setNextWaitDuration(2.0f);
                m_player.setEnergy(50);
                m_fishingSystem.startFishing(16, 11, m_player, &m_hud, &m_audio);
                m_fishingSystem.update(2.05f, &m_hud);
                assert(m_fishingSystem.getState() == FishingState::Bite);
                bool caught = m_fishingSystem.handleActionInput(m_inventory, &m_hud, &m_audio);
                assert(caught && "Caught fish succeeds");
                assert(m_statsSystem.getFishCaught() == 1 && "fish_caught incremented on catch event");

                // (c) Egg collected event
                assert(m_statsSystem.getEggsCollected() == 0);
                m_chickenSystem.clearGroundEggs();
                m_chickenSystem.addGroundEgg(2, 11);
                bool collected = m_chickenSystem.collectEgg(2, 11, m_inventory, &m_hud, &m_audio);
                assert(collected && "Collected egg succeeds");
                assert(m_statsSystem.getEggsCollected() == 1 && "eggs_collected incremented on egg collect event");

                // (d) Dishes cooked event
                assert(m_statsSystem.getDishesCooked() == 0);
                m_inventory.addItem(ItemType::Tomato, 2);
                bool cooked = m_cookingSystem.cook(ItemType::Salad, m_inventory, &m_hud, &m_audio);
                assert(cooked && "Cooking Salad succeeds");
                assert(m_statsSystem.getDishesCooked() == 1 && "dishes_cooked incremented on cook event");

                // (e) Sell transactions event
                assert(m_statsSystem.getLifetimeEarnedGold() == 0);
                assert(m_inventory.getItemCount(ItemType::Salad) == 1);
                bool sold = m_shopSystem.sellItem(ItemType::Salad, 1, m_inventory, &m_hud, &m_audio);
                assert(sold && "Selling Salad succeeds");
                assert(m_statsSystem.getLifetimeEarnedGold() == 40 && "lifetime_earned_gold incremented by sell income (+40G)");

                std::cout << "[Smoke Test] Event counters verified: harvest (+1 crop), catch (+1 fish), collect (+1 egg), cook (+1 dish), sell (+40G earned)!" << std::endl;
            }

            // 4. Earnings goal fires exactly once at 1000G crossing and persists across save/load
            {
                m_statsSystem.reset();
                m_statsSystem.setLifetimeEarnedGold(950);
                assert(!m_statsSystem.isGoalReached());
                m_hud.showFeedback("", 0.0f);

                // Add 60G -> 1010G (crosses 1000G threshold)
                m_statsSystem.recordSale(60, &m_hud);
                assert(m_statsSystem.getLifetimeEarnedGold() == 1010);
                assert(m_statsSystem.isGoalReached() && "Goal marked reached at crossing");
                assert(m_hud.getActiveFeedback() == "FARM GOAL REACHED: 1000G lifetime earnings!" && "Celebration banner fired on crossing");

                // Earn more gold -> goal does not fire again
                m_hud.showFeedback("DIFFERENT BANNER", 2.0f);
                m_statsSystem.recordSale(50, &m_hud);
                assert(m_statsSystem.getLifetimeEarnedGold() == 1060);
                assert(m_statsSystem.isGoalReached());
                assert(m_hud.getActiveFeedback() != "FARM GOAL REACHED: 1000G lifetime earnings!" && "Goal celebration never fires a second time");

                // Persists across save/load
                const std::string goalSaveFile = "smoke_goal_save.json";
                SaveSystem::save(*this, goalSaveFile);
                m_statsSystem.reset();
                assert(!m_statsSystem.isGoalReached());
                SaveSystem::load(*this, goalSaveFile);
                assert(m_statsSystem.getLifetimeEarnedGold() == 1060);
                assert(m_statsSystem.isGoalReached() && "goal_reached persists across save/load cycle");

                // Further sales after loading still do not re-trigger goal
                m_hud.showFeedback("OTHER BANNER", 2.0f);
                m_statsSystem.recordSale(100, &m_hud);
                assert(m_hud.getActiveFeedback() != "FARM GOAL REACHED: 1000G lifetime earnings!" && "Goal never fires after being loaded as reached");
                SaveSystem::removeSave(goalSaveFile);

                std::cout << "[Smoke Test] Earnings goal verified: one-time celebration banner fired at 1000G crossing, goal_reached marked true, and persists across save/load without re-triggering!" << std::endl;
            }

            // 5. Pause Save Now writes a loadable file
            {
                SaveSystem::removeSave("savegame.json");
                assert(!SaveSystem::exists("savegame.json"));

                pauseSaveNow();

                assert(SaveSystem::exists("savegame.json") && "pauseSaveNow wrote savegame.json");
                assert(m_hud.getActiveFeedback() == "Saved!" && "Saved! feedback banner displayed");

                bool loadOk = SaveSystem::load(*this, "savegame.json");
                assert(loadOk && "File written by Save Now is valid and loadable");
                SaveSystem::removeSave("savegame.json");

                std::cout << "[Smoke Test] Pause Save Now verified: wrote loadable savegame.json with 'Saved!' banner!" << std::endl;
            }

            // 6. Stats survive save/load
            {
                const std::string statsSaveFile = "smoke_stats_save.json";
                LifetimeStats testStats;
                testStats.lifetimeEarnedGold = 1450;
                testStats.cropsHarvested = 18;
                testStats.fishCaught = 9;
                testStats.eggsCollected = 7;
                testStats.dishesCooked = 11;
                testStats.goalReached = true;
                m_statsSystem.setStats(testStats);

                bool saved = SaveSystem::save(*this, statsSaveFile);
                assert(saved && "SaveSystem::save succeeded");

                m_statsSystem.reset();
                assert(m_statsSystem.getLifetimeEarnedGold() == 0);

                bool loaded = SaveSystem::load(*this, statsSaveFile);
                assert(loaded && "SaveSystem::load succeeded");

                assert(m_statsSystem.getLifetimeEarnedGold() == 1450 && "lifetime_earned_gold restored");
                assert(m_statsSystem.getCropsHarvested() == 18 && "crops_harvested restored");
                assert(m_statsSystem.getFishCaught() == 9 && "fish_caught restored");
                assert(m_statsSystem.getEggsCollected() == 7 && "eggs_collected restored");
                assert(m_statsSystem.getDishesCooked() == 11 && "dishes_cooked restored");
                assert(m_statsSystem.isGoalReached() == true && "goal_reached restored");

                SaveSystem::removeSave(statsSaveFile);
                std::cout << "[Smoke Test] Stats save/load survival verified: all 5 counters and goal flag restored exactly!" << std::endl;
            }
        }
    }
}

void Game::initRainParticles() {
    m_rainParticles.resize(80);
    for (size_t i = 0; i < m_rainParticles.size(); ++i) {
        m_rainParticles[i].x = static_cast<float>((i * 17 + 23) % (Config::SCREEN_WIDTH + 60) - 20);
        m_rainParticles[i].y = static_cast<float>((i * 29 + 11) % Config::SCREEN_HEIGHT);
        m_rainParticles[i].speed = 280.0f + static_cast<float>((i * 37) % 120);
        m_rainParticles[i].length = 8.0f + static_cast<float>((i * 13) % 8);
    }
}

void Game::updateRainParticles(float dt) {
    for (auto& p : m_rainParticles) {
        p.y += p.speed * dt;
        p.x -= (p.speed * 0.25f) * dt;
        if (p.y > static_cast<float>(Config::SCREEN_HEIGHT)) {
            p.y = -p.length;
            p.x += 120.0f;
            if (p.x > static_cast<float>(Config::SCREEN_WIDTH + 20)) {
                p.x -= static_cast<float>(Config::SCREEN_WIDTH + 40);
            }
        }
        if (p.x < -20.0f) {
            p.x += static_cast<float>(Config::SCREEN_WIDTH + 40);
        }
    }
}

void Game::renderRainParticles() {
    for (const auto& p : m_rainParticles) {
        int x1 = static_cast<int>(p.x);
        int y1 = static_cast<int>(p.y);
        int x2 = x1 - 2;
        int y2 = y1 + static_cast<int>(p.length);
        m_renderer.drawLine(x1, y1, x2, y2, SDL_Color{160, 205, 245, 180});
    }
}

void Game::render() {
    // Clear screen with a neutral dark tone
    m_renderer.clear(SDL_Color{20, 20, 20, 255});

    if (m_gameState == GameState::Title) {
        // Fill full screen background with grass tiles
        for (int gy = 0; gy < Config::SCREEN_HEIGHT; gy += Map::TILE_SIZE) {
            for (int gx = 0; gx < Config::SCREEN_WIDTH; gx += Map::TILE_SIZE) {
                if (!m_renderer.drawSprite("tile_grass_0.png", gx, gy, Map::TILE_SIZE, Map::TILE_SIZE)) {
                    m_renderer.drawRect(gx, gy, Map::TILE_SIZE, Map::TILE_SIZE, SDL_Color{85, 140, 50, 255});
                }
            }
        }
        // Render terrain tile map backdrop centered
        m_renderer.setOffset(WORLD_OFFSET_X, WORLD_OFFSET_Y);
        m_map.render(m_renderer);
        m_renderer.setOffset(0, 0);

        // Dimmed backdrop overlay
        m_renderer.drawRect(0, 0, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, SDL_Color{0, 0, 0, 160});
        // Title UI panel and menu
        m_titleUI.render(m_renderer);
        m_renderer.present();
        return;
    }

    if (m_location == Location::Farm) {
        // Fill full screen with lush grass tiles
        for (int gy = 0; gy < Config::SCREEN_HEIGHT; gy += Map::TILE_SIZE) {
            for (int gx = 0; gx < Config::SCREEN_WIDTH; gx += Map::TILE_SIZE) {
                if (!m_renderer.drawSprite("tile_grass_0.png", gx, gy, Map::TILE_SIZE, Map::TILE_SIZE)) {
                    m_renderer.drawRect(gx, gy, Map::TILE_SIZE, Map::TILE_SIZE, SDL_Color{85, 140, 50, 255});
                }
            }
        }

        m_renderer.setOffset(WORLD_OFFSET_X, WORLD_OFFSET_Y);

        // 1. Render terrain tile map
        m_map.render(m_renderer);

        // 2. Render planted crops and fishing effects
        m_farmSystem.render(m_renderer);
        m_fishingSystem.render(m_renderer);

        // 3. Render merchant NPCs (Seller and Buyer) and chicken livestock
        m_sellerNPC.render(m_renderer);
        m_buyerNPC.render(m_renderer);
        m_chickenSystem.render(m_renderer);
    } else {
        // Interior dark backdrop
        m_renderer.drawRect(0, 0, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, SDL_Color{15, 12, 10, 255});
        m_renderer.setOffset(WORLD_OFFSET_X, WORLD_OFFSET_Y);
        // Render Farmhouse interior room
        m_interiorMap.render(m_renderer);
    }

    // 4. Render farmer player
    m_player.render(m_renderer);

    // World interaction prompts (dynamic contextual prompt based on held tool and target tile)
    if (!m_inventoryUI.isOpen() && !m_cookingUI.isOpen() && !m_shopUI.isOpen()) {
        int playerTx = static_cast<int>(std::round(m_player.getX() / static_cast<float>(Map::TILE_SIZE)));
        int playerTy = static_cast<int>(std::round(m_player.getY() / static_cast<float>(Map::TILE_SIZE)));

        // Check if mouse hovers a tile within reach (dist <= 2)
        int worldMx = m_input.getMouseX() - WORLD_OFFSET_X;
        int worldMy = m_input.getMouseY() - WORLD_OFFSET_Y;
        int hoverTx = worldMx / Map::TILE_SIZE;
        int hoverTy = worldMy / Map::TILE_SIZE;

        Map& activeMap = (m_location == Location::Farm) ? m_map : m_interiorMap;
        int targetTx = -1, targetTy = -1;

        if (worldMx >= 0 && worldMy >= 0 && activeMap.inBounds(hoverTx, hoverTy)) {
            int hoverDist = std::max(std::abs(hoverTx - playerTx), std::abs(hoverTy - playerTy));
            if (hoverDist <= 2) {
                targetTx = hoverTx;
                targetTy = hoverTy;
            }
        }

        if (targetTx < 0 || targetTy < 0) {
            auto [ptx, pty] = m_player.getTargetTile();
            if (activeMap.inBounds(ptx, pty)) {
                targetTx = ptx;
                targetTy = pty;
            }
        }

        if (targetTx >= 0 && targetTy >= 0) {
            std::string promptText = "";
            const char* iconSprite = nullptr;
            bool showPrompt = false;

            if (m_location == Location::FarmhouseInterior) {
                TileType type = m_interiorMap.getTile(targetTx, targetTy).getType();
                if (type == TileType::Bed) {
                    promptText = "[E] SLEEP";
                    iconSprite = "ui_sleep_icon.png";
                    showPrompt = true;
                } else if (type == TileType::Stove) {
                    promptText = "[E] COOK";
                    iconSprite = "ui_cooking_icon.png";
                    showPrompt = true;
                } else if (type == TileType::InteriorExit) {
                    promptText = "[E] EXIT";
                    showPrompt = true;
                } else if (type == TileType::Chest) {
                    promptText = "[E] CHEST";
                    showPrompt = true;
                } else if (type == TileType::Bookshelf) {
                    promptText = "[E] READ";
                    showPrompt = true;
                }
            } else {
                // Location::Farm
                // 1. Seller NPC
                if ((targetTx == m_sellerNPC.getTileX() && targetTy == m_sellerNPC.getTileY()) ||
                    (targetTx >= 13 && targetTx <= 15 && targetTy == 1)) {
                    promptText = "[E] SHOP";
                    showPrompt = true;
                }
                // 2. Buyer NPC
                else if ((targetTx == m_buyerNPC.getTileX() && targetTy == m_buyerNPC.getTileY()) ||
                         (targetTx >= 16 && targetTx <= 18 && targetTy == 1)) {
                    promptText = "[E] SELL";
                    showPrompt = true;
                }
                // 3. Farmhouse entrance door
                else if (m_map.getTile(targetTx, targetTy).getType() == TileType::FarmhouseDoor || (targetTx == 2 && targetTy == 2)) {
                    promptText = "[E] ENTER";
                    showPrompt = true;
                }
                // 4. Farmhouse bed / sleep
                else if (m_map.getTile(targetTx, targetTy).getType() == TileType::Farmhouse || TimeSystem::isBedTile(targetTx, targetTy)) {
                    promptText = "[E] SLEEP";
                    iconSprite = "ui_sleep_icon.png";
                    showPrompt = true;
                }
                // 5. Outdoor Stove
                else if (m_map.getTile(targetTx, targetTy).getType() == TileType::Stove) {
                    promptText = "[E] COOK";
                    iconSprite = "ui_cooking_icon.png";
                    showPrompt = true;
                }
                // 6. Chicken Egg
                else if (m_chickenSystem.hasEggAt(targetTx, targetTy) || m_chickenSystem.hasEggAt(playerTx, playerTy)) {
                    if (m_chickenSystem.hasEggAt(playerTx, playerTy) && !m_chickenSystem.hasEggAt(targetTx, targetTy)) {
                        targetTx = playerTx;
                        targetTy = playerTy;
                    }
                    promptText = "[E] EGG";
                    showPrompt = true;
                }
                // 7. Water tile (Pond / Lake)
                else if (m_map.getTile(targetTx, targetTy).getType() == TileType::Water) {
                    if (m_inventory.activeTool == ToolType::WateringCan) {
                        promptText = "[E] FILL WATER";
                        iconSprite = "tool_watering_can.png";
                        showPrompt = true;
                    } else if (!m_fishingSystem.isFishing()) {
                        promptText = "[E] FISH";
                        showPrompt = true;
                    }
                }
                // 8. Ground / Farming Soil tiles
                else {
                    Crop* crop = m_farmSystem.findCrop(targetTx, targetTy);
                    if (crop) {
                        if (crop->isMature()) {
                            promptText = "[E] HARVEST";
                            showPrompt = true;
                        } else if (m_inventory.activeTool == ToolType::WateringCan &&
                                   m_map.getTile(targetTx, targetTy).getType() == TileType::TilledSoil) {
                            promptText = "[E] WATER";
                            iconSprite = "tool_watering_can.png";
                            showPrompt = true;
                        }
                    } else {
                        TileType t = m_map.getTile(targetTx, targetTy).getType();
                        if (t == TileType::Dirt) {
                            // "ô đất chưa bới thì nếu player ko cầm gì thì ko hiện ui đào , neeuus cầm cuốc thì sẽ hiện đào"
                            if (m_inventory.activeTool == ToolType::Hoe) {
                                promptText = "[E] TILL";
                                iconSprite = "tool_hoe.png";
                                showPrompt = true;
                            }
                        } else if (t == TileType::TilledSoil) {
                            // "tiếp theo như watering can nếu đất đã xởi thì tưới dc , chưa xới thì ko"
                            if (m_inventory.activeTool == ToolType::WateringCan) {
                                promptText = "[E] WATER";
                                iconSprite = "tool_watering_can.png";
                                showPrompt = true;
                            } else if (m_inventory.activeTool == ToolType::TomatoSeeds ||
                                       m_inventory.activeTool == ToolType::WheatSeeds ||
                                       m_inventory.activeTool == ToolType::CarrotSeeds) {
                                promptText = "[E] PLANT";
                                iconSprite = "item_seed_packet.png";
                                showPrompt = true;
                            }
                        } else if (t == TileType::WateredSoil) {
                            if (m_inventory.activeTool == ToolType::TomatoSeeds ||
                                m_inventory.activeTool == ToolType::WheatSeeds ||
                                m_inventory.activeTool == ToolType::CarrotSeeds) {
                                promptText = "[E] PLANT";
                                iconSprite = "item_seed_packet.png";
                                showPrompt = true;
                            }
                        }
                    }
                }
            }

            if (showPrompt) {
                int tw = BitmapFont::getTextWidth(promptText, 1);
                int promptW = tw + (iconSprite ? 30 : 16);
                int promptH = 22;
                int promptX = targetTx * Map::TILE_SIZE + (Map::TILE_SIZE - promptW) / 2;
                int promptY = targetTy * Map::TILE_SIZE - 24;
                if (promptY < 4) promptY = targetTy * Map::TILE_SIZE + Map::TILE_SIZE + 4;

                bool hasDialogue = m_renderer.drawDialogue(promptX, promptY, promptW, promptH, true);
                if (!hasDialogue) {
                    m_renderer.drawRect(promptX, promptY, promptW, promptH, SDL_Color{35, 27, 22, 240});
                    m_renderer.drawOutline(promptX, promptY, promptW, promptH, SDL_Color{195, 160, 105, 255});
                }
                int textX = promptX + 8;
                if (iconSprite) {
                    m_renderer.drawSprite(iconSprite, promptX + 4, promptY + 3, 16, 16);
                    textX = promptX + 22;
                } else {
                    textX = promptX + (promptW - tw) / 2;
                }
                SDL_Color textCol = hasDialogue ? SDL_Color{60, 36, 22, 255} : SDL_Color{255, 230, 130, 255};
                BitmapFont::drawText(m_renderer, promptText, textX, promptY + 7, textCol, 1);
            }
        }
    }

    // Reset renderer offset for screen-space HUD and UI overlays
    m_renderer.setOffset(0, 0);

    // Rain particle overlay outdoors on rainy days (indoor/interior unaffected)
    if (m_location == Location::Farm && m_timeSystem.getWeather() == Weather::Rainy) {
        renderRainParticles();
    }

    // 5. Render HUD with Day Counter & Weather Indicator
    m_hud.render(m_renderer, m_inventory, m_player, m_timeSystem.getDay(), m_timeSystem.getWeather());

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

    // 9. Render Pause Menu overlay and Stats View modal
    if (m_pauseUI.isOpen()) {
        m_pauseUI.render(m_renderer);
        m_hud.renderFeedbackBanner(m_renderer);
    }
    if (m_statsUI.isOpen()) {
        m_statsUI.render(m_renderer, m_timeSystem.getDay(), m_inventory.getMoney(), m_statsSystem, m_chickenSystem.getChickenCount());
    }

    // 10. Screen transition fade overlay (through black)
    if (m_fadePhase == FadePhase::FadeOut) {
        float progress = std::clamp(m_fadeTimer / FADE_OUT_DURATION, 0.0f, 1.0f);
        Uint8 alpha = static_cast<Uint8>(std::round(progress * 255.0f));
        m_renderer.drawRect(0, 0, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, SDL_Color{0, 0, 0, alpha});
    } else if (m_fadePhase == FadePhase::FadeIn) {
        float progress = std::clamp(m_fadeTimer / FADE_IN_DURATION, 0.0f, 1.0f);
        Uint8 alpha = static_cast<Uint8>(std::round((1.0f - progress) * 255.0f));
        m_renderer.drawRect(0, 0, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, SDL_Color{0, 0, 0, alpha});
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

        // Verify lifetime earnings equals sum of run's sales (40 + 50 + 20 + 14 = 124G)
        assert(m_statsSystem.getLifetimeEarnedGold() == 124 && "Lifetime earned gold must equal sum of Step 8 sales (124G)");

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
    } else if (m_playtestFrames == 155) {
        // Step 15: Weather simulation & Rainy day watering verification (Part C)
        // 1. Plant a Carrot seed on tilled soil at (5, 5) on Day 5 (Sunny)
        m_map.setTileType(5, 5, TileType::TilledSoil);
        m_inventory.activeTool = ToolType::CarrotSeeds;
        m_player.placeAtTileCenter(5, 6);
        m_player.setFacing(Direction::Up);
        bool planted = m_farmSystem.interact(5, 5, m_map, m_inventory, m_player, &m_hud);
        assert(planted && "Carrot seeds planted at (5, 5)");
        assert(m_map.getTile(5, 5).getType() == TileType::TilledSoil && "Soil must be unwatered tilled soil initially");

        // 2. Sleep overnight from Day 5 to Day 6 (Day 6 rolls deterministically as Rainy)
        m_timeSystem.sleep(m_player, m_farmSystem, m_map, &m_hud);
        assert(m_timeSystem.getDay() == 6 && "Day must advance to 6");
        assert(m_timeSystem.getWeather() == Weather::Rainy && "Day 6 must deterministically roll Weather::Rainy");

        // 3. Verify outdoor planted crop at (5, 5) counts as watered that day without manual watering
        assert(m_map.getTile(5, 5).getType() == TileType::WateredSoil && "Planted crop soil must show WateredSoil due to rainfall");
        assert(m_map.getTile(5, 5).isWatered());

        // 4. Verify 2.5x growth boost applies to rained crop without manual watering
        Crop* carrotCrop = m_farmSystem.findCrop(5, 5);
        assert(carrotCrop != nullptr && "Carrot crop exists at (5, 5)");
        carrotCrop->setStageTimer(0.0f);
        m_farmSystem.update(1.0f, m_map, m_timeSystem.getWeather());
        assert(std::abs(carrotCrop->getStageTimer() - 2.5f) < 0.001f && "Rained crop must receive 2.5x growth boost without manual watering");

        // 5. Sleep overnight from Day 6 (Rainy) to Day 7 (Sunny)
        m_timeSystem.sleep(m_player, m_farmSystem, m_map, &m_hud);
        assert(m_timeSystem.getDay() == 7 && "Day must advance to 7");
        assert(m_timeSystem.getWeather() == Weather::Sunny && "Day 7 must deterministically roll Weather::Sunny");
        assert(m_map.getTile(5, 5).getType() == TileType::TilledSoil && "Soil must dry back to TilledSoil on sunny day");

        // 6. Test save/load persistence of weather
        m_timeSystem.setDay(6);
        m_timeSystem.setWeather(Weather::Rainy);
        saveGame();
        m_timeSystem.setWeather(Weather::Sunny);
        assert(m_timeSystem.getWeather() == Weather::Sunny);
        loadGame();
        assert(m_timeSystem.getWeather() == Weather::Rainy && "Weather::Rainy must be restored on loadGame()");
        assert(m_timeSystem.getDay() == 6);

        std::cout << "[PASS] Step 15: Weather verified across days: Day 6 Rainy auto-waters crops with 2.5x boost, Day 7 Sunny dries soil, and weather persists in save file!" << std::endl;
    } else if (m_playtestFrames == 165) {
        // Step 16: Pond fishing, cooking Grilled Fish at stove, and selling both to Buyer NPC
        assert(m_inventory.getMoney() == 89 && "Wallet starts at 89G before Step 16");
        assert(m_location == Location::Farm);

        // 1. Move to pond bank at (15, 12) facing Right toward pond water tile (16, 12)
        m_player.placeAtTileCenter(15, 12);
        m_player.setFacing(Direction::Right);
        auto [tx, ty] = m_player.getTargetTile();
        assert(tx == 16 && ty == 12 && m_map.getTile(tx, ty).getType() == TileType::Water);

        // 2. Catch fish #1 using deterministic test injection
        m_fishingSystem.setNextWaitDuration(2.0f);
        bool start1 = m_fishingSystem.startFishing(tx, ty, m_player, &m_hud, &m_audio);
        assert(start1 && "First cast at pond succeeds");
        m_fishingSystem.update(2.05f, &m_hud);
        assert(m_fishingSystem.getState() == FishingState::Bite && "Bite window open");
        bool catch1 = m_fishingSystem.handleActionInput(m_inventory, &m_hud, &m_audio);
        assert(catch1 && "Catching first fish succeeds");
        assert(m_inventory.getItemCount(ItemType::Fish) == 1 && "1x Fish in inventory");

        // 3. Catch fish #2 so we have one to cook and one to sell
        m_fishingSystem.setNextWaitDuration(2.0f);
        bool start2 = m_fishingSystem.startFishing(tx, ty, m_player, &m_hud, &m_audio);
        assert(start2 && "Second cast at pond succeeds");
        m_fishingSystem.update(2.05f, &m_hud);
        assert(m_fishingSystem.getState() == FishingState::Bite && "Bite window open");
        bool catch2 = m_fishingSystem.handleActionInput(m_inventory, &m_hud, &m_audio);
        assert(catch2 && "Catching second fish succeeds");
        assert(m_inventory.getItemCount(ItemType::Fish) == 2 && "2x Fish in inventory");

        // 4. Cook 1x Grilled Fish at the stove (consumes 1x Fish, adds 1x Grilled Fish)
        const Recipe* r = m_cookingSystem.findRecipeByDish(ItemType::GrilledFish);
        assert(r != nullptr && "Grilled Fish recipe must be registered in cooking system");
        bool cooked = m_cookingSystem.cook(ItemType::GrilledFish, m_inventory, &m_hud, &m_audio);
        assert(cooked && "Cooking Grilled Fish at stove succeeds");
        assert(m_inventory.getItemCount(ItemType::Fish) == 1 && "1x Fish remains in inventory");
        assert(m_inventory.getItemCount(ItemType::GrilledFish) == 1 && "1x Grilled Fish now in inventory");

        // 5. Sell 1x Fish (30G) and 1x Grilled Fish (55G) to the Buyer NPC with exact wallet assertions
        assert(m_shopSystem.getSellPrice(ItemType::Fish) == 30 && "Buyer buys Fish for 30G per prices.txt");
        assert(m_shopSystem.getSellPrice(ItemType::GrilledFish) == 55 && "Buyer buys Grilled Fish for 55G per prices.txt");

        // Sell 1x Fish: 89G + 30G = 119G
        bool soldFish = m_shopSystem.sellItem(ItemType::Fish, 1, m_inventory, &m_hud, &m_audio);
        assert(soldFish && "Selling 1x Fish to Buyer succeeds");
        assert(m_inventory.getItemCount(ItemType::Fish) == 0 && "Fish consumed from inventory");
        assert(m_inventory.getMoney() == 119 && "Wallet must be exactly 89 + 30 = 119G");

        // Sell 1x Grilled Fish: 119G + 55G = 174G
        bool soldGrilled = m_shopSystem.sellItem(ItemType::GrilledFish, 1, m_inventory, &m_hud, &m_audio);
        assert(soldGrilled && "Selling 1x Grilled Fish to Buyer succeeds");
        assert(m_inventory.getItemCount(ItemType::GrilledFish) == 0 && "Grilled Fish consumed from inventory");
        assert(m_inventory.getMoney() == 174 && "Wallet must be exactly 119 + 55 = 174G");

        // Verify lifetime earnings equals sum of run's sales (124 + 30 + 55 = 209G)
        assert(m_statsSystem.getLifetimeEarnedGold() == 209 && "Lifetime earned gold must equal sum of Step 8 and Step 16 sales (209G)");

        std::cout << "[PASS] Step 16: Caught 2x Fish, cooked 1x Grilled Fish, and sold Fish (30G) & Grilled Fish (55G) to Buyer (Wallet: 174G)" << std::endl;
    } else if (m_playtestFrames == 170) {
        // Step 17: Buy Chicken from Seller, sleep overnight, collect egg from pen, and sell to Buyer NPC
        assert(m_inventory.getMoney() == 174 && "Wallet must be exactly 174G after Step 16");
        assert(m_chickenSystem.getChickenCount() == 0 && "Chicken count starts at 0");
        assert(m_chickenSystem.getGroundEggCount() == 0 && "Ground eggs start at 0");

        // 1. Walk to Seller NPC and buy 1x Chicken for 100G
        m_player.placeAtTileCenter(14, 3);
        m_player.setFacing(Direction::Up);
        assert(m_shopSystem.getBuyPrice(ItemType::Chicken) == 100 && "Chicken buy price must be 100G per prices.txt");

        bool boughtChicken = m_shopSystem.buyItem(ItemType::Chicken, 1, m_inventory, &m_hud, &m_audio);
        assert(boughtChicken && "Purchasing Chicken from Seller must succeed");
        assert(m_chickenSystem.getChickenCount() == 1 && "Farm chicken count increments to 1");
        assert(m_inventory.getItemCount(ItemType::Chicken) == 0 && "Chicken does not enter player bag slots");
        assert(m_inventory.getMoney() == 74 && "Wallet must be exactly 174 - 100 = 74G");

        // 2. Sleep overnight from Day 6 to Day 7 with forced egg spawn
        m_chickenSystem.forceNextEggSpawn(true);
        m_timeSystem.sleep(m_player, m_farmSystem, m_map, &m_hud);
        assert(m_timeSystem.getDay() == 7 && "Day counter advances to 7");
        assert(m_chickenSystem.getGroundEggCount() == 1 && "Chicken produced 1 egg overnight in the pen");

        // 3. Move into pen and collect egg from its tile
        const auto& eggs = m_chickenSystem.getGroundEggs();
        assert(!eggs.empty() && "Ground egg list must not be empty");
        int eggTx = eggs[0].tileX;
        int eggTy = eggs[0].tileY;
        assert(eggTx >= ChickenSystem::PEN_MIN_TILE_X && eggTx <= ChickenSystem::PEN_MAX_TILE_X);
        assert(eggTy >= ChickenSystem::PEN_MIN_TILE_Y && eggTy <= ChickenSystem::PEN_MAX_TILE_Y);

        m_player.placeAtTileCenter(eggTx, eggTy);
        bool collected = m_chickenSystem.collectEgg(eggTx, eggTy, m_inventory, &m_hud, &m_audio);
        assert(collected && "Egg collection from pen tile succeeds");
        assert(m_chickenSystem.getGroundEggCount() == 0 && "Egg removed from ground");
        assert(m_inventory.getItemCount(ItemType::Egg) == 1 && "1x Egg added to player inventory");

        // 4. Move to Buyer NPC and sell Egg for exactly 15G
        m_player.placeAtTileCenter(17, 3);
        m_player.setFacing(Direction::Up);
        assert(m_shopSystem.getSellPrice(ItemType::Egg) == 15 && "Buyer purchases Egg for exactly 15G per prices.txt");

        bool soldEgg = m_shopSystem.sellItem(ItemType::Egg, 1, m_inventory, &m_hud, &m_audio);
        assert(soldEgg && "Selling Egg to Buyer succeeds");
        assert(m_inventory.getItemCount(ItemType::Egg) == 0 && "Egg consumed from inventory");
        assert(m_inventory.getMoney() == 89 && "Wallet must be exactly 74 + 15 = 89G");

        // Verify lifetime earnings equals sum of all run's sales (209 + 15 = 224G)
        assert(m_statsSystem.getLifetimeEarnedGold() == 224 && "Lifetime earned gold must equal total playthrough sales (224G)");

        std::cout << "[PASS] Step 17: Bought 1x Chicken (-100G), forced overnight egg, collected from pen, and sold Egg (+15G) to Buyer (Wallet: 89G, Chickens: 1, Day: 7)" << std::endl;
    } else if (m_playtestFrames >= 180) {
        std::cout << "[PASS] Full playthrough playtest and farmhouse interior verifications passed cleanly across "
                  << m_timeSystem.getDay() << " in-game days!" << std::endl;
        m_isRunning = false;
    }
}
