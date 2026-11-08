#pragma once

#include "core/Renderer.h"
#include "core/Input.h"
#include "entities/Inventory.h"
#include "systems/CookingSystem.h"
#include "ui/HUD.h"

class AudioSystem;

// Interactive kitchen stove cooking modal UI.
// Lists recipes loaded from recipes.txt, displays ingredient requirements with live stock,
// and handles keyboard interaction to prepare dishes.
class CookingUI {
public:
    CookingUI() = default;

    bool isOpen() const { return m_isOpen; }
    void open() { m_isOpen = true; }
    void close() { m_isOpen = false; }
    void toggle() { m_isOpen = !m_isOpen; }

    int getSelectedIndex() const { return m_selectedIndex; }
    void setSelectedIndex(int idx) { m_selectedIndex = idx; }

    // Handles keyboard navigation (Arrow keys / WASD / Enter / Space / Escape)
    void handleInput(const Input& input, Inventory& inventory, CookingSystem& cookingSystem, HUD* hud = nullptr, AudioSystem* audio = nullptr);

    // Renders the cooking modal overlay, recipes list, ingredient checklist, and dish icons
    void render(Renderer& renderer, const Inventory& inventory, const CookingSystem& cookingSystem) const;

private:
    bool m_isOpen = false;
    int m_selectedIndex = 0;
};
