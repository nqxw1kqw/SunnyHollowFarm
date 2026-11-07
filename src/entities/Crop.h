#pragma once

#include "core/Renderer.h"

// 4 distinct growth stages for crops.
enum class CropStage {
    Seed = 1,     // Freshly planted seed in the ground
    Sprout = 2,   // Tiny green seedling sprouting from soil
    Growing = 3,  // Vegetative bush with small yellow flower buds
    Harvest = 4   // Fully mature plant bearing ripe red tomatoes
};

// Represents an individual crop planted in a specific tile cell.
class Crop {
public:
    // Base seconds needed to advance each stage without watering.
    static constexpr float TIME_PER_STAGE = 6.0f;

    Crop(int tileX, int tileY);

    int getTileX() const { return m_tileX; }
    int getTileY() const { return m_tileY; }

    CropStage getStage() const { return m_stage; }
    bool isMature() const { return m_stage == CropStage::Harvest; }

    // Advances the crop growth timer.
    // speedMultiplier is > 1.0 when the soil is watered.
    void update(float dt, float speedMultiplier);

    // Renders the pixel placeholder art corresponding to the current stage.
    void render(Renderer& renderer) const;

private:
    int m_tileX = 0;
    int m_tileY = 0;
    CropStage m_stage = CropStage::Seed;
    float m_stageTimer = 0.0f;
};
