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
            if (!getTile(tx, ty).isWalkable()) {
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
            }

            // Draw faint grid borders for readability in prototype mode
            renderer.drawOutline(px, py, TILE_SIZE, TILE_SIZE, SDL_Color{0, 0, 0, 25});
        }
    }
}
