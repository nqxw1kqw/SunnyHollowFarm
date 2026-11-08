#include "ui/InventoryUI.h"
#include "ui/BitmapFont.h"
#include <iostream>

void InventoryUI::handleInput(const Input& input, Inventory& inventory) {
    (void)inventory;
    if (!m_isOpen) return;

    // Close on Tab, I, or Escape
    if (input.isKeyPressed(SDL_SCANCODE_TAB) ||
        input.isKeyPressed(SDL_SCANCODE_I) ||
        input.isKeyPressed(SDL_SCANCODE_ESCAPE)) {
        close();
        return;
    }

    // Grid Navigation via Arrow Keys or WASD
    constexpr int COLS = Config::INVENTORY_COLS;
    constexpr int ROWS = Config::INVENTORY_ROWS;

    int col = m_selectedSlot % COLS;
    int row = m_selectedSlot / COLS;

    if (input.isKeyPressed(SDL_SCANCODE_LEFT) || input.isKeyPressed(SDL_SCANCODE_A)) {
        col = (col - 1 + COLS) % COLS;
    }
    if (input.isKeyPressed(SDL_SCANCODE_RIGHT) || input.isKeyPressed(SDL_SCANCODE_D)) {
        col = (col + 1) % COLS;
    }
    if (input.isKeyPressed(SDL_SCANCODE_UP) || input.isKeyPressed(SDL_SCANCODE_W)) {
        row = (row - 1 + ROWS) % ROWS;
    }
    if (input.isKeyPressed(SDL_SCANCODE_DOWN) || input.isKeyPressed(SDL_SCANCODE_S)) {
        row = (row + 1) % ROWS;
    }

    m_selectedSlot = row * COLS + col;
}

