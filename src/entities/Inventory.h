#pragma once

// Types of tools the farmer can hold.
enum class ToolType {
    Hoe = 0,
    WateringCan = 1,
    TomatoSeeds = 2
};

// Minimal inventory tracking seed stock, harvested produce, and the active tool.
struct Inventory {
    int tomatoSeeds = 6;
    int harvestedTomatoes = 0;
    ToolType activeTool = ToolType::Hoe;
};
