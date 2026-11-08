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
    const char* sprite = nullptr;
    switch (type) {
        case ToolType::Hoe:         sprite = "tool_hoe.png"; break;
        case ToolType::WateringCan: sprite = "tool_watering_can.png"; break;
        case ToolType::TomatoSeeds:
        case ToolType::WheatSeeds:
        case ToolType::CarrotSeeds: sprite = "item_seed_packet.png"; break;
        case ToolType::Tomato:      sprite = "item_tomato.png"; break;
        case ToolType::Salad:       sprite = "item_salad.png"; break;
        case ToolType::Bread:       sprite = "item_bread.png"; break;
    }
    if (sprite && renderer.drawSprite(sprite, x, y, 26, 26)) return;

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
        case ToolType::TomatoSeeds:
        case ToolType::WheatSeeds:
        case ToolType::CarrotSeeds: {
            // Seed packet pouch with color-coded emblem
            renderer.drawRect(x + 6, y + 6, 14, 16, SDL_Color{220, 200, 140, 255});
            renderer.drawRect(x + 8, y + 8, 10, 12, SDL_Color{115, 175, 80, 255});
            SDL_Color emblem = (type == ToolType::TomatoSeeds) ? SDL_Color{230, 45, 35, 255} :
                               (type == ToolType::WheatSeeds)  ? SDL_Color{240, 215, 60, 255} :
                               SDL_Color{240, 130, 30, 255};
            renderer.drawRect(x + 11, y + 11, 4, 5, emblem);
            renderer.drawRect(x + 12, y + 9, 2, 2, SDL_Color{50, 140, 40, 255});
            break;
        }
        case ToolType::Tomato:
            drawItemPlaceholder(const_cast<Renderer&>(renderer), ItemType::Tomato, x + 2, y + 2, 22);
            break;
        case ToolType::Salad:
            drawItemPlaceholder(const_cast<Renderer&>(renderer), ItemType::Salad, x + 2, y + 2, 22);
            break;
        case ToolType::Bread:
            drawItemPlaceholder(const_cast<Renderer&>(renderer), ItemType::Bread, x + 2, y + 2, 22);
            break;
    }
}

