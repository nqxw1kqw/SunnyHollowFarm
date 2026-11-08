#pragma once

#include <string>
#include "core/Renderer.h"

// Enumeration of all items in Sunny Hollow Farm.
// Designed up-front so Milestone 4 (Cooking: Salad, Bread) and Milestone 5 (Shops: Carrot, Wheat)
// slot directly into the inventory system without redesign.
enum class ItemType {
    Empty = 0,
    TomatoSeeds,
    CarrotSeeds,
    WheatSeeds,
    Tomato,
    Carrot,
    Wheat,
    Salad,
    Bread,
    Fish,
    GrilledFish,
    Egg,
    Omelette,
    Chicken
};

// Metadata for items including display names, descriptions, and economy tags.
struct ItemDefinition {
    ItemType type = ItemType::Empty;
    std::string name = "Empty";
    std::string description = "";
    int buyPrice = 0;       // Milestone 5 shop purchase cost
    int sellPrice = 0;      // Milestone 5 shop sale yield
    bool isSeed = false;
    bool isFood = false;
    int energyRestore = 0;  // Stamina restored when consumed
};

// Retrieves constant metadata for a given item type.
const ItemDefinition& getItemDefinition(ItemType type);

// Name conversion utilities for data parsing (recipes.txt, prices.txt)
ItemType itemTypeFromString(const std::string& name);
std::string itemTypeToString(ItemType type);

// Draws a pixel art placeholder icon for the given item type.
void drawItemPlaceholder(Renderer& renderer, ItemType type, int x, int y, int size = 32);
