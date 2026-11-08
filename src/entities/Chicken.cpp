#include "entities/Chicken.h"
#include <cmath>
#include <algorithm>

Chicken::Chicken(float x, float y)
    : m_x(x), m_y(y) {
}

void Chicken::setState(ChickenAnimState state, float duration) {
    m_state = state;
    m_stateDuration = duration;
    m_stateTimer = 0.0f;
    m_animTimer = 0.0f;
    if (state != ChickenAnimState::Walk) {
        m_vx = 0.0f;
        m_vy = 0.0f;
    }
}

void Chicken::update(float dt, float minX, float minY, float maxX, float maxY, std::mt19937& rng) {
    m_stateTimer += dt;
    m_animTimer += dt;

    if (m_state == ChickenAnimState::Walk) {
        // Toggle walk frames (walk_0 vs walk_1) every ~0.18s
        if (m_animTimer >= 0.18f) {
            m_animTimer -= 0.18f;
            m_walkFrame = 1 - m_walkFrame;
        }

        // Apply slow random walk velocity
        float nextX = m_x + m_vx * dt;
        float nextY = m_y + m_vy * dt;

        // Containment check against pen boundaries
        if (nextX < minX) {
            nextX = minX;
            m_vx = -m_vx;
            m_facingRight = true;
        } else if (nextX > maxX) {
            nextX = maxX;
            m_vx = -m_vx;
            m_facingRight = false;
        }

        if (nextY < minY) {
            nextY = minY;
            m_vy = -m_vy;
        } else if (nextY > maxY) {
            nextY = maxY;
            m_vy = -m_vy;
        }

        m_x = nextX;
        m_y = nextY;
    }

    // State transition when duration expires
    if (m_stateTimer >= m_stateDuration) {
        m_stateTimer = 0.0f;
        m_animTimer = 0.0f;

        std::uniform_int_distribution<int> stateDist(0, 2);
        int nextStateChoice = stateDist(rng);

        if (nextStateChoice == 0) {
            // Walk state
            m_state = ChickenAnimState::Walk;
            std::uniform_real_distribution<float> durDist(1.5f, 3.0f);
            m_stateDuration = durDist(rng);

            // Choose random walk angle
            std::uniform_real_distribution<float> angleDist(0.0f, 6.2831853f);
            float angle = angleDist(rng);
            m_vx = std::cos(angle) * WALK_SPEED;
            m_vy = std::sin(angle) * WALK_SPEED;
            m_facingRight = (m_vx > 0.0f);
        } else if (nextStateChoice == 1) {
            // Idle pause
            m_state = ChickenAnimState::Idle;
            std::uniform_real_distribution<float> durDist(1.5f, 3.0f);
            m_stateDuration = durDist(rng);
            m_vx = 0.0f;
            m_vy = 0.0f;
        } else {
            // Pecking pause
            m_state = ChickenAnimState::Peck;
            std::uniform_real_distribution<float> durDist(1.2f, 2.5f);
            m_stateDuration = durDist(rng);
            m_vx = 0.0f;
            m_vy = 0.0f;
        }
    }
}

void Chicken::render(Renderer& renderer) const {
    const int px = static_cast<int>(std::round(m_x));
    const int py = static_cast<int>(std::round(m_y));

    // Ground drop shadow
    renderer.drawRect(px + 7, py + 25, 18, 4, SDL_Color{20, 25, 20, 85});

    // Determine sprite filename by current state
    std::string spriteName;
    if (m_state == ChickenAnimState::Walk) {
        spriteName = (m_walkFrame == 0) ? "chicken_walk_0.png" : "chicken_walk_1.png";
    } else if (m_state == ChickenAnimState::Peck) {
        spriteName = "chicken_peck.png";
    } else {
        spriteName = "chicken_idle.png";
    }

    if (renderer.drawSprite(spriteName, px, py, 32, 32)) {
        return;
    }

    // Graceful drawn pixel-art placeholder fallback
    int bob = 0;
    if (m_state == ChickenAnimState::Walk) {
        bob = (m_walkFrame == 1) ? -1 : 0;
    }

    // Facing direction coordinates helper
    auto mirrorX = [this, px](int relX, int width) -> int {
        if (!m_facingRight) {
            return px + relX;
        } else {
            // Mirror around 32px center
            return px + (32 - relX - width);
        }
    };

    // 1. Legs / Orange feet
    SDL_Color feetCol{240, 150, 20, 255};
    if (m_state == ChickenAnimState::Walk) {
        if (m_walkFrame == 0) {
            renderer.drawRect(mirrorX(11, 3), py + 22, 3, 5, feetCol);
            renderer.drawRect(mirrorX(17, 3), py + 21, 3, 6, feetCol);
        } else {
            renderer.drawRect(mirrorX(11, 3), py + 21, 3, 6, feetCol);
            renderer.drawRect(mirrorX(17, 3), py + 22, 3, 5, feetCol);
        }
    } else {
        renderer.drawRect(mirrorX(12, 3), py + 22, 3, 5, feetCol);
        renderer.drawRect(mirrorX(17, 3), py + 22, 3, 5, feetCol);
    }

    // 2. White/cream round feather body
    SDL_Color bodyCol{250, 248, 242, 255};
    SDL_Color shadeCol{225, 220, 210, 255};
    renderer.drawRect(mirrorX(8, 16), py + 12 + bob, 16, 11, bodyCol);
    renderer.drawRect(mirrorX(10, 12), py + 21 + bob, 12, 2, shadeCol);

    // 3. Wing feather detail
    SDL_Color wingCol{238, 234, 224, 255};
    renderer.drawRect(mirrorX(11, 8), py + 14 + bob, 8, 6, wingCol);
    renderer.drawRect(mirrorX(13, 5), py + 18 + bob, 5, 2, shadeCol);

    // 4. Head, comb, beak & eye depending on state
    SDL_Color combCol{225, 40, 35, 255};
    SDL_Color beakCol{250, 190, 25, 255};
    SDL_Color eyeCol{25, 25, 30, 255};

    if (m_state == ChickenAnimState::Peck) {
        // Lowered head pecking at the grass
        renderer.drawRect(mirrorX(5, 8), py + 16, 8, 7, bodyCol);
        // Beak angled down towards ground
        renderer.drawRect(mirrorX(2, 4), py + 20, 4, 3, beakCol);
        // Comb on lowered head
        renderer.drawRect(mirrorX(7, 4), py + 14, 4, 3, combCol);
        // Wattle
        renderer.drawRect(mirrorX(5, 2), py + 21, 2, 3, combCol);
        // Eye
        renderer.drawRect(mirrorX(8, 2), py + 17, 2, 2, eyeCol);
    } else {
        // Standing upright (Idle or Walk)
        int headBob = bob;
        // Head
        renderer.drawRect(mirrorX(6, 8), py + 6 + headBob, 8, 8, bodyCol);
        // Red comb on top of head
        renderer.drawRect(mirrorX(8, 5), py + 3 + headBob, 5, 4, combCol);
        // Beak pointing forward
        renderer.drawRect(mirrorX(2, 5), py + 9 + headBob, 5, 3, beakCol);
        // Red wattle under beak
        renderer.drawRect(mirrorX(5, 2), py + 12 + headBob, 2, 3, combCol);
        // Eye
        renderer.drawRect(mirrorX(8, 2), py + 8 + headBob, 2, 2, eyeCol);
        renderer.drawRect(mirrorX(8, 1), py + 8 + headBob, 1, 1, SDL_Color{255, 255, 255, 255});
    }
}
