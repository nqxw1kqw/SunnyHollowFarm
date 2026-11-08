#pragma once

#include "entities/Inventory.h"
#include "ui/HUD.h"
#include <vector>
#include <unordered_map>
#include <string>

// Pricing metadata loaded from assets/data/prices.txt.
struct ItemPrice {
    int buyPrice = 0;   // Cost to buy from seller NPC
    int sellPrice = 0;  // Payout when sold to buyer NPC
};

// Shop system handling economy prices, purchases, and sales.
class ShopSystem {
public:
    ShopSystem() = default;

    // Parses prices from prices.txt (with fallback search paths).
    bool loadPrices(const std::string& filePath = "assets/data/prices.txt");

    // Price query accessors
    int getBuyPrice(ItemType item) const;
    int getSellPrice(ItemType item) const;

    // Purchases an item from the seller NPC, deducting coins and adding items.
    // Provides clear on-screen HUD feedback and console logging.
    bool buyItem(ItemType item, int count, Inventory& inventory, HUD* hud = nullptr);

    // Sells an item to the buyer NPC, removing items and granting coins.
    // Provides clear on-screen HUD feedback and console logging.
    bool sellItem(ItemType item, int count, Inventory& inventory, HUD* hud = nullptr);

    // Items offered by Seller NPC (seeds and pantry basics)
    const std::vector<ItemType>& getSellerCatalog() const { return m_sellerCatalog; }

    // Items accepted by Buyer NPC (farm produce and cooked dishes)
    const std::vector<ItemType>& getBuyerCatalog() const { return m_buyerCatalog; }

private:
    std::unordered_map<ItemType, ItemPrice> m_prices;
    std::vector<ItemType> m_sellerCatalog;
    std::vector<ItemType> m_buyerCatalog;
};
