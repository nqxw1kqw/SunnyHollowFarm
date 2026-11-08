#include <cmath>
#include "world/Map.h"
#include <algorithm>

Map::Map() {
    initDefault();
}

void Map::initDefault() {
    // 1. Fill entire map with Grass
    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; ++x) {
            m_tiles[getIndex(x, y)] = Tile(TileType::Grass);
        }
    }

    // 2. Fences along top border (leaving path open at center)
    for (int x = 0; x < WIDTH; ++x) {
        if (x < 8 || x > 11) {
            m_tiles[getIndex(x, 0)] = Tile(TileType::Fence);
        }
    }

    // 2b. Chicken pen enclosure in southwest meadow (4x3 interior: x=2..5, y=11..13)
    // Top fence at y=10 (leaving 1-tile gate gap at x=3 for player entry)
    for (int x = 1; x <= 6; ++x) {
        if (x != 3) {
            m_tiles[getIndex(x, 10)] = Tile(TileType::Fence);
        }
    }
    // Bottom fence at y=14
    for (int x = 1; x <= 6; ++x) {
        m_tiles[getIndex(x, 14)] = Tile(TileType::Fence);
    }
    // Left fence at x=1 and Right fence at x=6
    for (int y = 11; y <= 13; ++y) {
        m_tiles[getIndex(1, y)] = Tile(TileType::Fence);
        m_tiles[getIndex(6, y)] = Tile(TileType::Fence);
    }

    // 3. Central farming soil plot (8 wide x 6 tall)
    for (int y = 4; y <= 9; ++y) {
        for (int x = 5; x <= 12; ++x) {
            m_tiles[getIndex(x, y)] = Tile(TileType::Dirt);
        }
    }

    // Start with a couple of already tilled patches to make the farm inviting
    m_tiles[getIndex(6, 5)] = Tile(TileType::TilledSoil);
    m_tiles[getIndex(7, 5)] = Tile(TileType::TilledSoil);

    // 3b. Dirt path region leading from the farmhouse door toward the field and the stalls
    for (int x = 2; x <= 18; ++x) {
        m_tiles[getIndex(x, 3)] = Tile(TileType::Path);
    }
    for (int y = 4; y <= 6; ++y) {
        m_tiles[getIndex(4, y)] = Tile(TileType::Path);
    }

    // 4. Natural pond in bottom-right corner (3x3 water)
    for (int y = 11; y <= 13; ++y) {
        for (int x = 16; x <= 18; ++x) {
            m_tiles[getIndex(x, y)] = Tile(TileType::Water);
        }
    }

    // 5. Farmhouse structure in top-left (3 wide x 2 tall, tiles x=1..3, y=1..2)
    for (int y = 1; y <= 2; ++y) {
        for (int x = 1; x <= 3; ++x) {
            m_tiles[getIndex(x, y)] = Tile(TileType::Farmhouse);
        }
    }
    // Farmhouse entrance door at (2, 2) is enterable by stepping on it
    m_tiles[getIndex(2, 2)] = Tile(TileType::FarmhouseDoor);

    // 6. Kitchen Stove interaction point at (4, 2) on the farmhouse patio
    m_tiles[getIndex(4, 2)] = Tile(TileType::Stove);

    // 7. Market Shop Stalls in top-right (x=13..15 for Seller stall, x=16..18 for Buyer stall at y=1)
    for (int x = 13; x <= 18; ++x) {
        m_tiles[getIndex(x, 1)] = Tile(TileType::ShopStall);
    }
}

