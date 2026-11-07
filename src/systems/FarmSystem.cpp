#include "systems/FarmSystem.h"
#include <iostream>
#include <algorithm>

bool FarmSystem::hasCrop(int tileX, int tileY) const {
    return findCrop(tileX, tileY) != nullptr;
}

Crop* FarmSystem::findCrop(int tileX, int tileY) {
    for (auto& crop : m_crops) {
        if (crop.getTileX() == tileX && crop.getTileY() == tileY) {
            return &crop;
        }
    }
    return nullptr;
}

const Crop* FarmSystem::findCrop(int tileX, int tileY) const {
    for (const auto& crop : m_crops) {
        if (crop.getTileX() == tileX && crop.getTileY() == tileY) {
            return &crop;
        }
    }
    return nullptr;
}

bool FarmSystem::till(int tileX, int tileY, Map& map) {
    if (!map.inBounds(tileX, tileY)) return false;

    Tile& tile = map.getTile(tileX, tileY);
    if (tile.isTillable()) {
        tile.setType(TileType::TilledSoil);
        std::cout << "[Farm] Tilled soil at (" << tileX << ", " << tileY << ")" << std::endl;
        return true;
    }
    return false;
}

bool FarmSystem::water(int tileX, int tileY, Map& map) {
    if (!map.inBounds(tileX, tileY)) return false;

    Tile& tile = map.getTile(tileX, tileY);
    if (tile.getType() == TileType::TilledSoil) {
        tile.setType(TileType::WateredSoil);
        std::cout << "[Farm] Watered soil at (" << tileX << ", " << tileY << ") [Growth Boost Active]" << std::endl;
        return true;
    }
    return false;
}

bool FarmSystem::plant(int tileX, int tileY, Map& map, Inventory& inventory) {
    if (!map.inBounds(tileX, tileY)) return false;
    if (inventory.tomatoSeeds <= 0) {
        std::cout << "[Farm] Out of tomato seeds!" << std::endl;
        return false;
    }

    Tile& tile = map.getTile(tileX, tileY);
    if (!tile.isTilled()) {
        std::cout << "[Farm] Soil must be tilled first before planting!" << std::endl;
        return false;
    }

    if (hasCrop(tileX, tileY)) {
        std::cout << "[Farm] Tile already has a plant!" << std::endl;
        return false;
    }

    m_crops.emplace_back(tileX, tileY);
    inventory.tomatoSeeds--;
    std::cout << "[Farm] Planted tomato seed at (" << tileX << ", " << tileY
              << ") [Seeds remaining: " << inventory.tomatoSeeds << "]" << std::endl;
    return true;
}

bool FarmSystem::harvest(int tileX, int tileY, Map& map, Inventory& inventory) {
    (void)map;
    auto it = std::find_if(m_crops.begin(), m_crops.end(), [tileX, tileY](const Crop& c) {
        return c.getTileX() == tileX && c.getTileY() == tileY;
    });

    if (it != m_crops.end() && it->isMature()) {
        m_crops.erase(it);
        inventory.harvestedTomatoes++;
        std::cout << "[Farm] Harvested ripe tomato! [Total tomatoes in inventory: "
                  << inventory.harvestedTomatoes << "]" << std::endl;
        return true;
    }
    return false;
}

bool FarmSystem::interact(int tileX, int tileY, Map& map, Inventory& inventory) {
    if (!map.inBounds(tileX, tileY)) return false;

    // 1. If there is a mature crop ready to pick, harvest it immediately
    Crop* existingCrop = findCrop(tileX, tileY);
    if (existingCrop && existingCrop->isMature()) {
        return harvest(tileX, tileY, map, inventory);
    }

    // 2. Otherwise apply the currently selected tool
    switch (inventory.activeTool) {
        case ToolType::Hoe:
            return till(tileX, tileY, map);

        case ToolType::WateringCan:
            return water(tileX, tileY, map);

        case ToolType::TomatoSeeds:
            return plant(tileX, tileY, map, inventory);
    }

    return false;
}

void FarmSystem::update(float dt, Map& map) {
    for (auto& crop : m_crops) {
        float speedMultiplier = 1.0f;
        if (map.inBounds(crop.getTileX(), crop.getTileY())) {
            const Tile& tile = map.getTile(crop.getTileX(), crop.getTileY());
            // Watered soil provides a 2.5x growth rate boost
            if (tile.isWatered()) {
                speedMultiplier = 2.5f;
            }
        }
        crop.update(dt, speedMultiplier);
    }
}

void FarmSystem::render(Renderer& renderer) const {
    for (const auto& crop : m_crops) {
        crop.render(renderer);
    }
}
