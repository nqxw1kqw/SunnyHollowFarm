#pragma once

#include "core/Renderer.h"
#include "core/Input.h"
#include "entities/Inventory.h"
#include "systems/ShopSystem.h"
#include "ui/HUD.h"

// Tab mode for the shop modal.
enum class ShopMode {
    Buy,  // Seller NPC mode: purchasing seeds and staples
    Sell  // Buyer NPC mode: selling produce and prepared dishes
};

// Interactive shop interface for trading with Seller and Buyer NPCs.
class ShopUI {
public:
    ShopUI() = default;

    bool isOpen() const { return m_isOpen; }
    void open(ShopMode mode = ShopMode::Buy) {
        m_mode = mode;
        m_isOpen = true;
        m_selectedIndex = 0;
    }
    void close() { m_isOpen = false; }
    void toggle() { m_isOpen = !m_isOpen; }

    ShopMode getMode() const { return m_mode; }
    void setMode(ShopMode mode) {
        m_mode = mode;
        m_selectedIndex = 0;
    }

    int getSelectedIndex() const { return m_selectedIndex; }
    void setSelectedIndex(int idx) { m_selectedIndex = idx; }

    // Handles keyboard inputs (Arrow keys / WASD / Enter / Space / Tab / Escape)
    void handleInput(const Input& input, Inventory& inventory, ShopSystem& shopSystem, HUD* hud = nullptr);

    // Renders the shop modal window, item list, wallet balance, and buy/sell tabs
    void render(Renderer& renderer, const Inventory& inventory, const ShopSystem& shopSystem) const;

private:
    bool m_isOpen = false;
    ShopMode m_mode = ShopMode::Buy;
    int m_selectedIndex = 0;
};
