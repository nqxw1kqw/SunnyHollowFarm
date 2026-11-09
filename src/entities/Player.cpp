#include "entities/Player.h"
#include <cmath>

Player::Player() {
    // Start in the open grass near the farm plot
    m_x = 3.0f * Map::TILE_SIZE;
    m_y = 6.0f * Map::TILE_SIZE;
}

void Player::setPosition(float x, float y) {
    m_x = x;
    m_y = y;
}

void Player::placeAtTileCenter(int tileX, int tileY) {
    m_x = static_cast<float>(tileX * Map::TILE_SIZE);
    m_y = static_cast<float>(tileY * Map::TILE_SIZE);
}

bool Player::overlapsTile(int tileX, int tileY) const {
    const float px1 = m_x + COLLISION_OFFSET_X;
    const float px2 = px1 + COLLISION_WIDTH;
    const float py1 = m_y + COLLISION_OFFSET_Y;
    const float py2 = py1 + COLLISION_HEIGHT;

    const float tx1 = static_cast<float>(tileX * Map::TILE_SIZE);
    const float tx2 = tx1 + static_cast<float>(Map::TILE_SIZE);
    const float ty1 = static_cast<float>(tileY * Map::TILE_SIZE);
    const float ty2 = ty1 + static_cast<float>(Map::TILE_SIZE);

    return (px1 < tx2 && px2 > tx1 && py1 < ty2 && py2 > ty1);
}

std::pair<int, int> Player::getTargetTile() const {
    float targetX = m_x + 16.0f;
    float targetY = m_y + 16.0f;

    switch (m_facing) {
        case Direction::Down:  targetY += FACING_OFFSET; break;
        case Direction::Up:    targetY -= FACING_OFFSET; break;
        case Direction::Left:  targetX -= FACING_OFFSET; break;
        case Direction::Right: targetX += FACING_OFFSET; break;
    }

    int tileX = static_cast<int>(std::floor(targetX / Map::TILE_SIZE));
    int tileY = static_cast<int>(std::floor(targetY / Map::TILE_SIZE));

    return {tileX, tileY};
}

void Player::update(float dt, const Input& input, const Map& map) {
    float moveX = 0.0f;
    float moveY = 0.0f;

    // Check key presses to record last move direction
    const bool upPressed    = input.isKeyPressed(SDL_SCANCODE_W) || input.isKeyPressed(SDL_SCANCODE_UP);
    const bool downPressed  = input.isKeyPressed(SDL_SCANCODE_S) || input.isKeyPressed(SDL_SCANCODE_DOWN);
    const bool leftPressed  = input.isKeyPressed(SDL_SCANCODE_A) || input.isKeyPressed(SDL_SCANCODE_LEFT);
    const bool rightPressed = input.isKeyPressed(SDL_SCANCODE_D) || input.isKeyPressed(SDL_SCANCODE_RIGHT);

    const bool upHeld    = input.isKeyDown(SDL_SCANCODE_W) || input.isKeyDown(SDL_SCANCODE_UP);
    const bool downHeld  = input.isKeyDown(SDL_SCANCODE_S) || input.isKeyDown(SDL_SCANCODE_DOWN);
    const bool leftHeld  = input.isKeyDown(SDL_SCANCODE_A) || input.isKeyDown(SDL_SCANCODE_LEFT);
    const bool rightHeld = input.isKeyDown(SDL_SCANCODE_D) || input.isKeyDown(SDL_SCANCODE_RIGHT);

    if (upHeld) moveY -= 1.0f;
    if (downHeld) moveY += 1.0f;
    if (leftHeld) moveX -= 1.0f;
    if (rightHeld) moveX += 1.0f;

    m_isMoving = (moveX != 0.0f || moveY != 0.0f);

    if (m_isMoving) {
        // Facing = last move direction
        if (upPressed) {
            m_facing = Direction::Up;
        } else if (downPressed) {
            m_facing = Direction::Down;
        } else if (leftPressed) {
            m_facing = Direction::Left;
        } else if (rightPressed) {
            m_facing = Direction::Right;
        } else {
            if (moveX != 0.0f && moveY == 0.0f) {
                m_facing = (moveX < 0.0f) ? Direction::Left : Direction::Right;
            } else if (moveY != 0.0f && moveX == 0.0f) {
                m_facing = (moveY < 0.0f) ? Direction::Up : Direction::Down;
            }
        }

        // Normalize diagonal movement speed
        if (moveX != 0.0f && moveY != 0.0f) {
            constexpr float INV_SQRT2 = 0.70710678f;
            moveX *= INV_SQRT2;
            moveY *= INV_SQRT2;
        }

        const float deltaX = moveX * m_speed * dt;
        const float deltaY = moveY * m_speed * dt;

        // Collision box for player feet (20w x 24h centered horizontally, placed at feet)
        // X-axis collision check & sliding
        const float newX = m_x + deltaX;
        if (map.isAreaWalkable(newX + COLLISION_OFFSET_X, m_y + COLLISION_OFFSET_Y, COLLISION_WIDTH, COLLISION_HEIGHT)) {
            m_x = newX;
        }

        // Y-axis collision check & sliding
        const float newY = m_y + deltaY;
        if (map.isAreaWalkable(m_x + COLLISION_OFFSET_X, newY + COLLISION_OFFSET_Y, COLLISION_WIDTH, COLLISION_HEIGHT)) {
            m_y = newY;
        }

        // Advance walk animation cycle at steady ~8 fps rate (0.125s per frame)
        m_animTimer += dt;
        while (m_animTimer >= 0.125f) {
            m_animTimer -= 0.125f;
            m_animStep = (m_animStep + 1) % 4; // 0: neutral, 1: left, 2: neutral, 3: right
        }
    } else {
        // Idle pose when not moving: show standing frame (neutral)
        m_animStep = 0;
        m_animTimer = 0.0f;
    }
}

