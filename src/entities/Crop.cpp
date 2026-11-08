#include "entities/Crop.h"
#include "world/Map.h"

Crop::Crop(int tileX, int tileY, ItemType cropType)
    : m_tileX(tileX), m_tileY(tileY), m_cropType(cropType) {}

void Crop::advanceStage() {
    if (m_stage != CropStage::Harvest) {
        m_stage = static_cast<CropStage>(static_cast<int>(m_stage) + 1);
        m_stageTimer = 0.0f;
    }
}

void Crop::update(float dt, float speedMultiplier) {
    if (m_stage == CropStage::Harvest) {
        // Already ripe and ready for harvest; does not grow further
        return;
    }

    m_stageTimer += dt * speedMultiplier;

    if (m_stageTimer >= TIME_PER_STAGE) {
        m_stageTimer = 0.0f;
        int nextStage = static_cast<int>(m_stage) + 1;
        if (nextStage <= static_cast<int>(CropStage::Harvest)) {
            m_stage = static_cast<CropStage>(nextStage);
        }
    }
}

void Crop::render(Renderer& renderer) const {
    const int px = m_tileX * Map::TILE_SIZE;
    const int py = m_tileY * Map::TILE_SIZE;

    bool isWheat = (m_cropType == ItemType::Wheat || m_cropType == ItemType::WheatSeeds);

    if (!isWheat && renderer.drawSprite("tomato_stage" + std::to_string(static_cast<int>(m_stage)) + ".png",
                                       px, py, 32, 32)) {
        if (isMature()) renderer.drawSprite("fx_sparkle.png", px + 20, py, 12, 12);
        return;
    }

    if (isWheat) {
        switch (m_stage) {
            case CropStage::Seed: {
                // Soil mound with golden grain seeds
                renderer.drawRect(px + 10, py + 14, 12, 4, SDL_Color{70, 42, 22, 255});
                renderer.drawRect(px + 12, py + 13, 2, 2, SDL_Color{240, 215, 60, 255});
                renderer.drawRect(px + 17, py + 14, 2, 2, SDL_Color{240, 215, 60, 255});
                break;
            }
            case CropStage::Sprout: {
                // Thin golden-green shoots
                renderer.drawRect(px + 13, py + 14, 2, 8, SDL_Color{120, 185, 60, 255});
                renderer.drawRect(px + 17, py + 12, 2, 10, SDL_Color{120, 185, 60, 255});
                break;
            }
            case CropStage::Growing: {
                // Bushy green stalks with golden heads
                renderer.drawRect(px + 11, py + 10, 3, 13, SDL_Color{160, 175, 50, 255});
                renderer.drawRect(px + 16, py + 8, 3, 15, SDL_Color{175, 185, 55, 255});
                renderer.drawRect(px + 10, py + 7, 5, 5, SDL_Color{220, 195, 60, 255});
                renderer.drawRect(px + 15, py + 5, 5, 5, SDL_Color{230, 205, 65, 255});
                break;
            }
            case CropStage::Harvest: {
                // Full golden wheat sheaves ready for harvest
                renderer.drawRect(px + 8, py + 8, 16, 17, SDL_Color{210, 170, 45, 255});
                renderer.drawRect(px + 10, py + 5, 12, 7, SDL_Color{245, 210, 65, 255});
                renderer.drawRect(px + 7, py + 16, 18, 3, SDL_Color{170, 50, 40, 255}); // tie ribbon
                renderer.drawOutline(px + 4, py + 4, 24, 23, SDL_Color{255, 235, 100, 180}); // sparkle
                break;
            }
        }
        return;
    }

    // Default: Tomato
    switch (m_stage) {
        case CropStage::Seed: {
            // Stage 1: Little soil mound with visible seed grains
            renderer.drawRect(px + 10, py + 14, 12, 4, SDL_Color{70, 42, 22, 255});
            renderer.drawRect(px + 12, py + 13, 2, 2, SDL_Color{230, 200, 110, 255});
            renderer.drawRect(px + 17, py + 14, 2, 2, SDL_Color{230, 200, 110, 255});
            break;
        }

        case CropStage::Sprout: {
            // Stage 2: Small green seedling
            // Stem
            renderer.drawRect(px + 15, py + 15, 2, 7, SDL_Color{75, 170, 50, 255});
            // Left & Right leaves
            renderer.drawRect(px + 11, py + 13, 4, 3, SDL_Color{110, 210, 65, 255});
            renderer.drawRect(px + 17, py + 13, 4, 3, SDL_Color{110, 210, 65, 255});
            break;
        }

        case CropStage::Growing: {
            // Stage 3: Bushy tomato plant with small yellow flower buds
            // Foliage base
            renderer.drawRect(px + 8, py + 11, 16, 12, SDL_Color{50, 145, 40, 255});
            renderer.drawRect(px + 11, py + 7, 10, 5, SDL_Color{70, 175, 55, 255});
            // Yellow flower buds
            renderer.drawRect(px + 9, py + 14, 3, 3, SDL_Color{250, 225, 60, 255});
            renderer.drawRect(px + 19, py + 13, 3, 3, SDL_Color{250, 225, 60, 255});
            break;
        }

        case CropStage::Harvest: {
            // Stage 4: Mature leafy bush laden with ripe red tomatoes
            renderer.drawRect(px + 6, py + 8, 20, 15, SDL_Color{40, 130, 35, 255});
            renderer.drawRect(px + 9, py + 5, 14, 5, SDL_Color{60, 160, 45, 255});

            // Ripe red tomato #1 (left)
            renderer.drawRect(px + 7, py + 13, 6, 6, SDL_Color{230, 45, 35, 255});
            renderer.drawRect(px + 8, py + 14, 2, 2, SDL_Color{255, 120, 105, 255}); // highlight
            renderer.drawRect(px + 9, py + 12, 2, 2, SDL_Color{45, 140, 35, 255});   // stem calyx

            // Ripe red tomato #2 (right)
            renderer.drawRect(px + 18, py + 12, 6, 6, SDL_Color{230, 45, 35, 255});
            renderer.drawRect(px + 19, py + 13, 2, 2, SDL_Color{255, 120, 105, 255});
            renderer.drawRect(px + 20, py + 11, 2, 2, SDL_Color{45, 140, 35, 255});

            // Ripe red tomato #3 (center)
            renderer.drawRect(px + 13, py + 16, 6, 6, SDL_Color{215, 40, 30, 255});
            renderer.drawRect(px + 14, py + 17, 2, 2, SDL_Color{255, 120, 105, 255});

            // Golden harvest sparkle outline
            renderer.drawOutline(px + 4, py + 4, 24, 23, SDL_Color{255, 235, 100, 180});
            break;
        }
    }
}
