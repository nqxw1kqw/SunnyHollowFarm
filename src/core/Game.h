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
#include "systems/AudioSystem.h"
#include "systems/FishingSystem.h"
#include "systems/ChickenSystem.h"
#include "systems/StatsSystem.h"
#include "ui/StatsUI.h"
#include "ui/PauseUI.h"
#include "ui/TitleUI.h"

enum class GameState {
    Title,
    Playing
};

enum class Location {
    Farm,
    FarmhouseInterior
};

enum class FadePhase {
    None,
    FadeOut,
    FadeIn
};

enum class TransitionAction {
    None,
    EnterHouse,
    ExitHouse,
    Sleep
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
    const AudioSystem& getAudioSystem() const { return m_audio; }
    AudioSystem& getAudioSystem() { return m_audio; }
    const FishingSystem& getFishingSystem() const { return m_fishingSystem; }
    FishingSystem& getFishingSystem() { return m_fishingSystem; }
    const ChickenSystem& getChickenSystem() const { return m_chickenSystem; }
    ChickenSystem& getChickenSystem() { return m_chickenSystem; }
    const StatsSystem& getStatsSystem() const { return m_statsSystem; }
    StatsSystem& getStatsSystem() { return m_statsSystem; }
    const StatsUI& getStatsUI() const { return m_statsUI; }
    StatsUI& getStatsUI() { return m_statsUI; }
    const PauseUI& getPauseUI() const { return m_pauseUI; }
    PauseUI& getPauseUI() { return m_pauseUI; }
    const TitleUI& getTitleUI() const { return m_titleUI; }
    TitleUI& getTitleUI() { return m_titleUI; }
    GameState getGameState() const { return m_gameState; }
    void setGameState(GameState state) { m_gameState = state; }

    Location getLocation() const { return m_location; }
    void setLocation(Location loc) { m_location = loc; }
    const Map& getMap() const { return m_map; }
    Map& getMap() { return m_map; }
    const Map& getInteriorMap() const { return m_interiorMap; }
    Map& getInteriorMap() { return m_interiorMap; }
    const Map& getCurrentMap() const { return (m_location == Location::Farm) ? m_map : m_interiorMap; }
    Map& getCurrentMap() { return (m_location == Location::Farm) ? m_map : m_interiorMap; }
    const FarmSystem& getFarmSystem() const { return m_farmSystem; }
    FarmSystem& getFarmSystem() { return m_farmSystem; }
    void transitionTo(Location loc);

    // Save / Load and In-Game Reset (Part C)
    bool saveGame(const std::string& filepath = "savegame.json") const;
    bool loadGame(const std::string& filepath = "savegame.json");
    void resetGame();
    void pauseSaveNow();

    // Part B: Eat food for energy (Salad +25, Bread +35)
    bool eatSelectedItem();

    // Pond fishing interaction
    bool tryStartFishing();
    bool refillWateringCan();
    bool interactAt(int tx, int ty);

    // Centered world coordinates in 1600x900 viewport
    static constexpr int WORLD_OFFSET_X = (Config::SCREEN_WIDTH - Map::WIDTH * Map::TILE_SIZE) / 2;
    static constexpr int WORLD_OFFSET_Y = (Config::SCREEN_HEIGHT - Map::HEIGHT * Map::TILE_SIZE) / 2;

    // Screen transition fade state machine (~300 ms out + 300 ms in)
    static constexpr float FADE_OUT_DURATION = 0.3f;
    static constexpr float FADE_IN_DURATION = 0.3f;

    void startTransition(TransitionAction action);
    void updateTransition(float dt);
    void executeTransitionMidpoint();
    void fastForwardTransition();
    bool isTransitioning() const { return m_fadePhase != FadePhase::None; }
    bool isInputLocked() const { return m_fadePhase != FadePhase::None; }
    FadePhase getFadePhase() const { return m_fadePhase; }
    TransitionAction getTransitionAction() const { return m_transitionAction; }
    float getFadeTimer() const { return m_fadeTimer; }

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
    AudioSystem m_audio;
    HUD m_hud;
    InventoryUI m_inventoryUI;
    CookingUI m_cookingUI;
    ShopUI m_shopUI;
    FishingSystem m_fishingSystem;
    ChickenSystem m_chickenSystem;
    StatsSystem m_statsSystem;
    StatsUI m_statsUI;
    PauseUI m_pauseUI;
    TitleUI m_titleUI;
    GameState m_gameState = GameState::Title;

    bool m_isRunning = false;
    bool m_smokeTestMode = false;
    int m_smokeTestFrames = 0;
    bool m_playtestMode = false;
    int m_playtestFrames = 0;

    FadePhase m_fadePhase = FadePhase::None;
    TransitionAction m_transitionAction = TransitionAction::None;
    float m_fadeTimer = 0.0f;

    // Outdoor rain particle overlay
    struct RainParticle {
        float x = 0.0f;
        float y = 0.0f;
        float speed = 280.0f;
        float length = 10.0f;
    };
    std::vector<RainParticle> m_rainParticles;
    void initRainParticles();
    void updateRainParticles(float dt);
    void renderRainParticles();

    // Tactile action particle effects (Stardew Valley juiciness)
    struct WorldParticle {
        float x = 0.0f;
        float y = 0.0f;
        float vx = 0.0f;
        float vy = 0.0f;
        float life = 0.0f;
        float maxLife = 0.4f;
        SDL_Color color{200, 200, 200, 255};
        int size = 3;
    };
    std::vector<WorldParticle> m_worldParticles;

    struct FloatingText {
        float x = 0.0f;
        float y = 0.0f;
        float vy = -35.0f;
        float life = 0.0f;
        float maxLife = 1.2f;
        std::string text;
        SDL_Color color{255, 230, 80, 255};
    };
    std::vector<FloatingText> m_floatingTexts;

    void spawnDirtParticles(float wx, float wy);
    void spawnWaterParticles(float wx, float wy);
    void spawnSparkleParticles(float wx, float wy);
    void spawnFloatingText(float wx, float wy, const std::string& text, SDL_Color color = SDL_Color{255, 230, 80, 255});
    void updateEffects(float dt);
    void renderWorldEffects();
};
