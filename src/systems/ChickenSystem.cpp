#include "systems/ChickenSystem.h"
#include "world/Map.h"
#include <iostream>
#include <algorithm>

ChickenSystem::ChickenSystem()
    : m_rng(1337) {
}

bool ChickenSystem::addChicken() {
    if (static_cast<int>(m_chickens.size()) >= MAX_CHICKENS) {
        return false;
    }

    int idx = static_cast<int>(m_chickens.size());
    int tx = PEN_MIN_TILE_X + (idx % (PEN_MAX_TILE_X - PEN_MIN_TILE_X + 1));
    int ty = PEN_MIN_TILE_Y + (idx / (PEN_MAX_TILE_X - PEN_MIN_TILE_X + 1));

    float sx = static_cast<float>(tx * Map::TILE_SIZE);
    float sy = static_cast<float>(ty * Map::TILE_SIZE);

    m_chickens.emplace_back(sx, sy);
    std::cout << "[ChickenSystem] Spawned chicken #" << m_chickens.size()
              << " in pen at tile (" << tx << ", " << ty << ")." << std::endl;
    return true;
}

void ChickenSystem::setChickenCount(int count) {
    count = std::max(0, std::min(count, MAX_CHICKENS));
    m_chickens.clear();
    for (int i = 0; i < count; ++i) {
        int tx = PEN_MIN_TILE_X + (i % (PEN_MAX_TILE_X - PEN_MIN_TILE_X + 1));
        int ty = PEN_MIN_TILE_Y + (i / (PEN_MAX_TILE_X - PEN_MIN_TILE_X + 1));
        float sx = static_cast<float>(tx * Map::TILE_SIZE);
        float sy = static_cast<float>(ty * Map::TILE_SIZE);
        m_chickens.emplace_back(sx, sy);
    }
}

void ChickenSystem::addGroundEgg(int tileX, int tileY) {
    m_groundEggs.push_back({tileX, tileY});
}

void ChickenSystem::reset() {
    m_chickens.clear();
    m_groundEggs.clear();
    m_forceEggSpawn = false;
    m_rng.seed(1337);
}

bool ChickenSystem::hasEggAt(int tileX, int tileY) const {
    for (const auto& egg : m_groundEggs) {
        if (egg.tileX == tileX && egg.tileY == tileY) {
            return true;
        }
    }
    return false;
}

void ChickenSystem::onNewMorning(int day) {
    if (m_chickens.empty()) {
        return;
    }

    // Deterministic seeding based on in-game day if not manually overridden
    if (!m_forceEggSpawn) {
        m_rng.seed(static_cast<uint32_t>(day * 2654435761u + 42u));
    }

    const int maxGroundEggs = static_cast<int>(m_chickens.size()) * 2;
    int eggsLaidToday = 0;

    for (size_t i = 0; i < m_chickens.size(); ++i) {
        if (static_cast<int>(m_groundEggs.size()) >= maxGroundEggs) {
            std::cout << "[ChickenSystem] Ground egg cap (" << maxGroundEggs << ") reached. No more eggs laid." << std::endl;
            break;
        }

        bool laysEgg = m_forceEggSpawn;
        if (!laysEgg) {
            std::uniform_real_distribution<float> dist(0.0f, 1.0f);
            laysEgg = (dist(m_rng) < 0.60f); // ~60% chance
        }

        if (laysEgg) {
            const auto& ch = m_chickens[i];
            int chTileX = static_cast<int>(std::round(ch.getX() / static_cast<float>(Map::TILE_SIZE)));
            int chTileY = static_cast<int>(std::round(ch.getY() / static_cast<float>(Map::TILE_SIZE)));

            int targetX = chTileX;
            int targetY = chTileY;

            // If chicken tile is out of interior or already has an egg, find an unoccupied pen tile
            bool validTile = (targetX >= PEN_MIN_TILE_X && targetX <= PEN_MAX_TILE_X &&
                              targetY >= PEN_MIN_TILE_Y && targetY <= PEN_MAX_TILE_Y &&
                              !hasEggAt(targetX, targetY));

            if (!validTile) {
                for (int ty = PEN_MIN_TILE_Y; ty <= PEN_MAX_TILE_Y; ++ty) {
                    for (int tx = PEN_MIN_TILE_X; tx <= PEN_MAX_TILE_X; ++tx) {
                        if (!hasEggAt(tx, ty)) {
                            targetX = tx;
                            targetY = ty;
                            validTile = true;
                            break;
                        }
                    }
                    if (validTile) break;
                }
            }

            // Fallback: clamp to pen interior
            targetX = std::max(PEN_MIN_TILE_X, std::min(targetX, PEN_MAX_TILE_X));
            targetY = std::max(PEN_MIN_TILE_Y, std::min(targetY, PEN_MAX_TILE_Y));

            addGroundEgg(targetX, targetY);
            eggsLaidToday++;
            std::cout << "[ChickenSystem] Chicken #" << (i + 1) << " laid an egg at ("
                      << targetX << ", " << targetY << ") [Total ground eggs: "
                      << m_groundEggs.size() << "]" << std::endl;
        }
    }

    m_forceEggSpawn = false;
}

