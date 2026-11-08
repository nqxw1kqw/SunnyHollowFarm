#include "entities/Inventory.h"
#include <iostream>

Inventory::Inventory() {
    m_slots.resize(Config::INVENTORY_SLOT_COUNT);

    // Initial loadout: 6 tomato seeds in the first slot
    m_slots[0].type = ItemType::TomatoSeeds;
    m_slots[0].count = Config::INITIAL_TOMATO_SEEDS;
}

bool Inventory::hasSpaceFor(ItemType type, int count) const {
    if (type == ItemType::Empty || count <= 0) return false;
    int remaining = count;
    for (const auto& slot : m_slots) {
        if (slot.type == type && slot.count < Config::MAX_STACK_SIZE) {
            remaining -= (Config::MAX_STACK_SIZE - slot.count);
            if (remaining <= 0) return true;
        } else if (slot.isEmpty()) {
            remaining -= Config::MAX_STACK_SIZE;
            if (remaining <= 0) return true;
        }
    }
    return remaining <= 0;
}

bool Inventory::addItem(ItemType type, int count) {
    if (type == ItemType::Empty || count <= 0) return false;

    int remaining = count;

    // 1. Try to stack into existing slots with the same item type
    for (auto& slot : m_slots) {
        if (slot.type == type && slot.count < Config::MAX_STACK_SIZE) {
            int space = Config::MAX_STACK_SIZE - slot.count;
            int add = std::min(space, remaining);
            slot.count += add;
            remaining -= add;
            if (remaining <= 0) return true;
        }
    }

    // 2. Put remaining into first available empty slots
    for (auto& slot : m_slots) {
        if (slot.isEmpty()) {
            slot.type = type;
            int add = std::min(Config::MAX_STACK_SIZE, remaining);
            slot.count = add;
            remaining -= add;
            if (remaining <= 0) return true;
        }
    }

    // If remaining > 0, inventory was full
    return remaining == 0;
}

bool Inventory::removeItem(ItemType type, int count) {
    if (type == ItemType::Empty || count <= 0) return false;
    if (getItemCount(type) < count) return false;

    int remaining = count;
    for (auto& slot : m_slots) {
        if (slot.type == type) {
            if (slot.count <= remaining) {
                remaining -= slot.count;
                slot.clear();
            } else {
                slot.count -= remaining;
                remaining = 0;
            }
            if (remaining <= 0) break;
        }
    }

    return true;
}

int Inventory::getItemCount(ItemType type) const {
    if (type == ItemType::Empty) return 0;
    int total = 0;
    for (const auto& slot : m_slots) {
        if (slot.type == type) {
            total += slot.count;
        }
    }
    return total;
}
