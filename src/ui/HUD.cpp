#include "ui/HUD.h"
#include "ui/BitmapFont.h"
#include <algorithm>

void HUD::update(float dt) {
    if (m_feedbackTimer > 0.0f) {
        m_feedbackTimer -= dt;
        if (m_feedbackTimer < 0.0f) {
            m_feedbackTimer = 0.0f;
        }
    }
}

void HUD::showFeedback(const std::string& message, float duration) {
    m_feedbackMessage = message;
    m_feedbackDuration = duration;
    m_feedbackTimer = duration;
}

void HUD::drawToolIcon(Renderer& renderer, ToolType type, int x, int y) const {
    switch (type) {
        case ToolType::Hoe: {
            // Wooden handle
            renderer.drawLine(x + 5, y + 19, x + 18, y + 6, SDL_Color{140, 85, 45, 255});
            renderer.drawLine(x + 6, y + 19, x + 19, y + 6, SDL_Color{140, 85, 45, 255});
            // Iron blade head
            renderer.drawRect(x + 16, y + 4, 6, 4, SDL_Color{180, 190, 205, 255});
            renderer.drawRect(x + 20, y + 4, 3, 9, SDL_Color{140, 150, 165, 255});
            break;
        }
        case ToolType::WateringCan: {
            // Blue watering can body
            renderer.drawRect(x + 6, y + 9, 12, 10, SDL_Color{65, 135, 205, 255});
            // Handle
            renderer.drawRect(x + 4, y + 10, 2, 8, SDL_Color{45, 105, 165, 255});
            renderer.drawRect(x + 4, y + 8, 8, 2, SDL_Color{45, 105, 165, 255});
            // Spout
            renderer.drawLine(x + 18, y + 12, x + 23, y + 7, SDL_Color{75, 155, 225, 255});
            renderer.drawLine(x + 18, y + 13, x + 23, y + 8, SDL_Color{75, 155, 225, 255});
            // Rose head
            renderer.drawRect(x + 22, y + 6, 3, 4, SDL_Color{200, 220, 240, 255});
            break;
        }
        case ToolType::TomatoSeeds: {
            // Seed packet pouch
            renderer.drawRect(x + 6, y + 6, 14, 16, SDL_Color{220, 200, 140, 255});
            renderer.drawRect(x + 8, y + 8, 10, 12, SDL_Color{115, 175, 80, 255});
            // Small red tomato illustration on packet
            renderer.drawRect(x + 11, y + 12, 4, 4, SDL_Color{230, 45, 35, 255});
            renderer.drawRect(x + 12, y + 10, 2, 2, SDL_Color{50, 140, 40, 255});
            break;
        }
        case ToolType::WheatSeeds: {
            // Seed packet pouch with golden wheat emblem
            renderer.drawRect(x + 6, y + 6, 14, 16, SDL_Color{220, 200, 140, 255});
            renderer.drawRect(x + 8, y + 8, 10, 12, SDL_Color{115, 175, 80, 255});
            renderer.drawRect(x + 11, y + 11, 4, 5, SDL_Color{240, 215, 60, 255});
            renderer.drawRect(x + 12, y + 9, 2, 2, SDL_Color{200, 160, 40, 255});
            break;
        }
    }
}

void HUD::drawCoinIcon(Renderer& renderer, int x, int y) const {
    // Golden coin with outer rim and inner shine
    renderer.drawRect(x + 2, y, 12, 16, SDL_Color{195, 145, 20, 255});
    renderer.drawRect(x, y + 2, 16, 12, SDL_Color{195, 145, 20, 255});
    renderer.drawRect(x + 2, y + 2, 12, 12, SDL_Color{255, 215, 40, 255});
    renderer.drawRect(x + 4, y + 4, 4, 4, SDL_Color{255, 250, 150, 255});
    renderer.drawRect(x + 7, y + 6, 3, 4, SDL_Color{180, 130, 15, 255});
}