void Map::initInterior() {
    // 1. Fill entire map with Void (dark backdrop)
    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; ++x) {
            m_tiles[getIndex(x, y)] = Tile(TileType::Void);
        }
    }

    // 2. Room interior dimensions: x=5..14 (10 wide), y=3..10 (8 tall)
    // Wood plank floor
    for (int y = 4; y <= 9; ++y) {
        for (int x = 6; x <= 13; ++x) {
            m_tiles[getIndex(x, y)] = Tile(TileType::WoodFloor);
        }
    }

    // Top wall
    for (int x = 5; x <= 14; ++x) {
        m_tiles[getIndex(x, 3)] = Tile(TileType::Wall);
    }
    // Left and Right walls
    for (int y = 3; y <= 10; ++y) {
        m_tiles[getIndex(5, y)] = Tile(TileType::Wall);
        m_tiles[getIndex(14, y)] = Tile(TileType::Wall);
    }
    // Bottom wall
    for (int x = 5; x <= 14; ++x) {
        m_tiles[getIndex(x, 10)] = Tile(TileType::Wall);
    }

    // Exit threshold doorway at (9, 10)
    m_tiles[getIndex(9, 10)] = Tile(TileType::InteriorExit);

    // Props:
    // Bed at (6, 4)
    m_tiles[getIndex(6, 4)] = Tile(TileType::Bed);

    // Storage Chest at (8, 4)
    m_tiles[getIndex(8, 4)] = Tile(TileType::Chest);

    // Indoor kitchen stove at (11, 4)
    m_tiles[getIndex(11, 4)] = Tile(TileType::Stove);

    // Bookshelf / study at (13, 5)
    m_tiles[getIndex(13, 5)] = Tile(TileType::Bookshelf);
}

bool Map::inBounds(int tileX, int tileY) const {
    return tileX >= 0 && tileX < WIDTH && tileY >= 0 && tileY < HEIGHT;
}

Tile& Map::getTile(int tileX, int tileY) {
    return m_tiles[getIndex(tileX, tileY)];
}

const Tile& Map::getTile(int tileX, int tileY) const {
    return m_tiles[getIndex(tileX, tileY)];
}

void Map::setTileType(int tileX, int tileY, TileType type) {
    if (inBounds(tileX, tileY)) {
        m_tiles[getIndex(tileX, tileY)].setType(type);
    }
}

bool Map::isAreaWalkable(float x, float y, float w, float h) const {
    // Map bounds check
    if (x < 0.0f || y < 0.0f || (x + w) > WIDTH * TILE_SIZE || (y + h) > HEIGHT * TILE_SIZE) {
        return false;
    }

    // Overlapping tile bounds
    int startTileX = std::max(0, static_cast<int>(x / TILE_SIZE));
    int endTileX = std::min(WIDTH - 1, static_cast<int>((x + w - 0.01f) / TILE_SIZE));
    int startTileY = std::max(0, static_cast<int>(y / TILE_SIZE));
    int endTileY = std::min(HEIGHT - 1, static_cast<int>((y + h - 0.01f) / TILE_SIZE));

    for (int ty = startTileY; ty <= endTileY; ++ty) {
        for (int tx = startTileX; tx <= endTileX; ++tx) {
            // Block walking through solid tiles or NPC merchant positions (14, 2) and (17, 2)
            if (!getTile(tx, ty).isWalkable() || ((tx == 14 || tx == 17) && ty == 2)) {
                return false;
            }
        }
    }

    return true;
}

int Map::getCellVariant(int tileX, int tileY, int numVariants) {
    if (numVariants <= 1) return 0;
    uint32_t h = static_cast<uint32_t>((tileX * 0x1f1f1f1f) ^ (tileY * 0x5d5d5d5d));
    h = (h ^ (h >> 13)) * 0x5bd1e995;
    h ^= (h >> 15);
    return static_cast<int>(h % static_cast<uint32_t>(numVariants));
}

std::string Map::getShoreTileName(int tileX, int tileY) const {
    if (!inBounds(tileX, tileY) || getTile(tileX, tileY).getType() != TileType::Water) {
        return "";
    }

    auto isLand = [this](int x, int y) {
        if (!inBounds(x, y)) return true;
        return getTile(x, y).getType() != TileType::Water;
    };

    bool n = isLand(tileX, tileY - 1);
    bool s = isLand(tileX, tileY + 1);
    bool w = isLand(tileX - 1, tileY);
    bool e = isLand(tileX + 1, tileY);

    if (n && w) return "tile_shore_nw.png";
    if (n && e) return "tile_shore_ne.png";
    if (s && w) return "tile_shore_sw.png";
    if (s && e) return "tile_shore_se.png";

    if (n) return "tile_shore_n.png";
    if (s) return "tile_shore_s.png";
    if (w) return "tile_shore_w.png";
    if (e) return "tile_shore_e.png";

    // Open pond water alternates tile_water_0 / tile_water_1 on ~600 ms global timer (all in sync)
    int variant = getWaterVariant();
    return "tile_water_" + std::to_string(variant) + ".png";
}