void HUD::drawCoinIcon(Renderer& renderer, int x, int y) const {
    if (renderer.drawSprite("ui_coin.png", x, y, 16, 16)) return;
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

void HUD::drawRainIcon(Renderer& renderer, int x, int y) const {
    // Rain cloud body (soft cool slate blue-gray)
    SDL_Color cloudColor{160, 185, 215, 255};
    renderer.drawRect(x + 3, y + 4, 10, 6, cloudColor);
    renderer.drawRect(x + 5, y + 2, 6, 2, cloudColor);
    renderer.drawRect(x + 1, y + 6, 2, 4, cloudColor);
    renderer.drawRect(x + 13, y + 6, 2, 4, cloudColor);
    // Rain streaks / drops (bright sky blue)
    SDL_Color dropColor{100, 180, 255, 255};
    renderer.drawLine(x + 3, y + 12, x + 2, y + 15, dropColor);
    renderer.drawLine(x + 8, y + 12, x + 7, y + 15, dropColor);
    renderer.drawLine(x + 13, y + 12, x + 12, y + 15, dropColor);
}

void HUD::render(Renderer& renderer, const Inventory& inventory, const Player& player, int currentDay, Weather weather) const {
    // --- TOP HUD: Wallet, Energy, Day, Weather, and Bag Hint ---

    // 1. Money / Wallet counter panel (top-left)
    constexpr int COIN_PANEL_X = 14;
    constexpr int COIN_PANEL_Y = 8;
    constexpr int COIN_PANEL_W = 86;
    constexpr int COIN_PANEL_H = 38;

    renderer.drawRect(COIN_PANEL_X, COIN_PANEL_Y, COIN_PANEL_W, COIN_PANEL_H, SDL_Color{25, 20, 18, 220});
    renderer.drawOutline(COIN_PANEL_X, COIN_PANEL_Y, COIN_PANEL_W, COIN_PANEL_H, SDL_Color{170, 140, 95, 255});

    drawCoinIcon(renderer, COIN_PANEL_X + 6, COIN_PANEL_Y + 11);
    BitmapFont::drawNumber(renderer, inventory.getMoney(), COIN_PANEL_X + 26, COIN_PANEL_Y + 14, SDL_Color{255, 235, 140, 255}, 2);

    // 2. Energy / Stamina bar panel (top-center-left)
    constexpr int ENG_PANEL_X = 108;
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

    renderer.drawRect(BAR_X, BAR_Y, BAR_MAX_W, BAR_H, SDL_Color{45, 25, 25, 255});
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
    const SDL_Rect frameSource{0, 0, 32, 16};
    if (renderer.drawSprite("ui_energy_bar.png", BAR_X, BAR_Y, BAR_MAX_W, BAR_H, &frameSource)) {
        renderer.drawRect(BAR_X + 5, BAR_Y + 4, BAR_MAX_W - 10, 8, SDL_Color{45, 25, 25, 255});
        if (energyRatio > 0.0f) {
            const SDL_Rect fillSource{2, 20, std::max(1, static_cast<int>(28 * energyRatio)), 8};
            renderer.drawSprite("ui_energy_bar.png", BAR_X + 5, BAR_Y + 4,
                                std::max(1, static_cast<int>((BAR_MAX_W - 10) * energyRatio)), 8, &fillSource);
        }
    }

    BitmapFont::drawNumber(renderer, player.getEnergy(), ENG_PANEL_X + 96, ENG_PANEL_Y + 14, SDL_Color{255, 255, 255, 255}, 2);

    // 3. Day Counter Panel (top-center)
    constexpr int DAY_PANEL_X = 258;
    constexpr int DAY_PANEL_Y = 8;
    constexpr int DAY_PANEL_W = 86;
    constexpr int DAY_PANEL_H = 38;

    renderer.drawRect(DAY_PANEL_X, DAY_PANEL_Y, DAY_PANEL_W, DAY_PANEL_H, SDL_Color{25, 20, 18, 220});
    renderer.drawOutline(DAY_PANEL_X, DAY_PANEL_Y, DAY_PANEL_W, DAY_PANEL_H, SDL_Color{170, 140, 95, 255});

    if (!renderer.drawSprite("ui_sleep_icon.png", DAY_PANEL_X + 4, DAY_PANEL_Y + 8, 20, 20)) {
        drawSunIcon(renderer, DAY_PANEL_X + 5, DAY_PANEL_Y + 10);
    }
    BitmapFont::drawText(renderer, "DAY", DAY_PANEL_X + 26, DAY_PANEL_Y + 8, SDL_Color{255, 225, 120, 255}, 1);
    BitmapFont::drawNumber(renderer, currentDay, DAY_PANEL_X + 26, DAY_PANEL_Y + 19, SDL_Color{255, 255, 255, 255}, 2);

    // 3b. Weather Indicator Panel (top-center, right next to Day Counter)
    constexpr int WEATH_PANEL_X = 348;
    constexpr int WEATH_PANEL_Y = 8;
    constexpr int WEATH_PANEL_W = 46;
    constexpr int WEATH_PANEL_H = 38;

    renderer.drawRect(WEATH_PANEL_X, WEATH_PANEL_Y, WEATH_PANEL_W, WEATH_PANEL_H, SDL_Color{25, 20, 18, 220});
    renderer.drawOutline(WEATH_PANEL_X, WEATH_PANEL_Y, WEATH_PANEL_W, WEATH_PANEL_H, SDL_Color{170, 140, 95, 255});

    if (weather == Weather::Rainy) {
        drawRainIcon(renderer, WEATH_PANEL_X + 14, WEATH_PANEL_Y + 5);
        BitmapFont::drawText(renderer, "RAIN", WEATH_PANEL_X + 11, WEATH_PANEL_Y + 24, SDL_Color{140, 200, 255, 255}, 1);
    } else {
        drawSunIcon(renderer, WEATH_PANEL_X + 14, WEATH_PANEL_Y + 5);
        BitmapFont::drawText(renderer, "SUN", WEATH_PANEL_X + 14, WEATH_PANEL_Y + 24, SDL_Color{255, 225, 120, 255}, 1);
    }

    // 4. Inventory toggle key hint & Harvested produce panel (top-right)
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

    // --- BOTTOM HOTBAR: Tool/Item slots 1-8 (bottom-center) ---
    const int panelX = HOTBAR_X - 6;
    const int panelY = HOTBAR_Y - 5;
    const int panelW = HOTBAR_TOTAL_WIDTH + 12;
    const int panelH = HOTBAR_SLOT_SIZE + 10;

    bool hasPanel = const_cast<Renderer&>(renderer).drawNineSlice("ui_panel.png", panelX, panelY, panelW, panelH, 3, 3, 3, 3);
    if (!hasPanel) {
        renderer.drawRect(panelX, panelY, panelW, panelH, SDL_Color{25, 20, 18, 220});
        renderer.drawOutline(panelX, panelY, panelW, panelH, SDL_Color{170, 140, 95, 255});
    }

    const ToolType tools[HOTBAR_SLOT_COUNT] = {
        ToolType::Hoe,
        ToolType::WateringCan,
        ToolType::TomatoSeeds,
        ToolType::WheatSeeds,
        ToolType::CarrotSeeds,
        ToolType::Tomato,
        ToolType::Salad,
        ToolType::Bread
    };

    for (int i = 0; i < HOTBAR_SLOT_COUNT; ++i) {
        int slotX = HOTBAR_X + i * (HOTBAR_SLOT_SIZE + HOTBAR_SLOT_SPACING);
        int slotY = HOTBAR_Y;
        bool isSelected = (inventory.activeTool == tools[i]);

        SDL_Color slotBg = isSelected ? SDL_Color{85, 65, 40, 240} : SDL_Color{45, 38, 32, 220};
        renderer.drawRect(slotX, slotY, HOTBAR_SLOT_SIZE, HOTBAR_SLOT_SIZE, slotBg);
        renderer.drawSprite("ui_slot.png", slotX, slotY, HOTBAR_SLOT_SIZE, HOTBAR_SLOT_SIZE);

        drawToolIcon(renderer, tools[i], slotX + 4, slotY + 4);

        // Key number (1..8) in top-left
        BitmapFont::drawNumber(renderer, i + 1, slotX + 3, slotY + 3, SDL_Color{240, 230, 200, 255}, 1);

        // Quantity count for items / seeds
        int count = -1;
        switch (tools[i]) {
            case ToolType::TomatoSeeds: count = inventory.getItemCount(ItemType::TomatoSeeds); break;
            case ToolType::WheatSeeds:  count = inventory.getItemCount(ItemType::WheatSeeds); break;
            case ToolType::CarrotSeeds: count = inventory.getItemCount(ItemType::CarrotSeeds); break;
            case ToolType::Tomato:      count = inventory.getItemCount(ItemType::Tomato); break;
            case ToolType::Salad:       count = inventory.getItemCount(ItemType::Salad); break;
            case ToolType::Bread:       count = inventory.getItemCount(ItemType::Bread); break;
            default: break;
        }
        if (count >= 0) {
            BitmapFont::drawNumber(renderer, count, slotX + 18, slotY + 20, SDL_Color{255, 245, 140, 255}, 2);
        }

        SDL_Color borderCol = isSelected ? SDL_Color{255, 220, 70, 255} : SDL_Color{110, 90, 70, 255};
        renderer.drawOutline(slotX, slotY, HOTBAR_SLOT_SIZE, HOTBAR_SLOT_SIZE, borderCol);
        if (isSelected) {
            renderer.drawOutline(slotX - 1, slotY - 1, HOTBAR_SLOT_SIZE + 2, HOTBAR_SLOT_SIZE + 2, SDL_Color{255, 245, 120, 200});
        }
    }

    // 6. On-screen Feedback Banner (e.g. "NOT ENOUGH ENERGY!")
    renderFeedbackBanner(renderer);
}

void HUD::renderFeedbackBanner(Renderer& renderer) const {
    if (m_feedbackTimer <= 0.0f) return;

    int textW = BitmapFont::getTextWidth(m_feedbackMessage, 2);
    int BANNER_W = std::max(320, textW + 54);
    int BANNER_H = 36;
    int BANNER_X = (Config::SCREEN_WIDTH - BANNER_W) / 2;
    constexpr int BANNER_Y = 56;

    bool hasDialogue = const_cast<Renderer&>(renderer).drawDialogue(BANNER_X, BANNER_Y, BANNER_W, BANNER_H, false);
    if (!hasDialogue) {
        renderer.drawRect(BANNER_X, BANNER_Y, BANNER_W, BANNER_H, SDL_Color{35, 15, 15, 240});
        renderer.drawOutline(BANNER_X, BANNER_Y, BANNER_W, BANNER_H, SDL_Color{240, 60, 50, 255});
        renderer.drawOutline(BANNER_X - 1, BANNER_Y - 1, BANNER_W + 2, BANNER_H + 2, SDL_Color{255, 160, 40, 180});
    }

    // Warning '!' badge
    renderer.drawRect(BANNER_X + 10, BANNER_Y + 7, 20, 22, SDL_Color{210, 40, 30, 255});
    renderer.drawOutline(BANNER_X + 10, BANNER_Y + 7, 20, 22, SDL_Color{255, 220, 100, 255});
    BitmapFont::drawChar(renderer, '!', BANNER_X + 17, BANNER_Y + 11, SDL_Color{255, 255, 255, 255}, 2);

    // Feedback message text
    int textX = BANNER_X + 38 + std::max(0, (BANNER_W - 48 - textW) / 2);
    SDL_Color textCol = hasDialogue ? SDL_Color{60, 35, 25, 255} : SDL_Color{255, 240, 120, 255};
    BitmapFont::drawText(renderer, m_feedbackMessage, textX, BANNER_Y + 12, textCol, 2);
}