void HUD::drawEnergyIcon(Renderer& renderer, int x, int y) const {
    // Lightning bolt stamina icon
    renderer.drawRect(x + 6, y, 4, 6, SDL_Color{255, 230, 50, 255});
    renderer.drawRect(x + 4, y + 5, 6, 4, SDL_Color{255, 230, 50, 255});
    renderer.drawRect(x + 2, y + 8, 8, 3, SDL_Color{255, 245, 120, 255});
    renderer.drawRect(x + 5, y + 11, 4, 4, SDL_Color{255, 220, 40, 255});
    renderer.drawRect(x + 6, y + 14, 2, 3, SDL_Color{255, 200, 30, 255});
}

void HUD::drawSunIcon(Renderer& renderer, int x, int y) const {
    // Golden sun badge
    renderer.drawRect(x + 4, y + 4, 10, 10, SDL_Color{255, 215, 40, 255});
    renderer.drawRect(x + 6, y + 6, 6, 6, SDL_Color{255, 245, 140, 255});
    // Cardinal rays
    renderer.drawRect(x + 8, y + 1, 2, 2, SDL_Color{255, 185, 30, 255});
    renderer.drawRect(x + 8, y + 15, 2, 2, SDL_Color{255, 185, 30, 255});
    renderer.drawRect(x + 1, y + 8, 2, 2, SDL_Color{255, 185, 30, 255});
    renderer.drawRect(x + 15, y + 8, 2, 2, SDL_Color{255, 185, 30, 255});
}

