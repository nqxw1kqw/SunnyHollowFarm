#pragma once

#include "core/Renderer.h"
#include "core/Input.h"
#include "entities/Inventory.h"

class AudioSystem;

// Interactive inventory modal grid UI displaying all 18 storage slots,
// item counts, visual placeholder icons, detailed item descriptions, and coin wallet.
class InventoryUI {
public:
    InventoryUI() = default;

    // Window visibility toggle
    bool isOpen() const { return m_isOpen; }
    void open() { m_isOpen = true; }
    void close() { m_isOpen = false; }
    void toggle() { m_isOpen = !m_isOpen; }

    // Slot selection
    int getSelectedSlot() const { return m_selectedSlot; }
    void setSelectedSlot(int index) {
        m_selectedSlot = std::clamp(index, 0, Config::INVENTORY_SLOT_COUNT - 1);
    }

    // Handles keyboard navigation (Arrow keys / WASD / Tab / I / Escape)
    void handleInput(const Input& input, Inventory& inventory, AudioSystem* audio = nullptr);

    // Renders the modal overlay, slot grid, icons, and item details panel
    void render(Renderer& renderer, const Inventory& inventory) const;

private:
    bool m_isOpen = false;
    int m_selectedSlot = 0;
};
