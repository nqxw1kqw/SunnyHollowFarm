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

bool FarmSystem::till(int tileX, int tileY, Map& map, Player& player, HUD* hud) {
    if (!map.inBounds(tileX, tileY)) return false;

    Tile& tile = map.getTile(tileX, tileY);
    if (!tile.isTillable()) {
        return false;
    }

    // Energy check
    if (player.getEnergy() < Config::ENERGY_COST_TILL) {
        if (hud) {
            hud->showFeedback("NOT ENOUGH ENERGY!");
        }
        std::cout << "[Farm] Action refused: Not enough energy to till! (Need "
                  << Config::ENERGY_COST_TILL << ", have " << player.getEnergy() << ")" << std::endl;
        return false;
    }

    tile.setType(TileType::TilledSoil);
    player.consumeEnergy(Config::ENERGY_COST_TILL);
    std::cout << "[Farm] Tilled soil at (" << tileX << ", " << tileY
              << ") [Energy: " << player.getEnergy() << "/" << player.getMaxEnergy() << "]" << std::endl;
    return true;
}

bool FarmSystem::water(int tileX, int tileY, Map& map, Player& player, HUD* hud) {
    if (!map.inBounds(tileX, tileY)) return false;

    Tile& tile = map.getTile(tileX, tileY);
    if (tile.getType() != TileType::TilledSoil) {
        return false;
    }

    // Energy check
    if (player.getEnergy() < Config::ENERGY_COST_WATER) {
        if (hud) {
            hud->showFeedback("NOT ENOUGH ENERGY!");
        }
        std::cout << "[Farm] Action refused: Not enough energy to water! (Need "
                  << Config::ENERGY_COST_WATER << ", have " << player.getEnergy() << ")" << std::endl;
        return false;
    }

    tile.setType(TileType::WateredSoil);
    player.consumeEnergy(Config::ENERGY_COST_WATER);
    std::cout << "[Farm] Watered soil at (" << tileX << ", " << tileY
              << ") [Growth Boost Active] [Energy: " << player.getEnergy()
              << "/" << player.getMaxEnergy() << "]" << std::endl;
    return true;
}

bool FarmSystem::plant(int tileX, int tileY, Map& map, Inventory& inventory, Player& player, HUD* hud, ItemType seedType) {
    if (!map.inBounds(tileX, tileY)) return false;

    // Determine seed to plant
    if (seedType == ItemType::Empty) {
        if (inventory.activeTool == ToolType::WheatSeeds) {
            seedType = ItemType::WheatSeeds;
        } else {
            seedType = ItemType::TomatoSeeds;
        }
    }

    if (inventory.getItemCount(seedType) <= 0) {
        std::cout << "[Farm] Out of " << itemTypeToString(seedType) << "!" << std::endl;
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

    // Energy check
    if (player.getEnergy() < Config::ENERGY_COST_PLANT) {
        if (hud) {
            hud->showFeedback("NOT ENOUGH ENERGY!");
        }
        std::cout << "[Farm] Action refused: Not enough energy to plant! (Need "
                  << Config::ENERGY_COST_PLANT << ", have " << player.getEnergy() << ")" << std::endl;
        return false;
    }

    ItemType cropProduce = (seedType == ItemType::WheatSeeds) ? ItemType::Wheat : ItemType::Tomato;
    m_crops.emplace_back(tileX, tileY, cropProduce);
    inventory.removeItem(seedType, 1);
    player.consumeEnergy(Config::ENERGY_COST_PLANT);

    std::cout << "[Farm] Planted " << itemTypeToString(seedType) << " at (" << tileX << ", " << tileY
              << ") [Seeds remaining: " << inventory.getItemCount(seedType)
              << "] [Energy: " << player.getEnergy() << "/" << player.getMaxEnergy() << "]" << std::endl;
    return true;
}

bool FarmSystem::harvest(int tileX, int tileY, Map& map, Inventory& inventory, Player& player, HUD* hud) {
    (void)map;
    auto it = std::find_if(m_crops.begin(), m_crops.end(), [tileX, tileY](const Crop& c) {
        return c.getTileX() == tileX && c.getTileY() == tileY;
    });

    if (it == m_crops.end() || !it->isMature()) {
        return false;
    }

    // Energy check
    if (player.getEnergy() < Config::ENERGY_COST_HARVEST) {
        if (hud) {
            hud->showFeedback("NOT ENOUGH ENERGY!");
        }
        std::cout << "[Farm] Action refused: Not enough energy to harvest! (Need "
                  << Config::ENERGY_COST_HARVEST << ", have " << player.getEnergy() << ")" << std::endl;
        return false;
    }

    ItemType harvestItem = it->getHarvestItem();
    m_crops.erase(it);
    inventory.addItem(harvestItem, 1);
    player.consumeEnergy(Config::ENERGY_COST_HARVEST);

    std::cout << "[Farm] Harvested ripe " << itemTypeToString(harvestItem)
              << "! [Total " << itemTypeToString(harvestItem) << " in inventory: "
              << inventory.getItemCount(harvestItem)
              << "] [Energy: " << player.getEnergy() << "/" << player.getMaxEnergy() << "]" << std::endl;
    return true;
}

bool FarmSystem::interact(int tileX, int tileY, Map& map, Inventory& inventory, Player& player, HUD* hud) {
    if (!map.inBounds(tileX, tileY)) return false;

    // 1. If there is a mature crop ready to pick, harvest it immediately
    Crop* existingCrop = findCrop(tileX, tileY);
    if (existingCrop && existingCrop->isMature()) {
        return harvest(tileX, tileY, map, inventory, player, hud);
    }

    // 2. Otherwise apply the currently selected tool
    switch (inventory.activeTool) {
        case ToolType::Hoe:
            return till(tileX, tileY, map, player, hud);

        case ToolType::WateringCan:
            return water(tileX, tileY, map, player, hud);

        case ToolType::TomatoSeeds:
            return plant(tileX, tileY, map, inventory, player, hud, ItemType::TomatoSeeds);

        case ToolType::WheatSeeds:
            return plant(tileX, tileY, map, inventory, player, hud, ItemType::WheatSeeds);
    }

    return false;
}

void FarmSystem::advanceDay(Map& map) {
    // 1. Advance all active crops overnight by 1 growth stage
    for (auto& crop : m_crops) {
        crop.advanceStage();
    }

    // 2. Dry watered soil back to regular tilled soil under standard overnight drying rule
    for (int y = 0; y < Map::HEIGHT; ++y) {
        for (int x = 0; x < Map::WIDTH; ++x) {
            if (map.inBounds(x, y) && map.getTile(x, y).getType() == TileType::WateredSoil) {
                map.setTileType(x, y, TileType::TilledSoil);
            }
        }
    }

    std::cout << "[Farm] Overnight growth complete: crops advanced 1 stage, watered soil dried back to tilled soil." << std::endl;
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
