#pragma once

#include "world/Map.h"
#include "entities/Crop.h"
#include "entities/Inventory.h"
#include "entities/Player.h"
#include "ui/HUD.h"
#include "core/Renderer.h"
#include <vector>

// Core farming simulation system: handles tilling, watering, planting, growing, and harvesting.
// Enforces energy costs and provides feedback on exhausted stamina.
class FarmSystem {
public:
    FarmSystem() = default;

    // Checks if a crop is planted at the given tile coordinates.
    bool hasCrop(int tileX, int tileY) const;
    Crop* findCrop(int tileX, int tileY);
    const Crop* findCrop(int tileX, int tileY) const;

    // Tills grassy or dry dirt into farmable tilled soil (costs Config::ENERGY_COST_TILL).
    bool till(int tileX, int tileY, Map& map, Player& player, HUD* hud = nullptr);

    // Waters tilled soil, speeding up plant growth (costs Config::ENERGY_COST_WATER).
    bool water(int tileX, int tileY, Map& map, Player& player, HUD* hud = nullptr);

    // Plants a tomato seed from inventory onto tilled soil (costs Config::ENERGY_COST_PLANT).
    bool plant(int tileX, int tileY, Map& map, Inventory& inventory, Player& player, HUD* hud = nullptr);

    // Harvests a ripe crop, adding a tomato to inventory (costs Config::ENERGY_COST_HARVEST).
    bool harvest(int tileX, int tileY, Map& map, Inventory& inventory, Player& player, HUD* hud = nullptr);

    // Context-sensitive interaction using the player's active tool or harvesting mature crops.
    bool interact(int tileX, int tileY, Map& map, Inventory& inventory, Player& player, HUD* hud = nullptr);

    // Advances growth of all planted crops. Watered tiles yield a 2.5x growth rate boost.
    void update(float dt, Map& map);

    // Renders all active crops onto the map.
    void render(Renderer& renderer) const;

    size_t getCropCount() const { return m_crops.size(); }
    const std::vector<Crop>& getCrops() const { return m_crops; }

private:
    std::vector<Crop> m_crops;
};