void Player::render(Renderer& renderer) const {
    const int px = static_cast<int>(m_x);
    const int py = static_cast<int>(m_y);

    // Scale up Player to 48x48 (1.5x) matching NPCs and breaking out of 1-block grid constraint
    constexpr int PLAYER_WIDTH = 48;
    constexpr int PLAYER_HEIGHT = 48;
    const int renderX = px + (Map::TILE_SIZE - PLAYER_WIDTH) / 2; // px - 8
    const int renderY = (py + Map::TILE_SIZE) - PLAYER_HEIGHT;     // py - 16

    // Subtle head bob during walking steps
    const int bob = (m_isMoving && (m_animStep == 1 || m_animStep == 3)) ? -2 : 0;

    // Ground entity drop shadow anchored at player's feet
    renderer.drawRect(px + 1, py + 24, 30, 6, SDL_Color{20, 20, 20, 95});
    renderer.drawRect(px + 4, py + 25, 24, 4, SDL_Color{10, 10, 10, 65});

    const int frames[] = {0, 1, 0, 2};
    const SDL_Rect source{frames[m_animStep] * 32, static_cast<int>(m_facing) * 32, 32, 32};
    if (renderer.drawSprite("player_sheet.png", renderX, renderY + bob, PLAYER_WIDTH, PLAYER_HEIGHT, &source)) return;

    // High quality procedural player fallback (scaled 48x48)
    // Straw Hat (yellow)
    renderer.drawRect(renderX + 9, renderY + 3 + bob, 30, 10, SDL_Color{225, 195, 75, 255});
    renderer.drawRect(renderX + 6, renderY + 10 + bob, 36, 4, SDL_Color{205, 175, 60, 255}); // Hat brim
    renderer.drawRect(renderX + 10, renderY + 9 + bob, 28, 3, SDL_Color{140, 70, 30, 255});  // Hat ribbon band

    // Face / Head
    renderer.drawRect(renderX + 14, renderY + 14 + bob, 20, 10, SDL_Color{250, 205, 160, 255});

    // Eyes / Hair depending on direction
    if (m_facing == Direction::Down) {
        renderer.drawRect(renderX + 17, renderY + 18 + bob, 3, 3, SDL_Color{40, 25, 20, 255});
        renderer.drawRect(renderX + 27, renderY + 18 + bob, 3, 3, SDL_Color{40, 25, 20, 255});
    } else if (m_facing == Direction::Up) {
        // Back of hair visible
        renderer.drawRect(renderX + 14, renderY + 14 + bob, 20, 8, SDL_Color{90, 50, 25, 255});
    } else if (m_facing == Direction::Left) {
        renderer.drawRect(renderX + 15, renderY + 18 + bob, 3, 3, SDL_Color{40, 25, 20, 255});
    } else if (m_facing == Direction::Right) {
        renderer.drawRect(renderX + 29, renderY + 18 + bob, 3, 3, SDL_Color{40, 25, 20, 255});
    }

    // Shirt (warm red flannel)
    renderer.drawRect(renderX + 12, renderY + 24 + bob, 24, 8, SDL_Color{195, 60, 50, 255});

    // Overalls (blue denim)
    renderer.drawRect(renderX + 14, renderY + 30 + bob, 20, 8, SDL_Color{55, 95, 175, 255});

    // Boots / Feet with walk cycle step displacement
    const SDL_Color bootColor{75, 45, 25, 255};
    if (m_animStep == 1) {
        renderer.drawRect(renderX + 14, renderY + 36, 8, 9, bootColor);
        renderer.drawRect(renderX + 26, renderY + 39, 8, 6, bootColor);
    } else if (m_animStep == 3) {
        renderer.drawRect(renderX + 14, renderY + 39, 8, 6, bootColor);
        renderer.drawRect(renderX + 26, renderY + 36, 8, 9, bootColor);
    } else {
        renderer.drawRect(renderX + 14, renderY + 38, 8, 7, bootColor);
        renderer.drawRect(renderX + 26, renderY + 38, 8, 7, bootColor);
    }

    renderer.drawOutline(renderX + 8, renderY + 3 + bob, 32, 42, SDL_Color{30, 20, 15, 110});
}
