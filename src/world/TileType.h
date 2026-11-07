#pragma once

// Represents the terrain or ground state of a single 32x32 tile on the map.
enum class TileType {
    Grass,          // Natural grassy meadow
    Dirt,           // Untilled farm soil
    TilledSoil,     // Soil prepared with a hoe, ready for planting
    WateredSoil,    // Tilled soil that has been watered, accelerating crop growth
    Water,          // Pond water (impassable)
    Fence           // Boundary wooden fence (impassable)
};
