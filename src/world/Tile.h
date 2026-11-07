#pragma once

#include "world/TileType.h"

// Encapsulates state for a single map cell.
class Tile {
public:
    explicit Tile(TileType type = TileType::Grass)
        : m_type(type) {}

    TileType getType() const { return m_type; }
    void setType(TileType type) { m_type = type; }

    // Impassable tiles block player movement.
    bool isWalkable() const {
        return m_type != TileType::Water && m_type != TileType::Fence;
    }

    // Dirt or grass can be tilled using a hoe.
    bool isTillable() const {
        return m_type == TileType::Grass || m_type == TileType::Dirt;
    }

    // Tilled or watered soil can host crops.
    bool isTilled() const {
        return m_type == TileType::TilledSoil || m_type == TileType::WateredSoil;
    }

    // Soil is currently moist.
    bool isWatered() const {
        return m_type == TileType::WateredSoil;
    }

private:
    TileType m_type = TileType::Grass;
};
