#include "ui/StatsUI.h"
#include "ui/BitmapFont.h"
#include "core/GameConfig.h"
#include "systems/AudioSystem.h"
#include "systems/StatsSystem.h"
#include <vector>
#include <string>

void StatsUI::open(bool fromPause) {
    m_isOpen = true;
    m_fromPause = fromPause;
}

void StatsUI::close() {
    m_isOpen = false;
    m_fromPause = false;
}

void StatsUI::toggle() {
    if (m_isOpen) {
        close();
    } else {
        open(false);
    }
}

bool StatsUI::handleInput(const Input& input, AudioSystem* audio) {
    if (!m_isOpen) return false;

    if (input.isKeyPressed(SDL_SCANCODE_ESCAPE) ||
        input.isKeyPressed(SDL_SCANCODE_RETURN) ||
        input.isKeyPressed(SDL_SCANCODE_SPACE) ||
        input.isKeyPressed(SDL_SCANCODE_T)) {
        close();
        if (audio) {
            audio->playSFX(SFX::UIConfirm);
        }
        return true;
    }

    return false;
}

void StatsUI::render(Renderer& renderer, int day, int wallet, const StatsSystem& stats, int chickensOwned) const {
    if (!m_isOpen) return;

    // 1. Semi-transparent backdrop overlay
    renderer.drawRect(0, 0, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, SDL_Color{10, 8, 8, 180});

    // 2. Centered panel frame
    constexpr int WIN_W = 380;
    constexpr int WIN_H = 300;
    constexpr int WIN_X = (Config::SCREEN_WIDTH - WIN_W) / 2;
    constexpr int WIN_Y = (Config::SCREEN_HEIGHT - WIN_H) / 2;

    bool hasPanel = renderer.drawNineSlice("ui_panel.png", WIN_X, WIN_Y, WIN_W, WIN_H, 3, 3, 3, 3);
    if (!hasPanel) {
        renderer.drawRect(WIN_X, WIN_Y, WIN_W, WIN_H, SDL_Color{35, 27, 22, 252});
        renderer.drawOutline(WIN_X, WIN_Y, WIN_W, WIN_H, SDL_Color{195, 160, 105, 255});
        renderer.drawOutline(WIN_X + 2, WIN_Y + 2, WIN_W - 4, WIN_H - 4, SDL_Color{115, 85, 50, 255});
    }

    // 3. Header title
    std::string header = "FARM STATISTICS";
    int headerW = BitmapFont::getTextWidth(header, 2);
    int headerX = WIN_X + (WIN_W - headerW) / 2;
    SDL_Color headerCol = hasPanel ? SDL_Color{60, 36, 22, 255} : SDL_Color{255, 230, 130, 255};
    BitmapFont::drawText(renderer, header, headerX, WIN_Y + 16, headerCol, 2);

    // Divider line
    renderer.drawLine(WIN_X + 20, WIN_Y + 38, WIN_X + WIN_W - 20, WIN_Y + 38,
                      hasPanel ? SDL_Color{160, 130, 85, 180} : SDL_Color{115, 85, 50, 255});

    // 4. Metric rows
    struct MetricRow {
        std::string label;
        std::string value;
    };

    std::vector<MetricRow> rows = {
        {"DAY", std::to_string(day)},
        {"WALLET", std::to_string(wallet) + "G"},
        {"LIFETIME EARNED", std::to_string(stats.getLifetimeEarnedGold()) + "G"},
        {"CROPS HARVESTED", std::to_string(stats.getCropsHarvested())},
        {"FISH CAUGHT", std::to_string(stats.getFishCaught())},
        {"EGGS COLLECTED", std::to_string(stats.getEggsCollected())},
        {"DISHES COOKED", std::to_string(stats.getDishesCooked())},
        {"CHICKENS OWNED", std::to_string(chickensOwned)}
    };

    int rowY = WIN_Y + 52;
    constexpr int ROW_SPACING = 24;
    SDL_Color labelCol = hasPanel ? SDL_Color{75, 45, 25, 255} : SDL_Color{240, 220, 180, 255};
    SDL_Color valueCol = hasPanel ? SDL_Color{120, 60, 15, 255} : SDL_Color{255, 240, 120, 255};

    for (const auto& row : rows) {
        // Subtle row background strip
        renderer.drawRect(WIN_X + 20, rowY - 2, WIN_W - 40, 20,
                          hasPanel ? SDL_Color{195, 175, 140, 60} : SDL_Color{50, 40, 32, 120});

        BitmapFont::drawText(renderer, row.label, WIN_X + 30, rowY + 2, labelCol, 2);

        int valW = BitmapFont::getTextWidth(row.value, 2);
        BitmapFont::drawText(renderer, row.value, WIN_X + WIN_W - 30 - valW, rowY + 2, valueCol, 2);

        rowY += ROW_SPACING;
    }

    // 5. Footer instructions
    std::string hint = "[ESC / ENTER / T] CLOSE";
    int hintW = BitmapFont::getTextWidth(hint, 2);
    int hintX = WIN_X + (WIN_W - hintW) / 2;
    SDL_Color hintCol = hasPanel ? SDL_Color{110, 85, 65, 255} : SDL_Color{180, 160, 130, 255};
    BitmapFont::drawText(renderer, hint, hintX, WIN_Y + WIN_H - 24, hintCol, 2);
}
