#include "ui/CookingUI.h"
#include "ui/BitmapFont.h"
#include <iostream>

void CookingUI::handleInput(const Input& input, Inventory& inventory, CookingSystem& cookingSystem, HUD* hud) {
    if (!m_isOpen) return;

    // Close on Escape, Tab, or E
    if (input.isKeyPressed(SDL_SCANCODE_ESCAPE) ||
        input.isKeyPressed(SDL_SCANCODE_TAB) ||
        input.isKeyPressed(SDL_SCANCODE_E)) {
        close();
        return;
    }

    int recipeCount = cookingSystem.getRecipeCount();
    if (recipeCount == 0) return;

    // Navigate recipe list with Up/Down or W/S
    if (input.isKeyPressed(SDL_SCANCODE_UP) || input.isKeyPressed(SDL_SCANCODE_W)) {
        m_selectedIndex = (m_selectedIndex - 1 + recipeCount) % recipeCount;
    }
    if (input.isKeyPressed(SDL_SCANCODE_DOWN) || input.isKeyPressed(SDL_SCANCODE_S)) {
        m_selectedIndex = (m_selectedIndex + 1) % recipeCount;
    }

    // Cook with Return, Space, or C
    if (input.isKeyPressed(SDL_SCANCODE_RETURN) ||
        input.isKeyPressed(SDL_SCANCODE_SPACE) ||
        input.isKeyPressed(SDL_SCANCODE_C)) {
        bool ok = cookingSystem.cook(m_selectedIndex, inventory, hud);
        if (!ok) {
            std::cout << "[CookingUI] Cooking failed due to missing ingredients." << std::endl;
        }
    }
}

