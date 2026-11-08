#pragma once

#include "core/Renderer.h"

#include "entities/Item.h"

// 4 distinct growth stages for crops.
enum class CropStage {
    Seed = 1,     // Freshly planted seed in the ground
    Sprout = 2,   // Tiny green seedling sprouting from soil
    Growing = 3,  // Vegetative bush with small yellow flower buds
    Harvest = 4   // Fully mature plant bearing ripe red tomatoes / golden wheat
};

// Represents an individual crop planted in a specific tile cell.
class Crop {
public:
    // Base seconds needed to advance each stage without watering.
    static constexpr float TIME_PER_STAGE = 6.0f;

    Crop(int tileX, int tileY, ItemType cropType = ItemType::Tomato);

    int getTileX() const { return m_tileX; }
    int getTileY() const { return m_tileY; }

    ItemType getCropType() const { return m_cropType; }
    ItemType getHarvestItem() const {
        if (m_cropType == ItemType::Wheat || m_cropType == ItemType::WheatSeeds) return ItemType::Wheat;
        if (m_cropType == ItemType::Carrot || m_cropType == ItemType::CarrotSeeds) return ItemType::Carrot;
        return ItemType::Tomato;
    }

    CropStage getStage() const { return m_stage; }
    bool isMature() const { return m_stage == CropStage::Harvest; }

    // Advances the crop stage directly (e.g. overnight sleep growth in Milestone 6)
    void advanceStage();

    // Advances the crop growth timer.
    // speedMultiplier is > 1.0 when the soil is watered.
    void update(float dt, float speedMultiplier);

    // Renders the pixel placeholder art corresponding to the current stage.
    void render(Renderer& renderer) const;

private:
    int m_tileX = 0;
    int m_tileY = 0;
    ItemType m_cropType = ItemType::Tomato;
    CropStage m_stage = CropStage::Seed;
    float m_stageTimer = 0.0f;
};
