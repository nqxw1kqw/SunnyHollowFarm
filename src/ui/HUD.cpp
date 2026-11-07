#include "ui/HUD.h"
#include <string>

// 3x5 bitmap patterns for digits 0-9
static const unsigned short DIGIT_PATTERNS[10] = {
    0x7B6F, // 0: 111 101 101 101 111
    0x2492, // 1: 010 010 010 010 010
    0x73E7, // 2: 111 001 111 100 111
    0x73CF, // 3: 111 001 111 001 111
    0x5BC9, // 4: 101 101 111 001 001
    0x79CF, // 5: 111 100 111 001 111
    0x79EF, // 6: 111 100 111 101 111
    0x7249, // 7: 111 001 010 010 010
    0x7BEF, // 8: 111 101 111 101 111
    0x7BCF  // 9: 111 101 111 001 111
};

void HUD::drawNumber(Renderer& renderer, int number, int x, int y, SDL_Color color) const {
    std::string str = std::to_string(number);
    int curX = x;
    constexpr int PIXEL_SCALE = 2;

    for (char ch : str) {
        if (ch >= '0' && ch <= '9') {
            int digit = ch - '0';
            unsigned short pattern = DIGIT_PATTERNS[digit];
            for (int row = 0; row < 5; ++row) {
                for (int col = 0; col < 3; ++col) {
                    int bitIndex = 14 - (row * 3 + col);
                    if ((pattern >> bitIndex) & 1) {
                        renderer.drawRect(curX + col * PIXEL_SCALE, y + row * PIXEL_SCALE,
                                          PIXEL_SCALE, PIXEL_SCALE, color);
                    }
                }
            }
            curX += (3 * PIXEL_SCALE) + 2;
        }
    }
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
    }
}

void HUD::render(Renderer& renderer, const Inventory& inventory) const {
    // 1. Hotbar background panel (centered at top)
    constexpr int HOTBAR_X = 14;
    constexpr int HOTBAR_Y = 8;
    constexpr int SLOT_SIZE = 34;
    constexpr int SLOT_COUNT = 3;

    // Hotbar panel backing
    renderer.drawRect(HOTBAR_X - 4, HOTBAR_Y - 4,
                      (SLOT_SIZE * SLOT_COUNT) + 16, SLOT_SIZE + 8,
                      SDL_Color{25, 20, 18, 220});
    renderer.drawOutline(HOTBAR_X - 4, HOTBAR_Y - 4,
                         (SLOT_SIZE * SLOT_COUNT) + 16, SLOT_SIZE + 8,
                         SDL_Color{170, 140, 95, 255});

    // 2. Render tool slots: [1] Hoe, [2] Watering Can, [3] Tomato Seeds
    const ToolType tools[SLOT_COUNT] = {
        ToolType::Hoe,
        ToolType::WateringCan,
        ToolType::TomatoSeeds
    };

    for (int i = 0; i < SLOT_COUNT; ++i) {
        int slotX = HOTBAR_X + i * (SLOT_SIZE + 4);
        int slotY = HOTBAR_Y;
        bool isSelected = (inventory.activeTool == tools[i]);

        // Slot interior
        SDL_Color slotBg = isSelected ? SDL_Color{85, 65, 40, 240} : SDL_Color{45, 38, 32, 220};
        renderer.drawRect(slotX, slotY, SLOT_SIZE, SLOT_SIZE, slotBg);

        // Tool Icon
        drawToolIcon(renderer, tools[i], slotX + 4, slotY + 4);

        // Slot index number (1, 2, 3) in top-left of slot
        drawNumber(renderer, i + 1, slotX + 3, slotY + 3, SDL_Color{240, 230, 200, 255});

        // If this is seeds, show seed count in bottom-right
        if (tools[i] == ToolType::TomatoSeeds) {
            drawNumber(renderer, inventory.tomatoSeeds, slotX + 18, slotY + 20, SDL_Color{255, 245, 140, 255});
        }

        // Slot outline border
        SDL_Color borderCol = isSelected ? SDL_Color{255, 220, 70, 255} : SDL_Color{110, 90, 70, 255};
        renderer.drawOutline(slotX, slotY, SLOT_SIZE, SLOT_SIZE, borderCol);
        if (isSelected) {
            // Extra thickness for selected highlight
            renderer.drawOutline(slotX - 1, slotY - 1, SLOT_SIZE + 2, SLOT_SIZE + 2, SDL_Color{255, 245, 120, 200});
        }
    }

    // 3. Harvested produce panel on right: [Tomato Icon] x <count>
    constexpr int PROD_X = 500;
    constexpr int PROD_Y = 8;
    constexpr int PROD_W = 126;
    constexpr int PROD_H = 38;

    renderer.drawRect(PROD_X, PROD_Y, PROD_W, PROD_H, SDL_Color{25, 20, 18, 220});
    renderer.drawOutline(PROD_X, PROD_Y, PROD_W, PROD_H, SDL_Color{170, 140, 95, 255});

    // Tomato icon
    int tomX = PROD_X + 10;
    int tomY = PROD_Y + 10;
    renderer.drawRect(tomX + 2, tomY + 4, 16, 14, SDL_Color{225, 45, 35, 255});
    renderer.drawRect(tomX + 4, tomY + 6, 4, 4, SDL_Color{255, 120, 105, 255});
    // Green calyx
    renderer.drawRect(tomX + 8, tomY + 1, 4, 3, SDL_Color{45, 150, 40, 255});
    renderer.drawRect(tomX + 6, tomY + 3, 8, 2, SDL_Color{45, 150, 40, 255});

    // 'x' character marker
    renderer.drawLine(tomX + 24, tomY + 8, tomX + 28, tomY + 14, SDL_Color{230, 215, 180, 255});
    renderer.drawLine(tomX + 28, tomY + 8, tomX + 24, tomY + 14, SDL_Color{230, 215, 180, 255});

    // Tomato count number
    drawNumber(renderer, inventory.harvestedTomatoes, tomX + 34, tomY + 6, SDL_Color{255, 255, 255, 255});
}
