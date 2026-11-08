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
    // 1. Draw target tile highlight cursor
    auto [tx, ty] = getTargetTile();
    if (tx >= 0 && tx < Map::WIDTH && ty >= 0 && ty < Map::HEIGHT) {
        const int targetPx = tx * Map::TILE_SIZE;
        const int targetPy = ty * Map::TILE_SIZE;
        // Semi-transparent highlight box and clear border
        renderer.drawRect(targetPx + 2, targetPy + 2, Map::TILE_SIZE - 4, Map::TILE_SIZE - 4, SDL_Color{255, 255, 120, 60});
        renderer.drawOutline(targetPx + 1, targetPy + 1, Map::TILE_SIZE - 2, Map::TILE_SIZE - 2, SDL_Color{255, 230, 90, 220});
    }

    // 2. Draw animated player placeholder
    const int px = static_cast<int>(m_x);
    const int py = static_cast<int>(m_y);

    const int frames[] = {0, 1, 0, 2};
    const SDL_Rect source{frames[m_animStep] * 32, static_cast<int>(m_facing) * 32, 32, 32};
    if (renderer.drawSprite("player_sheet.png", px, py, 32, 32, &source)) return;

    // Subtle head bob during walking steps
    const int bob = (m_isMoving && (m_animStep == 1 || m_animStep == 3)) ? -1 : 0;

    // Straw Hat (yellow)
    renderer.drawRect(px + 6, py + 2 + bob, 20, 7, SDL_Color{225, 195, 75, 255});
    renderer.drawRect(px + 4, py + 7 + bob, 24, 2, SDL_Color{205, 175, 60, 255}); // Hat brim
    renderer.drawRect(px + 7, py + 6 + bob, 18, 2, SDL_Color{140, 70, 30, 255});  // Hat ribbon band

    // Face / Head
    renderer.drawRect(px + 9, py + 9 + bob, 14, 7, SDL_Color{250, 205, 160, 255});

    // Eyes / Hair depending on direction
    if (m_facing == Direction::Down) {
        renderer.drawRect(px + 11, py + 12 + bob, 2, 2, SDL_Color{40, 25, 20, 255});
        renderer.drawRect(px + 18, py + 12 + bob, 2, 2, SDL_Color{40, 25, 20, 255});
    } else if (m_facing == Direction::Up) {
        // Back of hair visible
        renderer.drawRect(px + 9, py + 9 + bob, 14, 5, SDL_Color{90, 50, 25, 255});
    } else if (m_facing == Direction::Left) {
        renderer.drawRect(px + 10, py + 12 + bob, 2, 2, SDL_Color{40, 25, 20, 255});
    } else if (m_facing == Direction::Right) {
        renderer.drawRect(px + 19, py + 12 + bob, 2, 2, SDL_Color{40, 25, 20, 255});
    }

    // Shirt (warm red flannel)
    renderer.drawRect(px + 8, py + 16 + bob, 16, 5, SDL_Color{195, 60, 50, 255});

    // Overalls (blue denim)
    renderer.drawRect(px + 9, py + 20 + bob, 14, 5, SDL_Color{55, 95, 175, 255});

    // Boots / Feet with walk cycle step displacement
    const SDL_Color bootColor{75, 45, 25, 255};
    if (m_animStep == 1) {
        // Left foot stepped forward, right foot back
        renderer.drawRect(px + 9, py + 24, 5, 6, bootColor);
        renderer.drawRect(px + 18, py + 26, 5, 4, bootColor);
    } else if (m_animStep == 3) {
        // Right foot stepped forward, left foot back
        renderer.drawRect(px + 9, py + 26, 5, 4, bootColor);
        renderer.drawRect(px + 18, py + 24, 5, 6, bootColor);
    } else {
        // Neutral stance
        renderer.drawRect(px + 9, py + 25, 5, 5, bootColor);
        renderer.drawRect(px + 18, py + 25, 5, 5, bootColor);
    }

    // Small black border outline around the character for crisp pixel art presence
    renderer.drawOutline(px + 5, py + 2 + bob, 22, 29, SDL_Color{30, 20, 15, 120});
}
