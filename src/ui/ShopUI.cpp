#include "ui/ShopUI.h"
#include "ui/BitmapFont.h"
#include <iostream>

#include "systems/AudioSystem.h"

void ShopUI::handleInput(const Input& input, Inventory& inventory, ShopSystem& shopSystem, HUD* hud, AudioSystem* audio) {
    if (!m_isOpen) return;

    // Close on Escape or E
    if (input.isKeyPressed(SDL_SCANCODE_ESCAPE) || input.isKeyPressed(SDL_SCANCODE_E)) {
        close();
        if (audio) audio->playSFX(SFX::UIConfirm);
        return;
    }

    // Switch between Buy and Sell tabs with Tab, Left, or Right
    if (input.isKeyPressed(SDL_SCANCODE_TAB) ||
        input.isKeyPressed(SDL_SCANCODE_LEFT) ||
        input.isKeyPressed(SDL_SCANCODE_RIGHT) ||
        input.isKeyPressed(SDL_SCANCODE_A) ||
        input.isKeyPressed(SDL_SCANCODE_D)) {
        m_mode = (m_mode == ShopMode::Buy) ? ShopMode::Sell : ShopMode::Buy;
        m_selectedIndex = 0;
        if (audio) audio->playSFX(SFX::UIConfirm);
        return;
    }

    const auto& catalog = (m_mode == ShopMode::Buy) ? shopSystem.getSellerCatalog() : shopSystem.getBuyerCatalog();
    int itemCount = static_cast<int>(catalog.size());
    if (itemCount == 0) return;

    // Navigate with Up/Down or W/S
    if (input.isKeyPressed(SDL_SCANCODE_UP) || input.isKeyPressed(SDL_SCANCODE_W)) {
        m_selectedIndex = (m_selectedIndex - 1 + itemCount) % itemCount;
        if (audio) audio->playSFX(SFX::UIConfirm);
    }
    if (input.isKeyPressed(SDL_SCANCODE_DOWN) || input.isKeyPressed(SDL_SCANCODE_S)) {
        m_selectedIndex = (m_selectedIndex + 1) % itemCount;
        if (audio) audio->playSFX(SFX::UIConfirm);
    }

    // Transact with Enter, Space, B, or S
    if (input.isKeyPressed(SDL_SCANCODE_RETURN) ||
        input.isKeyPressed(SDL_SCANCODE_SPACE) ||
        input.isKeyPressed(SDL_SCANCODE_B) ||
        input.isKeyPressed(SDL_SCANCODE_S)) {
        ItemType chosenItem = catalog[m_selectedIndex];
        if (m_mode == ShopMode::Buy) {
            shopSystem.buyItem(chosenItem, 1, inventory, hud, audio);
        } else {
            shopSystem.sellItem(chosenItem, 1, inventory, hud, audio);
        }
    }
}

