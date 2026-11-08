#pragma once

#include <vector>
#include <algorithm>
#include "core/GameConfig.h"
#include "entities/Item.h"

// Types of tools the farmer can hold on the primary hotbar.
enum class ToolType {
    Hoe = 0,
    WateringCan = 1,
    TomatoSeeds = 2,
    WheatSeeds = 3
};

// Represents a single item container slot in the inventory grid.
struct InventorySlot {
    ItemType type = ItemType::Empty;
    int count = 0;

    bool isEmpty() const { return type == ItemType::Empty || count <= 0; }
    void clear() { type = ItemType::Empty; count = 0; }
};

// Player inventory managing grid slots, seed stock, harvested produce, active tool, and coin wallet.
class Inventory {
public:
    Inventory();

    // Tool hotbar selection
    ToolType activeTool = ToolType::Hoe;

    // Currency / Wallet operations (Milestone 3 & 5)
    int getMoney() const { return m_money; }
    void setMoney(int amount) { m_money = std::max(0, amount); }
    void addMoney(int amount) { m_money += amount; }
    bool spendMoney(int amount) {
        if (m_money >= amount) {
            m_money -= amount;
            return true;
        }
        return false;
    }

    // Item management operations
    bool addItem(ItemType type, int count = 1);
    bool removeItem(ItemType type, int count = 1);
    int getItemCount(ItemType type) const;
    bool hasItem(ItemType type, int count = 1) const { return getItemCount(type) >= count; }

    // Slot accessors
    const std::vector<InventorySlot>& getSlots() const { return m_slots; }
    std::vector<InventorySlot>& getSlots() { return m_slots; }
    const InventorySlot& getSlot(int index) const { return m_slots.at(index); }
    InventorySlot& getSlot(int index) { return m_slots.at(index); }
    int getSlotCount() const { return static_cast<int>(m_slots.size()); }

    // Convenience accessors for existing Milestone 2 callers
    int getTomatoSeeds() const { return getItemCount(ItemType::TomatoSeeds); }
    int getHarvestedTomatoes() const { return getItemCount(ItemType::Tomato); }

private:
    int m_money = Config::INITIAL_MONEY;
    std::vector<InventorySlot> m_slots;
};