void CookingUI::render(Renderer& renderer, const Inventory& inventory, const CookingSystem& cookingSystem) const {
    if (!m_isOpen) return;

    // 1. Semi-transparent backdrop
    renderer.drawRect(0, 0, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, SDL_Color{10, 8, 8, 175});

    // 2. Cooking window frame
    constexpr int WIN_W = 420;
    constexpr int WIN_H = 300;
    constexpr int WIN_X = (Config::SCREEN_WIDTH - WIN_W) / 2; // 110
    constexpr int WIN_Y = (Config::SCREEN_HEIGHT - WIN_H) / 2; // 90

    bool hasPanel = renderer.drawNineSlice("ui_panel.png", WIN_X, WIN_Y, WIN_W, WIN_H, 3, 3, 3, 3);
    if (!hasPanel) {
        renderer.drawRect(WIN_X, WIN_Y, WIN_W, WIN_H, SDL_Color{35, 27, 22, 252});
        renderer.drawOutline(WIN_X, WIN_Y, WIN_W, WIN_H, SDL_Color{195, 160, 105, 255});
        renderer.drawOutline(WIN_X + 2, WIN_Y + 2, WIN_W - 4, WIN_H - 4, SDL_Color{115, 85, 50, 255});
    }

    // 3. Header icon & title
    bool hasCookingIcon = renderer.drawSprite("ui_cooking_icon.png", WIN_X + 16, WIN_Y + 8, 20, 20);
    int titleX = hasCookingIcon ? (WIN_X + 42) : (WIN_X + 16);
    SDL_Color titleCol = hasPanel ? SDL_Color{60, 36, 22, 255} : SDL_Color{255, 230, 130, 255};
    BitmapFont::drawText(renderer, "KITCHEN STOVE - RECIPES", titleX, WIN_Y + 12, titleCol, 2);
    renderer.drawLine(WIN_X + 10, WIN_Y + 32, WIN_X + WIN_W - 10, WIN_Y + 32,
                      hasPanel ? SDL_Color{180, 140, 90, 255} : SDL_Color{115, 85, 50, 255});

    // 4. Recipes List
    const auto& recipes = cookingSystem.getRecipes();
    int startY = WIN_Y + 42;
    constexpr int ITEM_HEIGHT = 70;

    for (size_t i = 0; i < recipes.size(); ++i) {
        const auto& r = recipes[i];
        int itemY = startY + static_cast<int>(i) * ITEM_HEIGHT;
        bool isSelected = (static_cast<int>(i) == m_selectedIndex);

        int rowX = WIN_X + 12;
        int rowW = WIN_W - 24;
        int rowH = ITEM_HEIGHT - 6; // 64
        const char* btnSprite = isSelected ? "ui_button_hover.png" : "ui_button.png";
        bool hasBtn = renderer.drawNineSlice(btnSprite, rowX, itemY, rowW, rowH, 8, 6, 8, 6);
        if (!hasBtn) {
            if (isSelected) {
                renderer.drawRect(rowX, itemY, rowW, rowH, SDL_Color{65, 48, 35, 255});
                renderer.drawOutline(rowX, itemY, rowW, rowH, SDL_Color{240, 200, 100, 255});
                BitmapFont::drawText(renderer, ">", WIN_X + 16, itemY + 14, SDL_Color{255, 225, 100, 255}, 2);
            } else {
                renderer.drawRect(rowX, itemY, rowW, rowH, SDL_Color{42, 32, 26, 200});
                renderer.drawOutline(rowX, itemY, rowW, rowH, SDL_Color{75, 55, 35, 255});
            }
        } else if (isSelected) {
            // Selection indicator: glove cursor next to the currently selected row
            if (!renderer.drawSprite("ui_cursor.png", WIN_X - 10, itemY + (rowH - 24) / 2, 24, 24)) {
                BitmapFont::drawText(renderer, ">", WIN_X + 16, itemY + 14, SDL_Color{60, 36, 22, 255}, 2);
            }
        }

        // Draw dish icon
        drawItemPlaceholder(renderer, r.result, WIN_X + 28, itemY + 8, 32);

        // Draw dish name
        SDL_Color nameColor;
        if (hasBtn) {
            nameColor = isSelected ? SDL_Color{50, 30, 15, 255} : SDL_Color{65, 42, 25, 255};
        } else {
            nameColor = isSelected ? SDL_Color{255, 240, 160, 255} : SDL_Color{220, 200, 180, 255};
        }
        BitmapFont::drawText(renderer, r.dishName, WIN_X + 68, itemY + 8, nameColor, 2);

        // Check if player has all ingredients
        std::string missingStr;
        bool canMake = cookingSystem.canCook(r, inventory, &missingStr);

        // Draw ingredients info
        int ingOffsetY = itemY + 28;
        for (size_t j = 0; j < r.ingredients.size(); ++j) {
            const auto& ing = r.ingredients[j];
            int have = inventory.getItemCount(ing.item);
            std::string ingText = itemTypeToString(ing.item) + ": " + std::to_string(ing.count) +
                                  " (Have: " + std::to_string(have) + ")";

            SDL_Color ingColor;
            if (hasBtn) {
                ingColor = (have >= ing.count) ? SDL_Color{35, 125, 40, 255} : SDL_Color{180, 45, 35, 255};
            } else {
                ingColor = (have >= ing.count) ? SDL_Color{120, 220, 120, 255} : SDL_Color{240, 110, 100, 255};
            }
            BitmapFont::drawText(renderer, ingText, WIN_X + 68, ingOffsetY + static_cast<int>(j) * 14, ingColor, 1);
        }

        // Status badge: READY or MISSING
        if (canMake) {
            SDL_Color readyCol = hasBtn ? SDL_Color{35, 125, 40, 255} : SDL_Color{120, 235, 120, 255};
            BitmapFont::drawText(renderer, "[READY]", WIN_X + WIN_W - 85, itemY + 14, readyCol, 2);
        } else {
            SDL_Color missingCol = hasBtn ? SDL_Color{180, 45, 35, 255} : SDL_Color{230, 90, 80, 255};
            BitmapFont::drawText(renderer, "[MISSING]", WIN_X + WIN_W - 95, itemY + 14, missingCol, 2);
        }
    }

    // 5. Footer instructions
    renderer.drawLine(WIN_X + 10, WIN_Y + WIN_H - 30, WIN_X + WIN_W - 10, WIN_Y + WIN_H - 30,
                      hasPanel ? SDL_Color{180, 140, 90, 255} : SDL_Color{115, 85, 50, 255});
    SDL_Color footerCol = hasPanel ? SDL_Color{110, 80, 50, 255} : SDL_Color{185, 165, 140, 255};
    BitmapFont::drawText(renderer, "[W/S] Select  [ENTER/SPACE] Cook  [ESC] Close",
                         WIN_X + 22, WIN_Y + WIN_H - 20, footerCol, 1);
}
