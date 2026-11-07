#pragma once

#include "world/Map.h"
#include "entities/Crop.h"
#include "entities/Inventory.h"
#include "core/Renderer.h"
#include <vector>

// Core farming simulation system: handles tilling, watering, planting, growing, and harvesting.
class FarmSystem {
public:
    FarmSystem() = default;

    // Checks if a crop is planted at the given tile coordinates.
    bool hasCrop(int tileX, int tileY) const;
    Crop* findCrop(int tileX, int tileY);
    const Crop* findCrop(int tileX, int tileY) const;

    // Tills grassy or dry dirt into farmable tilled soil.
    bool till(int tileX, int tileY, Map& map);

    // Waters tilled soil, speeding up plant growth.
    bool water(int tileX, int tileY, Map& map);

    // Plants a tomato seed from inventory onto tilled soil.
    bool plant(int tileX, int tileY, Map& map, Inventory& inventory);

    // Harvests a ripe crop, adding a tomato to inventory.
    bool harvest(int tileX, int tileY, Map& map, Inventory& inventory);

    // Context-sensitive interaction using the player's active tool or harvesting mature crops.
    bool interact(int tileX, int tileY, Map& map, Inventory& inventory);

    // Advances growth of all planted crops. Watered tiles yield a 2.5x growth rate boost.
    void update(float dt, Map& map);

    // Renders all active crops onto the map.
    void render(Renderer& renderer) const;

    size_t getCropCount() const { return m_crops.size(); }
    const std::vector<Crop>& getCrops() const { return m_crops; }

private:
    std::vector<Crop> m_crops;
};
