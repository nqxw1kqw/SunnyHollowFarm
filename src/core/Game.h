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
#include "ui/InventoryUI.h"
#include "systems/CookingSystem.h"
#include "ui/CookingUI.h"
#include "entities/NPC.h"
#include "systems/ShopSystem.h"
#include "ui/ShopUI.h"
#include "systems/TimeSystem.h"

enum class Location {
    Farm,
    FarmhouseInterior
};

// Top-level application class orchestrating game setup, fixed-timestep loop, and teardown.
class Game {
public:
    Game() = default;
    ~Game();

    // Initializes SDL, window, renderer, and subsystems.
    bool init(const char* title, int width, int height, bool smokeTestMode = false, bool playtestMode = false);

    // Runs the main fixed-timestep game loop until quit is requested.
    void run();

    // Cleans up resources on shutdown.
    void cleanUp();

    bool isRunning() const { return m_isRunning; }
    const Inventory& getInventory() const { return m_inventory; }
    Inventory& getInventory() { return m_inventory; }
    const Player& getPlayer() const { return m_player; }
    Player& getPlayer() { return m_player; }
    const HUD& getHUD() const { return m_hud; }
    HUD& getHUD() { return m_hud; }
    const InventoryUI& getInventoryUI() const { return m_inventoryUI; }
    InventoryUI& getInventoryUI() { return m_inventoryUI; }
    const CookingSystem& getCookingSystem() const { return m_cookingSystem; }
    CookingSystem& getCookingSystem() { return m_cookingSystem; }
    const CookingUI& getCookingUI() const { return m_cookingUI; }
    CookingUI& getCookingUI() { return m_cookingUI; }
    const ShopSystem& getShopSystem() const { return m_shopSystem; }
    ShopSystem& getShopSystem() { return m_shopSystem; }
    const ShopUI& getShopUI() const { return m_shopUI; }
    ShopUI& getShopUI() { return m_shopUI; }
    const NPC& getSellerNPC() const { return m_sellerNPC; }
    const NPC& getBuyerNPC() const { return m_buyerNPC; }
    const TimeSystem& getTimeSystem() const { return m_timeSystem; }
    TimeSystem& getTimeSystem() { return m_timeSystem; }

    Location getLocation() const { return m_location; }
    void setLocation(Location loc) { m_location = loc; }
    const Map& getInteriorMap() const { return m_interiorMap; }
    Map& getInteriorMap() { return m_interiorMap; }
    const Map& getCurrentMap() const { return (m_location == Location::Farm) ? m_map : m_interiorMap; }
    Map& getCurrentMap() { return (m_location == Location::Farm) ? m_map : m_interiorMap; }
    void transitionTo(Location loc);

private:
    void handleInput();
    void update(float dt);
    void updatePlaytest(float dt);
    void render();

    SDL_Window* m_window = nullptr;
    Renderer m_renderer;
    AssetManager m_assets;
    Time m_time;
    Input m_input;

    Location m_location = Location::Farm;
    Map m_map;
    Map m_interiorMap;
    Player m_player;
    Inventory m_inventory;
    FarmSystem m_farmSystem;
    CookingSystem m_cookingSystem;
    ShopSystem m_shopSystem;
    NPC m_sellerNPC{14, 2, NPCType::Seller, "Bram the Merchant"};
    NPC m_buyerNPC{17, 2, NPCType::Buyer, "Silas the Produce Buyer"};
    TimeSystem m_timeSystem;
    HUD m_hud;
    InventoryUI m_inventoryUI;
    CookingUI m_cookingUI;
    ShopUI m_shopUI;

    bool m_isRunning = false;
    bool m_smokeTestMode = false;
    int m_smokeTestFrames = 0;
    bool m_playtestMode = false;
    int m_playtestFrames = 0;
};
