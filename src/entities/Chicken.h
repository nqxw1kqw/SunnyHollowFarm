#pragma once

#include <random>
#include <string>
#include <SDL.h>
#include "core/Renderer.h"

// Lifecycle animation states for pen chickens
enum class ChickenAnimState {
    Idle,
    Walk,
    Peck
};

class Chicken {
public:
    static constexpr float CHICKEN_SIZE = 32.0f;
    static constexpr float WALK_SPEED = 24.0f;

    explicit Chicken(float x = 0.0f, float y = 0.0f);

    // Updates wandering AI, animation frame progression, and containment within pen boundaries
    void update(float dt, float minX, float minY, float maxX, float maxY, std::mt19937& rng);

    // Renders chicken using art contract PNGs with graceful drawn pixel-art fallback
    void render(Renderer& renderer) const;

    float getX() const { return m_x; }
    float getY() const { return m_y; }
    void setPosition(float x, float y) { m_x = x; m_y = y; }

    ChickenAnimState getState() const { return m_state; }
    bool isFacingRight() const { return m_facingRight; }
    int getWalkFrame() const { return m_walkFrame; }

    // Direct state injection for testing
    void setState(ChickenAnimState state, float duration = 2.0f);

private:
    float m_x = 0.0f;
    float m_y = 0.0f;
    float m_vx = 0.0f;
    float m_vy = 0.0f;

    ChickenAnimState m_state = ChickenAnimState::Idle;
    float m_stateTimer = 0.0f;
    float m_stateDuration = 2.0f;
    float m_animTimer = 0.0f;
    int m_walkFrame = 0; // 0 or 1
    bool m_facingRight = false;
};
