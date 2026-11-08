#include "systems/ShopSystem.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

namespace {
    std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, (last - first + 1));
    }
}

bool ShopSystem::loadPrices(const std::string& filePath) {
    m_prices.clear();
    m_sellerCatalog = {
        ItemType::TomatoSeeds,
        ItemType::WheatSeeds,
        ItemType::CarrotSeeds,
        ItemType::Wheat
    };
    m_buyerCatalog = {
        ItemType::Tomato,
        ItemType::Wheat,
        ItemType::Carrot,
        ItemType::Salad,
        ItemType::Bread,
        ItemType::Fish,
        ItemType::GrilledFish
    };

    // Candidate paths to ensure loading works when launched from repo root or build dir
    std::vector<std::string> candidatePaths = {
        filePath,
        "../" + filePath,
        "assets/data/prices.txt",
        "../assets/data/prices.txt",
        "/home/aika/projects/SunnyHollowFarm/assets/data/prices.txt"
    };

    std::ifstream file;
    std::string usedPath;
    for (const auto& path : candidatePaths) {
        file.open(path);
        if (file.is_open()) {
            usedPath = path;
            break;
        }
    }

    if (!file.is_open()) {
        std::cerr << "[ShopSystem] Warning: Unable to open prices file (" << filePath
                  << "). Setting up built-in default prices." << std::endl;
        // Built-in defaults
        m_prices[ItemType::TomatoSeeds] = {15, 8};
        m_prices[ItemType::WheatSeeds]  = {10, 5};
        m_prices[ItemType::CarrotSeeds] = {20, 10};
        m_prices[ItemType::Wheat]       = {18, 14};
        m_prices[ItemType::Tomato]      = {25, 20};
        m_prices[ItemType::Carrot]      = {30, 24};
        m_prices[ItemType::Salad]       = {50, 40};
        m_prices[ItemType::Bread]       = {60, 50};
        m_prices[ItemType::Fish]        = {0, 30};
        m_prices[ItemType::GrilledFish] = {0, 55};
        return false;
    }

    std::string line;
    int lineNum = 0;
    while (std::getline(file, line)) {
        lineNum++;
        std::string trimmed = trim(line);
        if (trimmed.empty() || trimmed[0] == '#') {
            continue;
        }

        // Format: <ItemName> = Buy:<BuyPrice>, Sell:<SellPrice>
        size_t eqPos = trimmed.find('=');
        if (eqPos == std::string::npos) {
            std::cerr << "[ShopSystem] Warning: Malformed price entry at line " << lineNum << ": " << line << std::endl;
            continue;
        }

        std::string itemName = trim(trimmed.substr(0, eqPos));
        std::string priceList = trim(trimmed.substr(eqPos + 1));

        ItemType itemType = itemTypeFromString(itemName);
        if (itemType == ItemType::Empty) {
            std::cerr << "[ShopSystem] Warning: Unknown item '" << itemName << "' at line " << lineNum << std::endl;
            continue;
        }

        ItemPrice priceEntry;
        std::stringstream ss(priceList);
        std::string token;
        while (std::getline(ss, token, ',')) {
            std::string t = trim(token);
            size_t colon = t.find(':');
            if (colon != std::string::npos) {
                std::string key = trim(t.substr(0, colon));
                int val = 0;
                try {
                    val = std::stoi(trim(t.substr(colon + 1)));
                } catch (...) {
                    val = 0;
                }
                if (key == "Buy" || key == "buy") priceEntry.buyPrice = val;
                else if (key == "Sell" || key == "sell") priceEntry.sellPrice = val;
            }
        }

        m_prices[itemType] = priceEntry;
        std::cout << "[ShopSystem] Price loaded for " << itemTypeToString(itemType)
                  << ": Buy=" << priceEntry.buyPrice << "G, Sell=" << priceEntry.sellPrice << "G" << std::endl;
    }

    std::cout << "[ShopSystem] Successfully loaded prices from " << usedPath << std::endl;
    return true;
}

int ShopSystem::getBuyPrice(ItemType item) const {
    auto it = m_prices.find(item);
    if (it != m_prices.end()) {
        return it->second.buyPrice;
    }
    return getItemDefinition(item).buyPrice;
}

int ShopSystem::getSellPrice(ItemType item) const {
    auto it = m_prices.find(item);
    if (it != m_prices.end()) {
        return it->second.sellPrice;
    }
    return getItemDefinition(item).sellPrice;
}

#include "systems/AudioSystem.h"

bool ShopSystem::buyItem(ItemType item, int count, Inventory& inventory, HUD* hud, AudioSystem* audio) {
    if (item == ItemType::Empty || count <= 0) return false;

    int unitPrice = getBuyPrice(item);
    int totalCost = unitPrice * count;

    if (inventory.getMoney() < totalCost) {
        std::cout << "[ShopSystem] Buy refused: Not enough money! (Need " << totalCost
                  << "G, have " << inventory.getMoney() << "G)" << std::endl;
        if (hud) {
            hud->showFeedback("NOT ENOUGH MONEY!");
        }
        return false;
    }

    if (!inventory.hasSpaceFor(item, count)) {
        std::cout << "[ShopSystem] Buy refused: Inventory full!" << std::endl;
        if (hud) {
            hud->showFeedback("INVENTORY FULL!");
        }
        return false;
    }

    // Deduct coins and deposit items
    inventory.spendMoney(totalCost);
    inventory.addItem(item, count);

    std::cout << "[ShopSystem] Purchased " << count << "x " << itemTypeToString(item)
              << " for " << totalCost << "G [Remaining Wallet: " << inventory.getMoney() << "G]" << std::endl;
    if (audio) {
        audio->playSFX(SFX::Buy);
    }
    if (hud) {
        std::string msg = "BOUGHT: " + itemTypeToString(item) + " (-" + std::to_string(totalCost) + "G)";
        std::transform(msg.begin(), msg.end(), msg.begin(), ::toupper);
        hud->showFeedback(msg);
    }

    return true;
}

bool ShopSystem::sellItem(ItemType item, int count, Inventory& inventory, HUD* hud, AudioSystem* audio) {
    if (item == ItemType::Empty || count <= 0) return false;

    int haveCount = inventory.getItemCount(item);
    if (haveCount < count) {
        std::cout << "[ShopSystem] Sell refused: Not enough items! (Need " << count
                  << ", have " << haveCount << ")" << std::endl;
        if (hud) {
            hud->showFeedback("NO ITEMS TO SELL!");
        }
        return false;
    }

    int unitPayout = getSellPrice(item);
    int totalPayout = unitPayout * count;

    // Remove items and add coins
    inventory.removeItem(item, count);
    inventory.addMoney(totalPayout);

    std::cout << "[ShopSystem] Sold " << count << "x " << itemTypeToString(item)
              << " for +" << totalPayout << "G [New Wallet: " << inventory.getMoney() << "G]" << std::endl;
    if (audio) {
        audio->playSFX(SFX::Sell);
    }
    if (hud) {
        std::string msg = "SOLD: " + itemTypeToString(item) + " (+" + std::to_string(totalPayout) + "G)";
        std::transform(msg.begin(), msg.end(), msg.begin(), ::toupper);
        hud->showFeedback(msg);
    }

    return true;
}