void ShopUI::render(Renderer& renderer, const Inventory& inventory, const ShopSystem& shopSystem) const {
    if (!m_isOpen) return;

    // 1. Semi-transparent backdrop
    renderer.drawRect(0, 0, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, SDL_Color{10, 8, 8, 175});

    // 2. Window frame
    constexpr int WIN_W = 440;
    constexpr int WIN_H = 320;
    constexpr int WIN_X = (Config::SCREEN_WIDTH - WIN_W) / 2; // 100
    constexpr int WIN_Y = (Config::SCREEN_HEIGHT - WIN_H) / 2; // 80

    bool hasPanel = renderer.drawNineSlice("ui_panel.png", WIN_X, WIN_Y, WIN_W, WIN_H, 3, 3, 3, 3);
    if (!hasPanel) {
        renderer.drawRect(WIN_X, WIN_Y, WIN_W, WIN_H, SDL_Color{35, 27, 22, 252});
        renderer.drawOutline(WIN_X, WIN_Y, WIN_W, WIN_H, SDL_Color{195, 160, 105, 255});
        renderer.drawOutline(WIN_X + 2, WIN_Y + 2, WIN_W - 4, WIN_H - 4, SDL_Color{115, 85, 50, 255});
    }

    // 3. Header title
    std::string title = (m_mode == ShopMode::Buy) ? "SELLER: SEEDS & BASICS" : "BUYER: CROPS & DISHES";
    SDL_Color titleCol = hasPanel ? SDL_Color{60, 36, 22, 255} : SDL_Color{255, 230, 130, 255};
    BitmapFont::drawText(renderer, title, WIN_X + 16, WIN_Y + 12, titleCol, 2);

    // Current player wallet money counter in top-right
    int coinX = WIN_X + WIN_W - 130;
    int coinY = WIN_Y + 12;
    renderer.drawRect(coinX, coinY, 12, 12, SDL_Color{255, 215, 40, 255});
    renderer.drawOutline(coinX, coinY, 12, 12, SDL_Color{195, 145, 20, 255});
    SDL_Color coinTextCol = hasPanel ? SDL_Color{110, 75, 15, 255} : SDL_Color{255, 235, 140, 255};
    BitmapFont::drawNumber(renderer, inventory.getMoney(), coinX + 16, coinY + 1, coinTextCol, 2);
    BitmapFont::drawText(renderer, "G", coinX + 16 + BitmapFont::getTextWidth(std::to_string(inventory.getMoney()), 2) + 4,
                         coinY + 1, hasPanel ? SDL_Color{140, 95, 20, 255} : SDL_Color{255, 205, 50, 255}, 2);

    // 4. Tab selectors: [ BUY ]  [ SELL ]
    int tabY = WIN_Y + 34;
    // BUY tab
    const char* buySprite = (m_mode == ShopMode::Buy) ? "ui_button_hover.png" : "ui_button.png";
    if (!renderer.drawNineSlice(buySprite, WIN_X + 16, tabY, 80, 20, 8, 6, 8, 6)) {
        SDL_Color buyTabBg = (m_mode == ShopMode::Buy) ? SDL_Color{75, 55, 35, 255} : SDL_Color{40, 30, 25, 255};
        renderer.drawRect(WIN_X + 16, tabY, 80, 20, buyTabBg);
        renderer.drawOutline(WIN_X + 16, tabY, 80, 20, SDL_Color{160, 130, 85, 255});
    }
    SDL_Color buyTextCol = (m_mode == ShopMode::Buy) ? SDL_Color{60, 36, 22, 255} : SDL_Color{120, 95, 75, 255};
    BitmapFont::drawText(renderer, "BUY", WIN_X + 44, tabY + 4, buyTextCol, 2);

    // SELL tab
    const char* sellSprite = (m_mode == ShopMode::Sell) ? "ui_button_hover.png" : "ui_button.png";
    if (!renderer.drawNineSlice(sellSprite, WIN_X + 104, tabY, 80, 20, 8, 6, 8, 6)) {
        SDL_Color sellTabBg = (m_mode == ShopMode::Sell) ? SDL_Color{75, 55, 35, 255} : SDL_Color{40, 30, 25, 255};
        renderer.drawRect(WIN_X + 104, tabY, 80, 20, sellTabBg);
        renderer.drawOutline(WIN_X + 104, tabY, 80, 20, SDL_Color{160, 130, 85, 255});
    }
    SDL_Color sellTextCol = (m_mode == ShopMode::Sell) ? SDL_Color{60, 36, 22, 255} : SDL_Color{120, 95, 75, 255};
    BitmapFont::drawText(renderer, "SELL", WIN_X + 128, tabY + 4, sellTextCol, 2);

    renderer.drawLine(WIN_X + 10, tabY + 24, WIN_X + WIN_W - 10, tabY + 24,
                      hasPanel ? SDL_Color{180, 140, 90, 255} : SDL_Color{115, 85, 50, 255});

    // 5. Items catalog list
    const auto& catalog = (m_mode == ShopMode::Buy) ? shopSystem.getSellerCatalog() : shopSystem.getBuyerCatalog();
    int listY = tabY + 30;
    constexpr int ITEM_H = 40;
    constexpr int MAX_VISIBLE = 5;

    int totalItems = static_cast<int>(catalog.size());
    int scrollOffset = 0;
    if (totalItems > MAX_VISIBLE) {
        scrollOffset = std::max(0, std::min(m_selectedIndex - (MAX_VISIBLE / 2), totalItems - MAX_VISIBLE));
    }

    int visibleCount = std::min(MAX_VISIBLE, totalItems);
    for (int vi = 0; vi < visibleCount; ++vi) {
        int i = scrollOffset + vi;
        ItemType item = catalog[i];
        int itemY = listY + vi * (ITEM_H + 4);
        bool isSelected = (i == m_selectedIndex);

        int rowX = WIN_X + 14;
        int rowW = WIN_W - 28;
        const char* btnSprite = isSelected ? "ui_button_hover.png" : "ui_button.png";
        bool hasBtn = renderer.drawNineSlice(btnSprite, rowX, itemY, rowW, ITEM_H, 8, 6, 8, 6);
        if (!hasBtn) {
            if (isSelected) {
                renderer.drawRect(rowX, itemY, rowW, ITEM_H, SDL_Color{65, 48, 35, 255});
                renderer.drawOutline(rowX, itemY, rowW, ITEM_H, SDL_Color{240, 200, 100, 255});
                BitmapFont::drawText(renderer, ">", WIN_X + 18, itemY + 12, SDL_Color{255, 225, 100, 255}, 2);
            } else {
                renderer.drawRect(rowX, itemY, rowW, ITEM_H, SDL_Color{42, 32, 26, 200});
                renderer.drawOutline(rowX, itemY, rowW, ITEM_H, SDL_Color{75, 55, 35, 255});
            }
        } else if (isSelected) {
            // Selection indicator: glove cursor next to the currently selected row
            if (!renderer.drawSprite("ui_cursor.png", WIN_X - 10, itemY + (ITEM_H - 24) / 2, 24, 24)) {
                BitmapFont::drawText(renderer, ">", WIN_X + 18, itemY + 12, SDL_Color{60, 36, 22, 255}, 2);
            }
        }

        // Item icon
        drawItemPlaceholder(renderer, item, WIN_X + 30, itemY + 4, 32);

        // Item name
        SDL_Color nameCol;
        if (hasBtn) {
            nameCol = isSelected ? SDL_Color{50, 30, 15, 255} : SDL_Color{65, 42, 25, 255};
        } else {
            nameCol = isSelected ? SDL_Color{255, 240, 160, 255} : SDL_Color{220, 200, 180, 255};
        }
        BitmapFont::drawText(renderer, itemTypeToString(item), WIN_X + 70, itemY + 6, nameCol, 2);

        // Price & player count
        int inBag = inventory.getItemCount(item);
        if (m_mode == ShopMode::Buy) {
            int price = shopSystem.getBuyPrice(item);
            std::string priceStr = "Cost: " + std::to_string(price) + "G";
            SDL_Color priceCol = hasBtn ? SDL_Color{140, 85, 15, 255} : SDL_Color{255, 215, 60, 255};
            BitmapFont::drawText(renderer, priceStr, WIN_X + 70, itemY + 22, priceCol, 1);

            std::string bagStr = (item == ItemType::Chicken)
                ? "(Pen: " + std::to_string(shopSystem.getChickenCount()) + "/6)"
                : "(In Bag: " + std::to_string(inBag) + ")";
            SDL_Color bagCol = hasBtn ? SDL_Color{100, 80, 65, 255} : SDL_Color{160, 150, 140, 255};
            BitmapFont::drawText(renderer, bagStr, WIN_X + 160, itemY + 22, bagCol, 1);

            // Buy afford badge
            if (item == ItemType::Chicken && shopSystem.getChickenCount() >= 6) {
                SDL_Color capCol = hasBtn ? SDL_Color{180, 45, 35, 255} : SDL_Color{230, 90, 80, 255};
                BitmapFont::drawText(renderer, "[CAP 6/6]", WIN_X + WIN_W - 95, itemY + 12, capCol, 1);
            } else if (inventory.getMoney() >= price) {
                SDL_Color buyBadgeCol = hasBtn ? SDL_Color{35, 125, 40, 255} : SDL_Color{120, 235, 120, 255};
                BitmapFont::drawText(renderer, "[BUY]", WIN_X + WIN_W - 75, itemY + 12, buyBadgeCol, 2);
            } else {
                SDL_Color noCoinsCol = hasBtn ? SDL_Color{180, 45, 35, 255} : SDL_Color{230, 90, 80, 255};
                BitmapFont::drawText(renderer, "[NO COINS]", WIN_X + WIN_W - 105, itemY + 12, noCoinsCol, 1);
            }
        } else {
            int price = shopSystem.getSellPrice(item);
            std::string priceStr = "Payout: +" + std::to_string(price) + "G";
            SDL_Color payoutCol = hasBtn ? SDL_Color{35, 125, 40, 255} : SDL_Color{120, 235, 120, 255};
            BitmapFont::drawText(renderer, priceStr, WIN_X + 70, itemY + 22, payoutCol, 1);

            std::string bagStr = "(In Bag: " + std::to_string(inBag) + ")";
            SDL_Color bagCol;
            if (hasBtn) {
                bagCol = (inBag > 0) ? SDL_Color{100, 80, 65, 255} : SDL_Color{140, 70, 60, 255};
            } else {
                bagCol = (inBag > 0) ? SDL_Color{220, 210, 180, 255} : SDL_Color{150, 100, 100, 255};
            }
            BitmapFont::drawText(renderer, bagStr, WIN_X + 180, itemY + 22, bagCol, 1);

            if (inBag > 0) {
                SDL_Color sellBadgeCol = hasBtn ? SDL_Color{35, 125, 40, 255} : SDL_Color{120, 235, 120, 255};
                BitmapFont::drawText(renderer, "[SELL]", WIN_X + WIN_W - 85, itemY + 12, sellBadgeCol, 2);
            } else {
                SDL_Color noneCol = hasBtn ? SDL_Color{130, 105, 90, 255} : SDL_Color{140, 120, 110, 255};
                BitmapFont::drawText(renderer, "[NONE]", WIN_X + WIN_W - 75, itemY + 12, noneCol, 1);
            }
        }
    }

    // 6. Footer controls
    renderer.drawLine(WIN_X + 10, WIN_Y + WIN_H - 26, WIN_X + WIN_W - 10, WIN_Y + WIN_H - 26,
                      hasPanel ? SDL_Color{180, 140, 90, 255} : SDL_Color{115, 85, 50, 255});
    SDL_Color footerCol = hasPanel ? SDL_Color{110, 80, 50, 255} : SDL_Color{185, 165, 140, 255};
    BitmapFont::drawText(renderer, "[W/S] Select  [ENTER/SPACE] Transact  [TAB] Switch Tab  [ESC] Close",
                         WIN_X + 14, WIN_Y + WIN_H - 18, footerCol, 1);
}
