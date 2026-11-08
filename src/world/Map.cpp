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

    // 3. Central farming soil plot (8 wide x 6 tall)
    for (int y = 4; y <= 9; ++y) {
        for (int x = 5; x <= 12; ++x) {
            m_tiles[getIndex(x, y)] = Tile(TileType::Dirt);
        }
    }

    // Start with a couple of already tilled patches to make the farm inviting
    m_tiles[getIndex(6, 5)] = Tile(TileType::TilledSoil);
    m_tiles[getIndex(7, 5)] = Tile(TileType::TilledSoil);

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

    // 6. Kitchen Stove interaction point at (4, 2) on the farmhouse patio
    m_tiles[getIndex(4, 2)] = Tile(TileType::Stove);

    // 7. Market Shop Stalls in top-right (x=13..15 for Seller stall, x=16..18 for Buyer stall at y=1)
    for (int x = 13; x <= 18; ++x) {
        m_tiles[getIndex(x, 1)] = Tile(TileType::ShopStall);
    }
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

void Map::render(Renderer& renderer) const {
    for (int ty = 0; ty < HEIGHT; ++ty) {
        for (int tx = 0; tx < WIDTH; ++tx) {
            const int px = tx * TILE_SIZE;
            const int py = ty * TILE_SIZE;
            const Tile& tile = getTile(tx, ty);

            // Source cells follow ASSETS_NOTES.md; collision/interaction tiles stay unchanged.
            SDL_Rect source{0, 0, 32, 32};
            const char* sprite = "tiles_ground.png";
            switch (tile.getType()) {
                case TileType::Dirt: break;
                case TileType::TilledSoil: source.x = 32; break;
                case TileType::Water: source.x = 32; source.y = 32; break;
                case TileType::WateredSoil: sprite = "tile_tilled_watered.png"; break;
                case TileType::Fence: sprite = "tiles_fence.png"; break;
                case TileType::Farmhouse:
                    sprite = "building_farmhouse.png";
                    source = {(tx - 1) * 32, (ty - 1) * 32, 32, 32};
                    break;
                case TileType::ShopStall:
                    sprite = "building_shop_stall.png";
                    source = {((tx - 13) % 3) * 32, 0, 32, 64};
                    break;
                case TileType::Stove: sprite = "building_stove.png"; source.h = 64; break;
                case TileType::Grass: source.y = 32; break;
            }
            if (tile.getType() == TileType::Fence || tile.getType() == TileType::Farmhouse ||
                tile.getType() == TileType::ShopStall || tile.getType() == TileType::Stove) {
                const SDL_Rect grass{0, 32, 32, 32};
                renderer.drawSprite("tiles_ground.png", px, py, 32, 32, &grass);
            }
            if (renderer.drawSprite(sprite, px, py + 32 - source.h, 32, source.h, &source)) continue;

            switch (tile.getType()) {
                case TileType::Grass: {
                    // Bright, warm green base
                    renderer.drawRect(px, py, TILE_SIZE, TILE_SIZE, SDL_Color{88, 155, 60, 255});
                    // Subtle grass blades
                    renderer.drawRect(px + 6, py + 8, 3, 5, SDL_Color{108, 178, 76, 255});
                    renderer.drawRect(px + 20, py + 18, 4, 6, SDL_Color{108, 178, 76, 255});
                    break;
                }
                case TileType::Dirt: {
                    // Warm earthy brown
                    renderer.drawRect(px, py, TILE_SIZE, TILE_SIZE, SDL_Color{142, 98, 59, 255});
                    // Subtle pebbles
                    renderer.drawRect(px + 8, py + 10, 3, 2, SDL_Color{118, 78, 42, 255});
                    renderer.drawRect(px + 22, py + 20, 2, 2, SDL_Color{160, 115, 75, 255});
                    break;
                }
                case TileType::TilledSoil: {
                    // Rich darker brown with tilled furrows
                    renderer.drawRect(px, py, TILE_SIZE, TILE_SIZE, SDL_Color{96, 60, 36, 255});
                    renderer.drawRect(px + 2, py + 6, TILE_SIZE - 4, 4, SDL_Color{74, 45, 26, 255});
                    renderer.drawRect(px + 2, py + 16, TILE_SIZE - 4, 4, SDL_Color{74, 45, 26, 255});
                    renderer.drawRect(px + 2, py + 24, TILE_SIZE - 4, 3, SDL_Color{74, 45, 26, 255});
                    break;
                }
                case TileType::WateredSoil: {
                    // Dark damp brown
                    renderer.drawRect(px, py, TILE_SIZE, TILE_SIZE, SDL_Color{58, 36, 20, 255});
                    renderer.drawRect(px + 2, py + 6, TILE_SIZE - 4, 4, SDL_Color{42, 24, 12, 255});
                    renderer.drawRect(px + 2, py + 16, TILE_SIZE - 4, 4, SDL_Color{42, 24, 12, 255});
                    renderer.drawRect(px + 2, py + 24, TILE_SIZE - 4, 3, SDL_Color{42, 24, 12, 255});
                    // Water sheen highlight
                    renderer.drawRect(px + 5, py + 8, 4, 2, SDL_Color{85, 95, 120, 180});
                    renderer.drawRect(px + 18, py + 18, 5, 2, SDL_Color{85, 95, 120, 180});
                    break;
                }
                case TileType::Water: {
                    // Blue pond
                    renderer.drawRect(px, py, TILE_SIZE, TILE_SIZE, SDL_Color{60, 130, 200, 255});
                    // Water ripple highlight
                    renderer.drawRect(px + 4, py + 8, 12, 2, SDL_Color{120, 185, 240, 220});
                    renderer.drawRect(px + 14, py + 20, 10, 2, SDL_Color{120, 185, 240, 220});
                    break;
                }
                case TileType::Fence: {
                    // Grass background with wooden fence post
                    renderer.drawRect(px, py, TILE_SIZE, TILE_SIZE, SDL_Color{88, 155, 60, 255});
                    renderer.drawRect(px + 10, py + 4, 12, 24, SDL_Color{150, 100, 50, 255});
                    renderer.drawRect(px + 2, py + 10, 28, 4, SDL_Color{130, 85, 40, 255});
                    renderer.drawRect(px + 2, py + 20, 28, 4, SDL_Color{130, 85, 40, 255});
                    break;
                }
                case TileType::Farmhouse: {
                    // Grass base under building edges
                    renderer.drawRect(px, py, TILE_SIZE, TILE_SIZE, SDL_Color{88, 155, 60, 255});

                    if (ty == 1) {
                        // Upper roof tier: rich terracotta red roof shingles
                        renderer.drawRect(px, py + 4, TILE_SIZE, TILE_SIZE - 4, SDL_Color{175, 55, 45, 255});
                        renderer.drawRect(px, py + 4, TILE_SIZE, 4, SDL_Color{205, 75, 60, 255});
                        renderer.drawOutline(px, py + 4, TILE_SIZE, TILE_SIZE - 4, SDL_Color{120, 35, 30, 255});
                        // Chimney on top right of roof (tile x=3, y=1)
                        if (tx == 3) {
                            renderer.drawRect(px + 18, py, 8, 12, SDL_Color{150, 60, 45, 255});
                            renderer.drawRect(px + 16, py - 2, 12, 3, SDL_Color{90, 35, 25, 255});
                        }
                    } else {
                        // Lower wall tier: warm wood planks
                        renderer.drawRect(px, py, TILE_SIZE, TILE_SIZE, SDL_Color{165, 110, 60, 255});
                        renderer.drawOutline(px, py, TILE_SIZE, TILE_SIZE, SDL_Color{110, 65, 35, 255});
                        renderer.drawLine(px, py + 10, px + TILE_SIZE, py + 10, SDL_Color{125, 75, 40, 255});
                        renderer.drawLine(px, py + 20, px + TILE_SIZE, py + 20, SDL_Color{125, 75, 40, 255});

                        // Tile (2, 2) is the farmhouse entrance door
                        if (tx == 2) {
                            renderer.drawRect(px + 8, py + 6, 16, 26, SDL_Color{95, 50, 25, 255});
                            renderer.drawOutline(px + 8, py + 6, 16, 26, SDL_Color{65, 30, 15, 255});
                            // Brass door knob
                            renderer.drawRect(px + 20, py + 18, 3, 3, SDL_Color{245, 210, 50, 255});
                        } else {
                            // Cozy warm window with cross pane
                            renderer.drawRect(px + 7, py + 7, 18, 16, SDL_Color{255, 225, 120, 255});
                            renderer.drawOutline(px + 7, py + 7, 18, 16, SDL_Color{85, 45, 20, 255});
                            renderer.drawLine(px + 16, py + 7, px + 16, py + 23, SDL_Color{85, 45, 20, 255});
                            renderer.drawLine(px + 7, py + 15, px + 25, py + 15, SDL_Color{85, 45, 20, 255});
                        }
                    }
                    break;
                }
                case TileType::Stove: {
                    // Kitchen stone patio foundation
                    renderer.drawRect(px, py, TILE_SIZE, TILE_SIZE, SDL_Color{115, 110, 105, 255});
                    renderer.drawOutline(px, py, TILE_SIZE, TILE_SIZE, SDL_Color{85, 80, 75, 255});

                    // Cast-iron stove body
                    renderer.drawRect(px + 4, py + 6, 24, 23, SDL_Color{48, 48, 52, 255});
                    renderer.drawOutline(px + 4, py + 6, 24, 23, SDL_Color{25, 25, 28, 255});

                    // Top range surface
                    renderer.drawRect(px + 3, py + 4, 26, 4, SDL_Color{75, 75, 82, 255});

                    // Fiery glowing stove door with embers
                    renderer.drawRect(px + 8, py + 15, 16, 11, SDL_Color{20, 20, 22, 255});
                    renderer.drawRect(px + 10, py + 17, 12, 7, SDL_Color{235, 85, 25, 255});
                    renderer.drawRect(px + 12, py + 19, 8, 4, SDL_Color{255, 200, 50, 255});

                    // Metal cooking pot on stove top with rising steam
                    renderer.drawRect(px + 10, py + 1, 12, 6, SDL_Color{160, 165, 175, 255});
                    renderer.drawRect(px + 8, py + 3, 2, 2, SDL_Color{130, 135, 145, 255});
                    renderer.drawRect(px + 22, py + 3, 2, 2, SDL_Color{130, 135, 145, 255});
                    // Steam puffs
                    renderer.drawRect(px + 12, py - 2, 2, 2, SDL_Color{230, 235, 245, 200});
                    renderer.drawRect(px + 16, py - 4, 3, 2, SDL_Color{230, 235, 245, 200});
                    break;
                }
                case TileType::ShopStall: {
                    // Grass base under stall
                    renderer.drawRect(px, py, TILE_SIZE, TILE_SIZE, SDL_Color{88, 155, 60, 255});

                    // Wooden counter table surface & support posts
                    renderer.drawRect(px + 2, py + 18, 28, 12, SDL_Color{145, 95, 50, 255});
                    renderer.drawOutline(px + 2, py + 18, 28, 12, SDL_Color{105, 65, 30, 255});
                    renderer.drawRect(px + 4, py + 10, 4, 18, SDL_Color{120, 75, 35, 255});
                    renderer.drawRect(px + 24, py + 10, 4, 18, SDL_Color{120, 75, 35, 255});

                    // Festive market awning canopy
                    bool isSeller = (tx <= 15);
                    SDL_Color primaryColor = isSeller ? SDL_Color{215, 45, 40, 255} : SDL_Color{45, 115, 200, 255};
                    SDL_Color stripeColor = isSeller ? SDL_Color{245, 240, 230, 255} : SDL_Color{250, 215, 65, 255};

                    // Draw striped canopy top
                    renderer.drawRect(px, py + 2, TILE_SIZE, 12, primaryColor);
                    renderer.drawRect(px + 6, py + 2, 6, 12, stripeColor);
                    renderer.drawRect(px + 20, py + 2, 6, 12, stripeColor);
                    renderer.drawOutline(px, py + 2, TILE_SIZE, 12, SDL_Color{70, 50, 40, 255});

                    // Scalloped canopy drape fringe
                    for (int f = 0; f < 4; ++f) {
                        SDL_Color fringeCol = (f % 2 == 0) ? primaryColor : stripeColor;
                        renderer.drawRect(px + f * 8, py + 14, 8, 3, fringeCol);
                    }
                    break;
                }
            }

            // Draw faint grid borders for readability in prototype mode
            renderer.drawOutline(px, py, TILE_SIZE, TILE_SIZE, SDL_Color{0, 0, 0, 25});
        }
    }
}