void Map::update(float dt) {
    m_waterAnimTimer += dt;
    if (m_waterAnimTimer >= 1.2f) {
        m_waterAnimTimer = std::fmod(m_waterAnimTimer, 1.2f);
    }
}

int Map::getWaterVariant() const {
    int v = static_cast<int>(std::floor(m_waterAnimTimer / 0.6f)) % 2;
    return (v < 0) ? (v + 2) : v;
}

void Map::render(Renderer& renderer) const {
    const bool isOutdoor = (getTile(0, 0).getType() != TileType::Void);

    for (int ty = 0; ty < HEIGHT; ++ty) {
        for (int tx = 0; tx < WIDTH; ++tx) {
            const int px = tx * TILE_SIZE;
            const int py = ty * TILE_SIZE;
            const Tile& tile = getTile(tx, ty);

            if (isOutdoor) {
                // 1. Render continuous ground base by region (Grass, Path, Dirt, Water/Shore)
                TileType baseTerrain = tile.getType();
                if (baseTerrain == TileType::Fence || baseTerrain == TileType::Farmhouse ||
                    baseTerrain == TileType::FarmhouseDoor || baseTerrain == TileType::ShopStall ||
                    (baseTerrain == TileType::Stove && ty == 2)) {
                    baseTerrain = TileType::Grass;
                } else if (baseTerrain == TileType::TilledSoil || baseTerrain == TileType::WateredSoil) {
                    baseTerrain = TileType::Dirt;
                }

                switch (baseTerrain) {
                    case TileType::Grass: {
                        int variant = getCellVariant(tx, ty, 8);
                        std::string spriteName = "tile_grass_" + std::to_string(variant) + ".png";
                        if (!renderer.drawSprite(spriteName, px, py, TILE_SIZE, TILE_SIZE)) {
                            const SDL_Rect grassSrc{0, 32, 32, 32};
                            if (!renderer.drawSprite("tiles_ground.png", px, py, TILE_SIZE, TILE_SIZE, &grassSrc)) {
                                renderer.drawRect(px, py, TILE_SIZE, TILE_SIZE, SDL_Color{88, 155, 60, 255});
                                renderer.drawRect(px + 6, py + 8, 3, 5, SDL_Color{108, 178, 76, 255});
                                renderer.drawRect(px + 20, py + 18, 4, 6, SDL_Color{108, 178, 76, 255});
                            }
                        }
                        break;
                    }
                    case TileType::Path: {
                        int variant = getCellVariant(tx, ty, 2);
                        std::string spriteName = "tile_path_" + std::to_string(variant) + ".png";
                        if (!renderer.drawSprite(spriteName, px, py, TILE_SIZE, TILE_SIZE)) {
                            const SDL_Rect dirtSrc{0, 0, 32, 32};
                            if (!renderer.drawSprite("tiles_ground.png", px, py, TILE_SIZE, TILE_SIZE, &dirtSrc)) {
                                renderer.drawRect(px, py, TILE_SIZE, TILE_SIZE, SDL_Color{155, 110, 68, 255});
                                renderer.drawRect(px + 6, py + 12, 3, 2, SDL_Color{135, 92, 52, 255});
                                renderer.drawRect(px + 20, py + 22, 2, 2, SDL_Color{175, 128, 82, 255});
                            }
                        }
                        break;
                    }
                    case TileType::Dirt: {
                        int variant = getCellVariant(tx, ty, 2);
                        std::string spriteName = "tile_dirt_" + std::to_string(variant) + ".png";
                        if (!renderer.drawSprite(spriteName, px, py, TILE_SIZE, TILE_SIZE)) {
                            const SDL_Rect dirtSrc{0, 0, 32, 32};
                            if (!renderer.drawSprite("tiles_ground.png", px, py, TILE_SIZE, TILE_SIZE, &dirtSrc)) {
                                renderer.drawRect(px, py, TILE_SIZE, TILE_SIZE, SDL_Color{142, 98, 59, 255});
                                renderer.drawRect(px + 8, py + 10, 3, 2, SDL_Color{118, 78, 42, 255});
                                renderer.drawRect(px + 22, py + 20, 2, 2, SDL_Color{160, 115, 75, 255});
                            }
                        }
                        break;
                    }
                    case TileType::Water: {
                        std::string spriteName = getShoreTileName(tx, ty);
                        if (!renderer.drawSprite(spriteName, px, py, TILE_SIZE, TILE_SIZE)) {
                            const SDL_Rect waterSrc{32, 32, 32, 32};
                            if (!renderer.drawSprite("tiles_ground.png", px, py, TILE_SIZE, TILE_SIZE, &waterSrc)) {
                                renderer.drawRect(px, py, TILE_SIZE, TILE_SIZE, SDL_Color{60, 130, 200, 255});
                                renderer.drawRect(px + 4, py + 8, 12, 2, SDL_Color{120, 185, 240, 220});
                                renderer.drawRect(px + 14, py + 20, 10, 2, SDL_Color{120, 185, 240, 220});
                            }
                        }
                        break;
                    }
                    default:
                        break;
                }

                // 2. Render overlays on top of the continuous ground
                if (tile.getType() == TileType::TilledSoil) {
                    const SDL_Rect tilledSrc{32, 0, 32, 32};
                    if (!renderer.drawSprite("tiles_ground.png", px, py, TILE_SIZE, TILE_SIZE, &tilledSrc)) {
                        renderer.drawRect(px + 2, py + 6, TILE_SIZE - 4, 4, SDL_Color{74, 45, 26, 255});
                        renderer.drawRect(px + 2, py + 16, TILE_SIZE - 4, 4, SDL_Color{74, 45, 26, 255});
                        renderer.drawRect(px + 2, py + 24, TILE_SIZE - 4, 3, SDL_Color{74, 45, 26, 255});
                    }
                    continue;
                }

                if (tile.getType() == TileType::WateredSoil) {
                    if (!renderer.drawSprite("tile_tilled_watered.png", px, py, TILE_SIZE, TILE_SIZE)) {
                        renderer.drawRect(px, py, TILE_SIZE, TILE_SIZE, SDL_Color{58, 36, 20, 255});
                        renderer.drawRect(px + 2, py + 6, TILE_SIZE - 4, 4, SDL_Color{42, 24, 12, 255});
                        renderer.drawRect(px + 2, py + 16, TILE_SIZE - 4, 4, SDL_Color{42, 24, 12, 255});
                        renderer.drawRect(px + 2, py + 24, TILE_SIZE - 4, 3, SDL_Color{42, 24, 12, 255});
                        renderer.drawRect(px + 5, py + 8, 4, 2, SDL_Color{85, 95, 120, 180});
                        renderer.drawRect(px + 18, py + 18, 5, 2, SDL_Color{85, 95, 120, 180});
                    }
                    continue;
                }

                if (tile.getType() == TileType::Fence) {
                    bool hasVert = (ty > 0 && getTile(tx, ty - 1).getType() == TileType::Fence) ||
                                   (ty < HEIGHT - 1 && getTile(tx, ty + 1).getType() == TileType::Fence);
                    bool hasHoriz = (tx > 0 && getTile(tx - 1, ty).getType() == TileType::Fence) ||
                                    (tx < WIDTH - 1 && getTile(tx + 1, ty).getType() == TileType::Fence);
                    bool isVertOnly = (hasVert && !hasHoriz);
                    const SDL_Rect fenceSrc{isVertOnly ? 32 : 0, 0, 32, 32};
                    if (!renderer.drawSprite("tiles_fence.png", px, py, TILE_SIZE, TILE_SIZE, &fenceSrc)) {
                        if (isVertOnly) {
                            renderer.drawRect(px + 4, py + 10, 24, 12, SDL_Color{150, 100, 50, 255});
                            renderer.drawRect(px + 10, py + 2, 4, 28, SDL_Color{130, 85, 40, 255});
                            renderer.drawRect(px + 20, py + 2, 4, 28, SDL_Color{130, 85, 40, 255});
                        } else {
                            renderer.drawRect(px + 10, py + 4, 12, 24, SDL_Color{150, 100, 50, 255});
                            renderer.drawRect(px + 2, py + 10, 28, 4, SDL_Color{130, 85, 40, 255});
                            renderer.drawRect(px + 2, py + 20, 28, 4, SDL_Color{130, 85, 40, 255});
                        }
                    }
                    continue;
                }

                if (tile.getType() == TileType::Farmhouse || tile.getType() == TileType::FarmhouseDoor) {
                    const SDL_Rect houseSrc{(tx - 1) * 32, (ty - 1) * 32, 32, 32};
                    if (!renderer.drawSprite("building_farmhouse.png", px, py, 32, 32, &houseSrc)) {
                        if (ty == 1) {
                            renderer.drawRect(px, py + 4, TILE_SIZE, TILE_SIZE - 4, SDL_Color{175, 55, 45, 255});
                            renderer.drawRect(px, py + 4, TILE_SIZE, 4, SDL_Color{205, 75, 60, 255});
                            renderer.drawOutline(px, py + 4, TILE_SIZE, TILE_SIZE - 4, SDL_Color{120, 35, 30, 255});
                            if (tx == 3) {
                                renderer.drawRect(px + 18, py, 8, 12, SDL_Color{150, 60, 45, 255});
                                renderer.drawRect(px + 16, py - 2, 12, 3, SDL_Color{90, 35, 25, 255});
                            }
                        } else {
                            renderer.drawRect(px, py, TILE_SIZE, TILE_SIZE, SDL_Color{165, 110, 60, 255});
                            renderer.drawOutline(px, py, TILE_SIZE, TILE_SIZE, SDL_Color{110, 65, 35, 255});
                            renderer.drawLine(px, py + 10, px + TILE_SIZE, py + 10, SDL_Color{125, 75, 40, 255});
                            renderer.drawLine(px, py + 20, px + TILE_SIZE, py + 20, SDL_Color{125, 75, 40, 255});
                            if (tx == 2) {
                                renderer.drawRect(px + 8, py + 6, 16, 26, SDL_Color{95, 50, 25, 255});
                                renderer.drawOutline(px + 8, py + 6, 16, 26, SDL_Color{65, 30, 15, 255});
                                renderer.drawRect(px + 20, py + 18, 3, 3, SDL_Color{245, 210, 50, 255});
                            } else {
                                renderer.drawRect(px + 7, py + 7, 18, 16, SDL_Color{255, 225, 120, 255});
                                renderer.drawOutline(px + 7, py + 7, 18, 16, SDL_Color{85, 45, 20, 255});
                                renderer.drawLine(px + 16, py + 7, px + 16, py + 23, SDL_Color{85, 45, 20, 255});
                                renderer.drawLine(px + 7, py + 15, px + 25, py + 15, SDL_Color{85, 45, 20, 255});
                            }
                        }
                    }
                    continue;
                }

                if (tile.getType() == TileType::ShopStall) {
                    const SDL_Rect stallSrc{((tx - 13) % 3) * 32, 0, 32, 64};
                    if (!renderer.drawSprite("building_shop_stall.png", px, py - 32, 32, 64, &stallSrc)) {
                        renderer.drawRect(px + 2, py + 18, 28, 12, SDL_Color{145, 95, 50, 255});
                        renderer.drawOutline(px + 2, py + 18, 28, 12, SDL_Color{105, 65, 30, 255});
                        renderer.drawRect(px + 4, py + 10, 4, 18, SDL_Color{120, 75, 35, 255});
                        renderer.drawRect(px + 24, py + 10, 4, 18, SDL_Color{120, 75, 35, 255});

                        bool isSeller = (tx <= 15);
                        SDL_Color primaryColor = isSeller ? SDL_Color{215, 45, 40, 255} : SDL_Color{45, 115, 200, 255};
                        SDL_Color stripeColor = isSeller ? SDL_Color{245, 240, 230, 255} : SDL_Color{250, 215, 65, 255};

                        renderer.drawRect(px, py + 2, TILE_SIZE, 12, primaryColor);
                        renderer.drawRect(px + 6, py + 2, 6, 12, stripeColor);
                        renderer.drawRect(px + 20, py + 2, 6, 12, stripeColor);
                        renderer.drawOutline(px, py + 2, TILE_SIZE, 12, SDL_Color{70, 50, 40, 255});

                        for (int f = 0; f < 4; ++f) {
                            SDL_Color fringeCol = (f % 2 == 0) ? primaryColor : stripeColor;
                            renderer.drawRect(px + f * 8, py + 14, 8, 3, fringeCol);
                        }
                    }
                    continue;
                }

                if (tile.getType() == TileType::Stove && ty == 2) {
                    const SDL_Rect stoveSrc{0, 0, 32, 64};
                    if (!renderer.drawSprite("building_stove.png", px, py - 32, 32, 64, &stoveSrc)) {
                        renderer.drawRect(px, py, TILE_SIZE, TILE_SIZE, SDL_Color{115, 110, 105, 255});
                        renderer.drawOutline(px, py, TILE_SIZE, TILE_SIZE, SDL_Color{85, 80, 75, 255});
                        renderer.drawRect(px + 4, py + 6, 24, 23, SDL_Color{48, 48, 52, 255});
                        renderer.drawOutline(px + 4, py + 6, 24, 23, SDL_Color{25, 25, 28, 255});
                        renderer.drawRect(px + 3, py + 4, 26, 4, SDL_Color{75, 75, 82, 255});
                        renderer.drawRect(px + 8, py + 15, 16, 11, SDL_Color{20, 20, 22, 255});
                        renderer.drawRect(px + 10, py + 17, 12, 7, SDL_Color{235, 85, 25, 255});
                        renderer.drawRect(px + 12, py + 19, 8, 4, SDL_Color{255, 200, 50, 255});
                        renderer.drawRect(px + 10, py + 1, 12, 6, SDL_Color{160, 165, 175, 255});
                    }
                    continue;
                }

                // Pure ground tiles (Grass, Path, Dirt, Water) already drawn cleanly without grid outlines
                continue;
            }

            // Interior Room Rendering (milestone 7)
            switch (tile.getType()) {
                case TileType::Void: {
                    renderer.drawRect(px, py, TILE_SIZE, TILE_SIZE, SDL_Color{16, 12, 14, 255});
                    break;
                }
                case TileType::WoodFloor: {
                    renderer.drawRect(px, py, TILE_SIZE, TILE_SIZE, SDL_Color{188, 138, 86, 255});
                    renderer.drawLine(px, py + 10, px + TILE_SIZE, py + 10, SDL_Color{150, 105, 60, 255});
                    renderer.drawLine(px + 16, py, px + 16, py + 10, SDL_Color{150, 105, 60, 255});
                    renderer.drawLine(px, py + 21, px + TILE_SIZE, py + 21, SDL_Color{150, 105, 60, 255});
                    break;
                }
                case TileType::Wall: {
                    renderer.drawRect(px, py, TILE_SIZE, TILE_SIZE, SDL_Color{105, 65, 38, 255});
                    renderer.drawRect(px + 2, py + 2, TILE_SIZE - 4, TILE_SIZE - 4, SDL_Color{125, 78, 46, 255});
                    renderer.drawOutline(px, py, TILE_SIZE, TILE_SIZE, SDL_Color{70, 42, 22, 255});
                    renderer.drawLine(px, py + 16, px + TILE_SIZE, py + 16, SDL_Color{85, 52, 28, 255});
                    break;
                }
                case TileType::Bed: {
                    renderer.drawRect(px, py, TILE_SIZE, TILE_SIZE, SDL_Color{188, 138, 86, 255});
                    renderer.drawRect(px + 3, py + 4, 26, 26, SDL_Color{115, 65, 32, 255});
                    renderer.drawRect(px + 5, py + 11, 22, 17, SDL_Color{185, 48, 42, 255});
                    renderer.drawRect(px + 5, py + 17, 22, 4, SDL_Color{220, 75, 65, 255});
                    renderer.drawRect(px + 7, py + 5, 18, 6, SDL_Color{248, 242, 230, 255});
                    renderer.drawOutline(px + 7, py + 5, 18, 6, SDL_Color{195, 185, 170, 255});
                    break;
                }
                case TileType::Chest: {
                    renderer.drawRect(px, py, TILE_SIZE, TILE_SIZE, SDL_Color{188, 138, 86, 255});
                    renderer.drawRect(px + 4, py + 8, 24, 20, SDL_Color{138, 82, 40, 255});
                    renderer.drawRect(px + 8, py + 8, 3, 20, SDL_Color{65, 65, 75, 255});
                    renderer.drawRect(px + 21, py + 8, 3, 20, SDL_Color{65, 65, 75, 255});
                    renderer.drawLine(px + 4, py + 15, px + 28, py + 15, SDL_Color{55, 32, 16, 255});
                    renderer.drawRect(px + 14, py + 14, 4, 5, SDL_Color{245, 210, 50, 255});
                    break;
                }
                case TileType::Stove: {
                    renderer.drawRect(px, py, TILE_SIZE, TILE_SIZE, SDL_Color{188, 138, 86, 255});
                    const SDL_Rect stoveSrc{0, 0, 32, 64};
                    if (!renderer.drawSprite("building_stove.png", px, py - 32, 32, 64, &stoveSrc)) {
                        renderer.drawRect(px + 4, py + 6, 24, 23, SDL_Color{48, 48, 52, 255});
                        renderer.drawOutline(px + 4, py + 6, 24, 23, SDL_Color{25, 25, 28, 255});
                        renderer.drawRect(px + 3, py + 4, 26, 4, SDL_Color{75, 75, 82, 255});
                        renderer.drawRect(px + 8, py + 15, 16, 11, SDL_Color{20, 20, 22, 255});
                        renderer.drawRect(px + 10, py + 17, 12, 7, SDL_Color{235, 85, 25, 255});
                    }
                    break;
                }
                case TileType::Bookshelf: {
                    renderer.drawRect(px, py, TILE_SIZE, TILE_SIZE, SDL_Color{188, 138, 86, 255});
                    renderer.drawRect(px + 3, py + 2, 26, 28, SDL_Color{95, 55, 28, 255});
                    renderer.drawOutline(px + 3, py + 2, 26, 28, SDL_Color{65, 35, 18, 255});
                    renderer.drawRect(px + 5, py + 15, 22, 3, SDL_Color{125, 75, 40, 255});
                    renderer.drawRect(px + 6, py + 5, 4, 10, SDL_Color{195, 50, 45, 255});
                    renderer.drawRect(px + 11, py + 6, 4, 9, SDL_Color{45, 120, 190, 255});
                    renderer.drawRect(px + 16, py + 4, 5, 11, SDL_Color{45, 150, 75, 255});
                    renderer.drawRect(px + 22, py + 7, 3, 8, SDL_Color{220, 185, 45, 255});
                    renderer.drawRect(px + 6, py + 18, 5, 10, SDL_Color{160, 60, 160, 255});
                    renderer.drawRect(px + 12, py + 19, 4, 9, SDL_Color{210, 110, 35, 255});
                    renderer.drawRect(px + 17, py + 18, 6, 10, SDL_Color{60, 130, 180, 255});
                    break;
                }
                case TileType::InteriorExit: {
                    renderer.drawRect(px, py, TILE_SIZE, TILE_SIZE, SDL_Color{188, 138, 86, 255});
                    renderer.drawRect(px + 3, py + 6, 26, 20, SDL_Color{165, 55, 45, 255});
                    renderer.drawOutline(px + 3, py + 6, 26, 20, SDL_Color{235, 205, 120, 255});
                    renderer.drawRect(px + 7, py + 10, 18, 12, SDL_Color{195, 75, 60, 255});
                    renderer.drawRect(px + 14, py + 12, 4, 8, SDL_Color{255, 235, 160, 255});
                    renderer.drawRect(px + 12, py + 18, 8, 3, SDL_Color{255, 235, 160, 255});
                    break;
                }
                default:
                    break;
            }
        }
    }

    // Draw hanging wooden shop signs at the outdoor market stalls
    if (getTile(13, 1).getType() == TileType::ShopStall) {
        renderer.drawSprite("ui_shop_sign.png", 432, 14, 64, 32);
        renderer.drawSprite("ui_shop_sign.png", 528, 14, 64, 32);
    }
}
