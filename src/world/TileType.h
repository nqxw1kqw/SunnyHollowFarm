#pragma once

// Represents the terrain or ground state of a single 32x32 tile on the map.
enum class TileType {
    Grass,          // Natural grassy meadow
    Dirt,           // Untilled farm soil
    Path,           // Beaten dirt footpath
    TilledSoil,     // Soil prepared with a hoe, ready for planting
    WateredSoil,    // Tilled soil that has been watered, accelerating crop growth
    Water,          // Pond water (impassable)
    Fence,          // Boundary wooden fence (impassable)
    Farmhouse,      // Farmhouse exterior structure (impassable)
    FarmhouseDoor,  // Farmhouse entrance door (walkable transition to interior)
    Stove,          // Kitchen stove cooking interaction point (impassable)
    ShopStall,      // Market canopy and wooden merchant counter (impassable)
    WoodFloor,      // Hardwood plank floor inside farmhouse (walkable)
    Wall,           // Farmhouse interior wall (impassable)
    Bed,            // Indoor comfortable bed (impassable, rest to advance day)
    Chest,          // Storage chest (impassable, check inventory/produce storage)
    Bookshelf,      // Bookshelf and desk (impassable, read farming almanac)
    InteriorExit,   // Interior door threshold/rug (walkable transition to outdoor farm)
    Void            // Dark void surrounding interior room (impassable)
};