void InventoryUI::render(Renderer& renderer, const Inventory& inventory) const {
    if (!m_isOpen) return;

    // 1. Semi-transparent darkened background overlay
    renderer.drawRect(0, 0, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, SDL_Color{10, 8, 8, 175});

    // 2. Modal inventory window frame
    constexpr int WIN_W = 430;
    constexpr int WIN_H = 310;
    constexpr int WIN_X = (Config::SCREEN_WIDTH - WIN_W) / 2; // 105
    constexpr int WIN_Y = (Config::SCREEN_HEIGHT - WIN_H) / 2; // 85

    // Modal background & double border
    renderer.drawRect(WIN_X, WIN_Y, WIN_W, WIN_H, SDL_Color{35, 27, 22, 252});
    renderer.drawOutline(WIN_X, WIN_Y, WIN_W, WIN_H, SDL_Color{195, 160, 105, 255});
    renderer.drawOutline(WIN_X + 2, WIN_Y + 2, WIN_W - 4, WIN_H - 4, SDL_Color{115, 85, 50, 255});

    // 3. Header bar: "INVENTORY" and Money Counter
    BitmapFont::drawText(renderer, "INVENTORY", WIN_X + 18, WIN_Y + 12, SDL_Color{255, 230, 130, 255}, 2);

    // Coin wallet display in top right of modal
    int coinX = WIN_X + WIN_W - 140;
    int coinY = WIN_Y + 12;
    renderer.drawRect(coinX, coinY, 12, 12, SDL_Color{255, 215, 40, 255});
    renderer.drawOutline(coinX, coinY, 12, 12, SDL_Color{195, 145, 20, 255});
    BitmapFont::drawNumber(renderer, inventory.getMoney(), coinX + 16, coinY + 1, SDL_Color{255, 235, 140, 255}, 2);
    BitmapFont::drawText(renderer, "G", coinX + 16 + BitmapFont::getTextWidth(std::to_string(inventory.getMoney()), 2) + 4,
                         coinY + 1, SDL_Color{255, 205, 50, 255}, 2);

    // Separator line under header
    renderer.drawLine(WIN_X + 10, WIN_Y + 32, WIN_X + WIN_W - 10, WIN_Y + 32, SDL_Color{115, 85, 50, 255});

    // 4. Slots grid
    constexpr int COLS = Config::INVENTORY_COLS;
    constexpr int SLOT_SIZE = 42;
    constexpr int SLOT_GAP = 8;
    constexpr int GRID_W = COLS * SLOT_SIZE + (COLS - 1) * SLOT_GAP; // 6*42 + 5*8 = 292
    constexpr int GRID_X = WIN_X + (WIN_W - GRID_W) / 2;
    constexpr int GRID_Y = WIN_Y + 42;

    for (int i = 0; i < Config::INVENTORY_SLOT_COUNT; ++i) {
        int r = i / COLS;
        int c = i % COLS;
        int sx = GRID_X + c * (SLOT_SIZE + SLOT_GAP);
        int sy = GRID_Y + r * (SLOT_SIZE + SLOT_GAP);
        bool isSelected = (i == m_selectedSlot);

        // Slot interior
        SDL_Color slotBg = isSelected ? SDL_Color{75, 60, 48, 255} : SDL_Color{48, 38, 32, 255};
        renderer.drawRect(sx, sy, SLOT_SIZE, SLOT_SIZE, slotBg);
        renderer.drawSprite("ui_slot.png", sx, sy, SLOT_SIZE, SLOT_SIZE);

        // Slot border
        SDL_Color borderCol = isSelected ? SDL_Color{255, 230, 80, 255} : SDL_Color{95, 78, 62, 255};
        renderer.drawOutline(sx, sy, SLOT_SIZE, SLOT_SIZE, borderCol);
        if (isSelected) {
            renderer.drawOutline(sx - 1, sy - 1, SLOT_SIZE + 2, SLOT_SIZE + 2, SDL_Color{255, 245, 140, 200});
        }

        // Draw item if slot is occupied
        const auto& slot = inventory.getSlot(i);
        if (!slot.isEmpty()) {
            drawItemPlaceholder(renderer, slot.type, sx + 5, sy + 5, 32);

            // Item count in bottom-right of slot
            if (slot.count > 1) {
                // Drop shadow
                BitmapFont::drawNumber(renderer, slot.count, sx + 25, sy + 29, SDL_Color{20, 20, 20, 255}, 1);
                BitmapFont::drawNumber(renderer, slot.count, sx + 24, sy + 28, SDL_Color{255, 255, 255, 255}, 1);
            }
        }
    }

    // 5. Item Detail Panel (bottom of window)
    constexpr int DETAIL_X = WIN_X + 16;
    constexpr int DETAIL_Y = WIN_Y + 200;
    constexpr int DETAIL_W = WIN_W - 32;
    constexpr int DETAIL_H = 74;

    renderer.drawRect(DETAIL_X, DETAIL_Y, DETAIL_W, DETAIL_H, SDL_Color{24, 18, 14, 240});
    renderer.drawOutline(DETAIL_X, DETAIL_Y, DETAIL_W, DETAIL_H, SDL_Color{120, 95, 65, 255});

    const auto& selectedSlot = inventory.getSlot(m_selectedSlot);
    if (!selectedSlot.isEmpty()) {
        const ItemDefinition& def = getItemDefinition(selectedSlot.type);

        // Icon preview
        drawItemPlaceholder(renderer, selectedSlot.type, DETAIL_X + 10, DETAIL_Y + 12, 48);

        // Item Name
        BitmapFont::drawText(renderer, def.name, DETAIL_X + 68, DETAIL_Y + 12, SDL_Color{255, 230, 130, 255}, 2);

        // Item Count badge
        std::string countStr = "X " + std::to_string(selectedSlot.count);
        BitmapFont::drawText(renderer, countStr, DETAIL_X + 68 + BitmapFont::getTextWidth(def.name, 2) + 12,
                             DETAIL_Y + 14, SDL_Color{220, 200, 150, 255}, 1);

        // Description
        BitmapFont::drawText(renderer, def.description, DETAIL_X + 68, DETAIL_Y + 34, SDL_Color{210, 195, 175, 255}, 1);

        // Extra details (seed / food restore)
        if (def.isFood && def.energyRestore > 0) {
            std::string foodInfo = "+ " + std::to_string(def.energyRestore) + " ENERGY WHEN EATEN";
            BitmapFont::drawText(renderer, foodInfo, DETAIL_X + 68, DETAIL_Y + 50, SDL_Color{100, 220, 110, 255}, 1);
        } else if (def.isSeed) {
            BitmapFont::drawText(renderer, "PLANTABLE SEED: READY FOR TILLED DIRT", DETAIL_X + 68, DETAIL_Y + 50, SDL_Color{140, 210, 130, 255}, 1);
        } else {
            std::string valueStr = "SELL VALUE: " + std::to_string(def.sellPrice) + " COINS";
            BitmapFont::drawText(renderer, valueStr, DETAIL_X + 68, DETAIL_Y + 50, SDL_Color{240, 210, 100, 255}, 1);
        }
    } else {
        BitmapFont::drawText(renderer, "EMPTY SLOT", DETAIL_X + 16, DETAIL_Y + 18, SDL_Color{150, 135, 120, 255}, 2);
        BitmapFont::drawText(renderer, "SELECT A SLOT WITH AN ITEM TO VIEW ITS DETAILS.",
                             DETAIL_X + 16, DETAIL_Y + 40, SDL_Color{120, 105, 90, 255}, 1);
    }

    // 6. Navigation Controls Footer Hint
    BitmapFont::drawText(renderer, "[TAB / I / ESC] CLOSE   [ARROWS / WASD] SELECT",
                         WIN_X + 38, WIN_Y + WIN_H - 18, SDL_Color{170, 145, 110, 255}, 1);
}
