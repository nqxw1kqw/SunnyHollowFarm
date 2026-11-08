#pragma once

#include <random>
#include <string>
#include "world/Map.h"
#include "entities/Player.h"
#include "entities/Inventory.h"
#include "ui/HUD.h"
#include "systems/AudioSystem.h"

// Lifecycle states of the pond fishing mechanic
enum class FishingState {
    None,     // Idle / not fishing
    Waiting,  // Line cast in water, waiting for fish to bite (2.0s - 5.0s)
    Bite      // Bite window active (~800 ms) with sparkle and "!" prompt
};

class FishingSystem {
public:
    FishingSystem();

    // Cancel current fishing session immediately (e.g. on movement, modal open, or early scare)
    void cancel();

    // State queries
    bool isFishing() const { return m_state != FishingState::None; }
    FishingState getState() const { return m_state; }
    int getWaterTileX() const { return m_waterTileX; }
    int getWaterTileY() const { return m_waterTileY; }
    float getStateTimer() const { return m_stateTimer; }
    float getWaitDuration() const { return m_waitDuration; }
    float getBiteWindowDuration() const { return m_biteWindowDuration; }

    // Start fishing when player faces pond water tile from the bank
    bool startFishing(int waterTileX, int waterTileY, Player& player, HUD* hud = nullptr, AudioSystem* audio = nullptr);

    // Handle action key input (key E or Space) while in an active fishing state
    bool handleActionInput(Inventory& inventory, HUD* hud = nullptr, AudioSystem* audio = nullptr);

    // Per-frame time update for wait and bite window progression
    void update(float dt, HUD* hud = nullptr);

    // Renders fishing visual elements (bobber while waiting, sparkle and "!" during bite)
    void render(Renderer& renderer) const;

    // Test injection hooks for deterministic verification
    void setNextWaitDuration(float seconds) { m_injectedWaitDuration = seconds; }
    void setSeed(uint32_t seed) { m_rng.seed(seed); }

private:
    FishingState m_state = FishingState::None;
    int m_waterTileX = -1;
    int m_waterTileY = -1;
    float m_stateTimer = 0.0f;
    float m_waitDuration = 3.0f;
    float m_biteWindowDuration = 0.8f; // ~800 ms bite window
    float m_injectedWaitDuration = -1.0f;

    std::mt19937 m_rng;
};
