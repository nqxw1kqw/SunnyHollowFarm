#pragma once

#include <vector>
#include <random>
#include <functional>
#include "entities/Chicken.h"
#include "entities/Inventory.h"
#include "ui/HUD.h"
#include "systems/AudioSystem.h"

// Represents an egg lying on the ground in the chicken pen
struct GroundEgg {
    int tileX = 0;
    int tileY = 0;
};

class ChickenSystem {
public:
    static constexpr int PEN_MIN_TILE_X = 2;
    static constexpr int PEN_MAX_TILE_X = 5;
    static constexpr int PEN_MIN_TILE_Y = 11;
    static constexpr int PEN_MAX_TILE_Y = 13;
    static constexpr int MAX_CHICKENS = 6;
    static constexpr int GATE_TILE_X = 3;
    static constexpr int GATE_TILE_Y = 10;

    using OnEggCollectedCallback = std::function<void(int count)>;

    ChickenSystem();

    void setOnEggCollectedCallback(OnEggCollectedCallback cb) { m_onEggCollectedCallback = cb; }

    // Spawns a chicken inside the pen up to pen cap 6. Returns false if already at capacity.
    bool addChicken();

    // Directly sets chicken count and restores spawn positions (for save/load or tests)
    void setChickenCount(int count);
    int getChickenCount() const { return static_cast<int>(m_chickens.size()); }

    const std::vector<Chicken>& getChickens() const { return m_chickens; }
    std::vector<Chicken>& getChickens() { return m_chickens; }

    const std::vector<GroundEgg>& getGroundEggs() const { return m_groundEggs; }
    int getGroundEggCount() const { return static_cast<int>(m_groundEggs.size()); }

    // Adds a ground egg at specified tile
    void addGroundEgg(int tileX, int tileY);
    void clearGroundEggs() { m_groundEggs.clear(); }
    void reset();

    // Morning egg roll for each chicken (~60% chance, capped at 2x chicken count)
    void onNewMorning(int day);

    // Collects an egg at (tileX, tileY) if present. Respects inventory-capacity rule.
    bool collectEgg(int tileX, int tileY, Inventory& inventory, HUD* hud = nullptr, AudioSystem* audio = nullptr);

    // Checks if there is an egg at (tileX, tileY)
    bool hasEggAt(int tileX, int tileY) const;

    // Updates wandering AI, pecking pauses, and pen containment
    void update(float dt);

    // Renders ground eggs and all chickens
    void render(Renderer& renderer) const;

    // Test injection hooks
    void forceNextEggSpawn(bool force = true) { m_forceEggSpawn = force; }
    void setRngSeed(uint32_t seed) { m_rng.seed(seed); }

private:
    std::vector<Chicken> m_chickens;
    std::vector<GroundEgg> m_groundEggs;
    std::mt19937 m_rng;
    bool m_forceEggSpawn = false;
    OnEggCollectedCallback m_onEggCollectedCallback;
};
