#pragma once

// Centralized game configuration and tuning parameters for Sunny Hollow Farm.
namespace Config {
    // Window & Map dimensions
    constexpr int SCREEN_WIDTH = 1600;
    constexpr int SCREEN_HEIGHT = 900;
    constexpr int TILE_SIZE = 32;
    constexpr int MAP_COLS = 20;
    constexpr int MAP_ROWS = 15;

    // Player starting stats & limits
    constexpr float PLAYER_MOVE_SPEED = 120.0f; // Pixels per second
    constexpr int INITIAL_MONEY = 100;    // Sensible starting coin wallet
    constexpr int INITIAL_ENERGY = 100;   // Max starting stamina
    constexpr int MAX_ENERGY = 100;

    // Milestone 3: Energy costs for farming actions
    // Centralized in one place for straightforward tuning and balancing.
    constexpr int ENERGY_COST_TILL = 5;      // Hoeing soil into tilled ground
    constexpr int ENERGY_COST_WATER = 3;     // Watering tilled soil
    constexpr int ENERGY_COST_PLANT = 2;     // Sowing a seed onto tilled soil
    constexpr int ENERGY_COST_HARVEST = 4;   // Harvesting mature produce
    constexpr int ENERGY_COST_FISH = 3;      // Fishing at the pond water tile

    // Inventory grid specifications
    constexpr int INVENTORY_COLS = 6;
    constexpr int INVENTORY_ROWS = 3;
    constexpr int INVENTORY_SLOT_COUNT = INVENTORY_COLS * INVENTORY_ROWS; // 18 slots
    constexpr int INITIAL_TOMATO_SEEDS = 6;
    constexpr int MAX_STACK_SIZE = 99;
}
