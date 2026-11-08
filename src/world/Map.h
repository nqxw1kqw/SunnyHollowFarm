#pragma once

#include "world/Tile.h"
#include "core/Renderer.h"
#include <array>

// 20x15 grid of 32x32 tiles representing the farm world.
class Map {
public:
    static constexpr int WIDTH = 20;
    static constexpr int HEIGHT = 15;
    static constexpr int TILE_SIZE = 32;

    Map();

    // Sets up the initial cozy farm layout with grass, tillable dirt plot, pond, and fences.
    void initDefault();

    // Sets up the farmhouse interior room with bed, indoor stove, storage chest, bookshelf, and exit.
    void initInterior();

    bool inBounds(int tileX, int tileY) const;

    Tile& getTile(int tileX, int tileY);
    const Tile& getTile(int tileX, int tileY) const;

    void setTileType(int tileX, int tileY, TileType type);

    // Collision check: returns true if the bounding rectangle can walk here.
    bool isAreaWalkable(float x, float y, float w, float h) const;

    // Renders the entire tile map using colored placeholder rectangles.
    void render(Renderer& renderer) const;

private:
    std::array<Tile, WIDTH * HEIGHT> m_tiles;

    int getIndex(int tileX, int tileY) const {
        return tileY * WIDTH + tileX;
    }
};