bool ChickenSystem::collectEgg(int tileX, int tileY, Inventory& inventory, HUD* hud, AudioSystem* audio) {
    auto it = std::find_if(m_groundEggs.begin(), m_groundEggs.end(), [tileX, tileY](const GroundEgg& egg) {
        return egg.tileX == tileX && egg.tileY == tileY;
    });

    if (it == m_groundEggs.end()) {
        return false;
    }

    if (!inventory.hasSpaceFor(ItemType::Egg, 1)) {
        std::cout << "[ChickenSystem] Cannot collect egg: Inventory is full!" << std::endl;
        if (hud) {
            hud->showFeedback("INVENTORY FULL!");
        }
        return false;
    }

    m_groundEggs.erase(it);
    inventory.addItem(ItemType::Egg, 1);
    if (m_onEggCollectedCallback) {
        m_onEggCollectedCallback(1);
    }

    std::cout << "[ChickenSystem] Collected 1x Egg from (" << tileX << ", " << tileY
              << ") [Total Eggs in bag: " << inventory.getItemCount(ItemType::Egg) << "]" << std::endl;

    if (audio) {
        audio->playSFX(SFX::Harvest);
    }
    if (hud) {
        hud->showFeedback("COLLECTED 1 EGG!");
    }

    return true;
}

void ChickenSystem::update(float dt) {
    // Wander bounds (pen interior: x=2..5, y=11..13)
    const float minX = static_cast<float>(PEN_MIN_TILE_X * Map::TILE_SIZE);
    const float maxX = static_cast<float>((PEN_MAX_TILE_X + 1) * Map::TILE_SIZE) - Chicken::CHICKEN_SIZE;
    const float minY = static_cast<float>(PEN_MIN_TILE_Y * Map::TILE_SIZE);
    const float maxY = static_cast<float>((PEN_MAX_TILE_Y + 1) * Map::TILE_SIZE) - Chicken::CHICKEN_SIZE;

    for (auto& chicken : m_chickens) {
        chicken.update(dt, minX, minY, maxX, maxY, m_rng);
    }
}

void ChickenSystem::render(Renderer& renderer) const {
    // 1. Render ground eggs
    for (const auto& egg : m_groundEggs) {
        const int px = egg.tileX * Map::TILE_SIZE;
        const int py = egg.tileY * Map::TILE_SIZE;

        if (!renderer.drawSprite("item_egg.png", px, py, Map::TILE_SIZE, Map::TILE_SIZE)) {
            // Drawn placeholder fallback: egg nestled in grass
            // Grass tuft / straw nest beneath egg
            renderer.drawRect(px + 8, py + 22, 16, 4, SDL_Color{160, 140, 70, 255});
            renderer.drawRect(px + 6, py + 20, 20, 3, SDL_Color{180, 160, 85, 255});

            // Smooth ivory egg shape
            renderer.drawRect(px + 12, py + 12, 8, 4, SDL_Color{248, 244, 235, 255});
            renderer.drawRect(px + 10, py + 15, 12, 7, SDL_Color{252, 248, 240, 255});
            renderer.drawRect(px + 11, py + 21, 10, 2, SDL_Color{230, 222, 205, 255});
            // Highlight shine
            renderer.drawRect(px + 13, py + 14, 2, 2, SDL_Color{255, 255, 255, 255});
        }
    }

    // 2. Render chickens
    for (const auto& chicken : m_chickens) {
        chicken.render(renderer);
    }
}
