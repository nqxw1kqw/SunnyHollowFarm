#include "ui/ShopUI.h"
#include "ui/BitmapFont.h"
#include <iostream>

void ShopUI::handleInput(const Input& input, Inventory& inventory, ShopSystem& shopSystem, HUD* hud) {
    if (!m_isOpen) return;

    // Close on Escape or E
    if (input.isKeyPressed(SDL_SCANCODE_ESCAPE) || input.isKeyPressed(SDL_SCANCODE_E)) {
        close();
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
        return;
    }

    const auto& catalog = (m_mode == ShopMode::Buy) ? shopSystem.getSellerCatalog() : shopSystem.getBuyerCatalog();
    int itemCount = static_cast<int>(catalog.size());
    if (itemCount == 0) return;

    // Navigate with Up/Down or W/S
    if (input.isKeyPressed(SDL_SCANCODE_UP) || input.isKeyPressed(SDL_SCANCODE_W)) {
        m_selectedIndex = (m_selectedIndex - 1 + itemCount) % itemCount;
    }
    if (input.isKeyPressed(SDL_SCANCODE_DOWN) || input.isKeyPressed(SDL_SCANCODE_S)) {
        m_selectedIndex = (m_selectedIndex + 1) % itemCount;
    }

    // Transact with Enter, Space, B, or S
    if (input.isKeyPressed(SDL_SCANCODE_RETURN) ||
        input.isKeyPressed(SDL_SCANCODE_SPACE) ||
        input.isKeyPressed(SDL_SCANCODE_B) ||
        input.isKeyPressed(SDL_SCANCODE_S)) {
        ItemType chosenItem = catalog[m_selectedIndex];
        if (m_mode == ShopMode::Buy) {
            shopSystem.buyItem(chosenItem, 1, inventory, hud);
        } else {
            shopSystem.sellItem(chosenItem, 1, inventory, hud);
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

    renderer.drawRect(WIN_X, WIN_Y, WIN_W, WIN_H, SDL_Color{35, 27, 22, 252});
    renderer.drawOutline(WIN_X, WIN_Y, WIN_W, WIN_H, SDL_Color{195, 160, 105, 255});
    renderer.drawOutline(WIN_X + 2, WIN_Y + 2, WIN_W - 4, WIN_H - 4, SDL_Color{115, 85, 50, 255});

    // 3. Header title
    std::string title = (m_mode == ShopMode::Buy) ? "SELLER: SEEDS & BASICS" : "BUYER: CROPS & DISHES";
    BitmapFont::drawText(renderer, title, WIN_X + 16, WIN_Y + 12, SDL_Color{255, 230, 130, 255}, 2);

    // Current player wallet money counter in top-right
    int coinX = WIN_X + WIN_W - 130;
    int coinY = WIN_Y + 12;
    renderer.drawRect(coinX, coinY, 12, 12, SDL_Color{255, 215, 40, 255});
    renderer.drawOutline(coinX, coinY, 12, 12, SDL_Color{195, 145, 20, 255});
    BitmapFont::drawNumber(renderer, inventory.getMoney(), coinX + 16, coinY + 1, SDL_Color{255, 235, 140, 255}, 2);
    BitmapFont::drawText(renderer, "G", coinX + 16 + BitmapFont::getTextWidth(std::to_string(inventory.getMoney()), 2) + 4,
                         coinY + 1, SDL_Color{255, 205, 50, 255}, 2);

    // 4. Tab selectors: [ BUY ]  [ SELL ]
    int tabY = WIN_Y + 34;
    // BUY tab
    SDL_Color buyTabBg = (m_mode == ShopMode::Buy) ? SDL_Color{75, 55, 35, 255} : SDL_Color{40, 30, 25, 255};
    renderer.drawRect(WIN_X + 16, tabY, 80, 20, buyTabBg);
    renderer.drawOutline(WIN_X + 16, tabY, 80, 20, SDL_Color{160, 130, 85, 255});
    BitmapFont::drawText(renderer, "BUY", WIN_X + 44, tabY + 4, (m_mode == ShopMode::Buy) ? SDL_Color{255, 240, 140, 255} : SDL_Color{180, 160, 140, 255}, 2);

    // SELL tab
    SDL_Color sellTabBg = (m_mode == ShopMode::Sell) ? SDL_Color{75, 55, 35, 255} : SDL_Color{40, 30, 25, 255};
    renderer.drawRect(WIN_X + 104, tabY, 80, 20, sellTabBg);
    renderer.drawOutline(WIN_X + 104, tabY, 80, 20, SDL_Color{160, 130, 85, 255});
    BitmapFont::drawText(renderer, "SELL", WIN_X + 128, tabY + 4, (m_mode == ShopMode::Sell) ? SDL_Color{255, 240, 140, 255} : SDL_Color{180, 160, 140, 255}, 2);

    renderer.drawLine(WIN_X + 10, tabY + 24, WIN_X + WIN_W - 10, tabY + 24, SDL_Color{115, 85, 50, 255});

    // 5. Items catalog list
    const auto& catalog = (m_mode == ShopMode::Buy) ? shopSystem.getSellerCatalog() : shopSystem.getBuyerCatalog();
    int listY = tabY + 30;
    constexpr int ITEM_H = 40;

    for (size_t i = 0; i < catalog.size(); ++i) {
        ItemType item = catalog[i];
        int itemY = listY + static_cast<int>(i) * (ITEM_H + 4);
        bool isSelected = (static_cast<int>(i) == m_selectedIndex);

        // Highlight box
        if (isSelected) {
            renderer.drawRect(WIN_X + 14, itemY, WIN_W - 28, ITEM_H, SDL_Color{65, 48, 35, 255});
            renderer.drawOutline(WIN_X + 14, itemY, WIN_W - 28, ITEM_H, SDL_Color{240, 200, 100, 255});
            BitmapFont::drawText(renderer, ">", WIN_X + 18, itemY + 12, SDL_Color{255, 225, 100, 255}, 2);
        } else {
            renderer.drawRect(WIN_X + 14, itemY, WIN_W - 28, ITEM_H, SDL_Color{42, 32, 26, 200});
            renderer.drawOutline(WIN_X + 14, itemY, WIN_W - 28, ITEM_H, SDL_Color{75, 55, 35, 255});
        }

        // Item icon
        drawItemPlaceholder(renderer, item, WIN_X + 30, itemY + 4, 32);

        // Item name
        SDL_Color nameCol = isSelected ? SDL_Color{255, 240, 160, 255} : SDL_Color{220, 200, 180, 255};
        BitmapFont::drawText(renderer, itemTypeToString(item), WIN_X + 70, itemY + 6, nameCol, 2);

        // Price & player count
        int inBag = inventory.getItemCount(item);
        if (m_mode == ShopMode::Buy) {
            int price = shopSystem.getBuyPrice(item);
            std::string priceStr = "Cost: " + std::to_string(price) + "G";
            BitmapFont::drawText(renderer, priceStr, WIN_X + 70, itemY + 22, SDL_Color{255, 215, 60, 255}, 1);

            std::string bagStr = "(In Bag: " + std::to_string(inBag) + ")";
            BitmapFont::drawText(renderer, bagStr, WIN_X + 160, itemY + 22, SDL_Color{160, 150, 140, 255}, 1);

            // Buy afford badge
            if (inventory.getMoney() >= price) {
                BitmapFont::drawText(renderer, "[BUY]", WIN_X + WIN_W - 75, itemY + 12, SDL_Color{120, 235, 120, 255}, 2);
            } else {
                BitmapFont::drawText(renderer, "[NO COINS]", WIN_X + WIN_W - 105, itemY + 12, SDL_Color{230, 90, 80, 255}, 1);
            }
        } else {
            int price = shopSystem.getSellPrice(item);
            std::string priceStr = "Payout: +" + std::to_string(price) + "G";
            BitmapFont::drawText(renderer, priceStr, WIN_X + 70, itemY + 22, SDL_Color{120, 235, 120, 255}, 1);

            std::string bagStr = "(In Bag: " + std::to_string(inBag) + ")";
            BitmapFont::drawText(renderer, bagStr, WIN_X + 180, itemY + 22, (inBag > 0) ? SDL_Color{220, 210, 180, 255} : SDL_Color{150, 100, 100, 255}, 1);

            if (inBag > 0) {
                BitmapFont::drawText(renderer, "[SELL]", WIN_X + WIN_W - 85, itemY + 12, SDL_Color{120, 235, 120, 255}, 2);
            } else {
                BitmapFont::drawText(renderer, "[NONE]", WIN_X + WIN_W - 75, itemY + 12, SDL_Color{140, 120, 110, 255}, 1);
            }
        }
    }

    // 6. Footer controls
    renderer.drawLine(WIN_X + 10, WIN_Y + WIN_H - 26, WIN_X + WIN_W - 10, WIN_Y + WIN_H - 26, SDL_Color{115, 85, 50, 255});
    BitmapFont::drawText(renderer, "[W/S] Select  [ENTER/SPACE] Transact  [TAB] Switch Tab  [ESC] Close",
                         WIN_X + 14, WIN_Y + WIN_H - 18, SDL_Color{185, 165, 140, 255}, 1);
}
