#pragma once

#include "entities/Direction.h"
#include "core/Renderer.h"
#include "core/Input.h"
#include "world/Map.h"
#include <utility>

// Controllable farmer entity with 4-directional movement, walk animation, and collision.
class Player {
public:
    Player();

    // Sets initial player position in pixel coordinates.
    void setPosition(float x, float y);

    float getX() const { return m_x; }
    float getY() const { return m_y; }
    Direction getFacing() const { return m_facing; }

    // Computes the grid coordinates of the tile directly in front of the player.
    std::pair<int, int> getTargetTile() const;

    // Updates movement, animation, and collisions using the fixed timestep delta.
    void update(float dt, const Input& input, const Map& map);

    // Renders the animated farmer placeholder and targeted tile cursor.
    void render(Renderer& renderer) const;

private:
    float m_x = 160.0f;
    float m_y = 160.0f;
    float m_speed = 120.0f; // Pixels per second

    Direction m_facing = Direction::Down;
    bool m_isMoving = false;

    // Walk animation variables
    float m_animTimer = 0.0f;
    int m_animStep = 0; // 0..3 cycle: neutral, left-foot, neutral, right-foot
};