void HUD::render(Renderer& renderer, const Inventory& inventory, const Player& player, int currentDay) const {
    // 1. Hotbar background panel (top-left)
    constexpr int HOTBAR_X = 14;
    constexpr int HOTBAR_Y = 8;
    constexpr int SLOT_SIZE = 34;
    constexpr int SLOT_COUNT = 4;

    renderer.drawRect(HOTBAR_X - 4, HOTBAR_Y - 4,
                      (SLOT_SIZE * SLOT_COUNT) + 16, SLOT_SIZE + 8,
                      SDL_Color{25, 20, 18, 220});
    renderer.drawOutline(HOTBAR_X - 4, HOTBAR_Y - 4,
                         (SLOT_SIZE * SLOT_COUNT) + 16, SLOT_SIZE + 8,
                         SDL_Color{170, 140, 95, 255});

    const ToolType tools[SLOT_COUNT] = {
        ToolType::Hoe,
        ToolType::WateringCan,
        ToolType::TomatoSeeds,
        ToolType::WheatSeeds
    };

    for (int i = 0; i < SLOT_COUNT; ++i) {
        int slotX = HOTBAR_X + i * (SLOT_SIZE + 4);
        int slotY = HOTBAR_Y;
        bool isSelected = (inventory.activeTool == tools[i]);

        SDL_Color slotBg = isSelected ? SDL_Color{85, 65, 40, 240} : SDL_Color{45, 38, 32, 220};
        renderer.drawRect(slotX, slotY, SLOT_SIZE, SLOT_SIZE, slotBg);

        drawToolIcon(renderer, tools[i], slotX + 4, slotY + 4);

        // Key number (1, 2, 3, 4) in top-left
        BitmapFont::drawNumber(renderer, i + 1, slotX + 3, slotY + 3, SDL_Color{240, 230, 200, 255}, 1);

        // Seed count for seed tools
        if (tools[i] == ToolType::TomatoSeeds) {
            int seedCount = inventory.getItemCount(ItemType::TomatoSeeds);
            BitmapFont::drawNumber(renderer, seedCount, slotX + 18, slotY + 20, SDL_Color{255, 245, 140, 255}, 2);
        } else if (tools[i] == ToolType::WheatSeeds) {
            int seedCount = inventory.getItemCount(ItemType::WheatSeeds);
            BitmapFont::drawNumber(renderer, seedCount, slotX + 18, slotY + 20, SDL_Color{255, 245, 140, 255}, 2);
        }

        SDL_Color borderCol = isSelected ? SDL_Color{255, 220, 70, 255} : SDL_Color{110, 90, 70, 255};
        renderer.drawOutline(slotX, slotY, SLOT_SIZE, SLOT_SIZE, borderCol);
        if (isSelected) {
            renderer.drawOutline(slotX - 1, slotY - 1, SLOT_SIZE + 2, SLOT_SIZE + 2, SDL_Color{255, 245, 120, 200});
        }
    }

    // 2. Money / Wallet counter panel
    constexpr int COIN_PANEL_X = 168;
    constexpr int COIN_PANEL_Y = 8;
    constexpr int COIN_PANEL_W = 74;
    constexpr int COIN_PANEL_H = 38;

    renderer.drawRect(COIN_PANEL_X, COIN_PANEL_Y, COIN_PANEL_W, COIN_PANEL_H, SDL_Color{25, 20, 18, 220});
    renderer.drawOutline(COIN_PANEL_X, COIN_PANEL_Y, COIN_PANEL_W, COIN_PANEL_H, SDL_Color{170, 140, 95, 255});

    drawCoinIcon(renderer, COIN_PANEL_X + 6, COIN_PANEL_Y + 11);
    BitmapFont::drawNumber(renderer, inventory.getMoney(), COIN_PANEL_X + 26, COIN_PANEL_Y + 14, SDL_Color{255, 235, 140, 255}, 2);

    // 3. Energy / Stamina bar panel
    constexpr int ENG_PANEL_X = 246;
    constexpr int ENG_PANEL_Y = 8;
    constexpr int ENG_PANEL_W = 142;
    constexpr int ENG_PANEL_H = 38;

    renderer.drawRect(ENG_PANEL_X, ENG_PANEL_Y, ENG_PANEL_W, ENG_PANEL_H, SDL_Color{25, 20, 18, 220});
    renderer.drawOutline(ENG_PANEL_X, ENG_PANEL_Y, ENG_PANEL_W, ENG_PANEL_H, SDL_Color{170, 140, 95, 255});

    drawEnergyIcon(renderer, ENG_PANEL_X + 5, ENG_PANEL_Y + 10);

    // Energy gauge bar
    constexpr int BAR_X = ENG_PANEL_X + 22;
    constexpr int BAR_Y = ENG_PANEL_Y + 11;
    constexpr int BAR_MAX_W = 70;
    constexpr int BAR_H = 16;

    renderer.drawRect(BAR_X, BAR_Y, BAR_MAX_W, BAR_H, SDL_Color{45, 25, 25, 255}); // Empty slot background
    renderer.drawOutline(BAR_X, BAR_Y, BAR_MAX_W, BAR_H, SDL_Color{110, 90, 70, 255});

    float energyRatio = static_cast<float>(player.getEnergy()) / static_cast<float>(player.getMaxEnergy());
    energyRatio = std::clamp(energyRatio, 0.0f, 1.0f);
    int fillW = static_cast<int>((BAR_MAX_W - 2) * energyRatio);

    if (fillW > 0) {
        SDL_Color fillColor;
        if (energyRatio > 0.5f) {
            fillColor = SDL_Color{65, 200, 75, 255};  // Healthy green
        } else if (energyRatio > 0.2f) {
            fillColor = SDL_Color{240, 180, 35, 255}; // Caution amber
        } else {
            fillColor = SDL_Color{230, 45, 40, 255};  // Danger red
        }
        renderer.drawRect(BAR_X + 1, BAR_Y + 1, fillW, BAR_H - 2, fillColor);
    }

    // Numerical value of remaining energy
    BitmapFont::drawNumber(renderer, player.getEnergy(), ENG_PANEL_X + 96, ENG_PANEL_Y + 14, SDL_Color{255, 255, 255, 255}, 2);

    // 4. Milestone 6: Day Counter Panel
    constexpr int DAY_PANEL_X = 392;
    constexpr int DAY_PANEL_Y = 8;
    constexpr int DAY_PANEL_W = 86;
    constexpr int DAY_PANEL_H = 38;

    renderer.drawRect(DAY_PANEL_X, DAY_PANEL_Y, DAY_PANEL_W, DAY_PANEL_H, SDL_Color{25, 20, 18, 220});
    renderer.drawOutline(DAY_PANEL_X, DAY_PANEL_Y, DAY_PANEL_W, DAY_PANEL_H, SDL_Color{170, 140, 95, 255});

    drawSunIcon(renderer, DAY_PANEL_X + 5, DAY_PANEL_Y + 10);
    BitmapFont::drawText(renderer, "DAY", DAY_PANEL_X + 26, DAY_PANEL_Y + 8, SDL_Color{255, 225, 120, 255}, 1);
    BitmapFont::drawNumber(renderer, currentDay, DAY_PANEL_X + 26, DAY_PANEL_Y + 19, SDL_Color{255, 255, 255, 255}, 2);

    // 5. Inventory toggle key hint & Harvested produce panel
    constexpr int PROD_X = 482;
    constexpr int PROD_Y = 8;
    constexpr int PROD_W = 146;
    constexpr int PROD_H = 38;

    renderer.drawRect(PROD_X, PROD_Y, PROD_W, PROD_H, SDL_Color{25, 20, 18, 220});
    renderer.drawOutline(PROD_X, PROD_Y, PROD_W, PROD_H, SDL_Color{170, 140, 95, 255});

    BitmapFont::drawText(renderer, "TAB", PROD_X + 8, PROD_Y + 9, SDL_Color{255, 225, 120, 255}, 1);
    BitmapFont::drawText(renderer, "BAG", PROD_X + 8, PROD_Y + 21, SDL_Color{220, 210, 190, 255}, 1);

    int tomX = PROD_X + 48;
    int tomY = PROD_Y + 9;
    drawItemPlaceholder(const_cast<Renderer&>(renderer), ItemType::Tomato, tomX, tomY, 22);

    BitmapFont::drawChar(renderer, 'X', tomX + 26, tomY + 6, SDL_Color{230, 215, 180, 255}, 2);
    int tomatoCount = inventory.getItemCount(ItemType::Tomato);
    BitmapFont::drawNumber(renderer, tomatoCount, tomX + 42, tomY + 6, SDL_Color{255, 255, 255, 255}, 2);

    // 6. On-screen Feedback Banner (e.g. "NOT ENOUGH ENERGY!")
    if (m_feedbackTimer > 0.0f) {
        constexpr int BANNER_W = 320;
        constexpr int BANNER_H = 36;
        constexpr int BANNER_X = (Config::SCREEN_WIDTH - BANNER_W) / 2;
        constexpr int BANNER_Y = 56;

        renderer.drawRect(BANNER_X, BANNER_Y, BANNER_W, BANNER_H, SDL_Color{35, 15, 15, 240});
        renderer.drawOutline(BANNER_X, BANNER_Y, BANNER_W, BANNER_H, SDL_Color{240, 60, 50, 255});
        renderer.drawOutline(BANNER_X - 1, BANNER_Y - 1, BANNER_W + 2, BANNER_H + 2, SDL_Color{255, 160, 40, 180});

        // Warning '!' badge
        renderer.drawRect(BANNER_X + 10, BANNER_Y + 7, 20, 22, SDL_Color{210, 40, 30, 255});
        renderer.drawOutline(BANNER_X + 10, BANNER_Y + 7, 20, 22, SDL_Color{255, 220, 100, 255});
        BitmapFont::drawChar(renderer, '!', BANNER_X + 17, BANNER_Y + 11, SDL_Color{255, 255, 255, 255}, 2);

        // Feedback message text
        int textW = BitmapFont::getTextWidth(m_feedbackMessage, 2);
        int textX = BANNER_X + 38 + std::max(0, (BANNER_W - 48 - textW) / 2);
        BitmapFont::drawText(renderer, m_feedbackMessage, textX, BANNER_Y + 12, SDL_Color{255, 240, 120, 255}, 2);
    }
}
