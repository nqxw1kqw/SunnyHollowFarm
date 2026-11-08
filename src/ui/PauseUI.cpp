#include "ui/PauseUI.h"
#include "ui/BitmapFont.h"
#include "core/GameConfig.h"
#include "systems/AudioSystem.h"
#include <vector>
#include <string>

void PauseUI::open() {
    m_isOpen = true;
    m_selectedIndex = 0;
}

void PauseUI::close() {
    m_isOpen = false;
}

PauseAction PauseUI::handleInput(const Input& input, AudioSystem* audio) {
    if (!m_isOpen) return PauseAction::None;

    // Esc resumes gameplay
    if (input.isKeyPressed(SDL_SCANCODE_ESCAPE)) {
        close();
        if (audio) audio->playSFX(SFX::UIConfirm);
        return PauseAction::Resume;
    }

    // Navigate Up/Down or W/S
    if (input.isKeyPressed(SDL_SCANCODE_UP) || input.isKeyPressed(SDL_SCANCODE_W)) {
        m_selectedIndex = (m_selectedIndex - 1 + OPTION_COUNT) % OPTION_COUNT;
        if (audio) audio->playSFX(SFX::UIConfirm);
    }
    if (input.isKeyPressed(SDL_SCANCODE_DOWN) || input.isKeyPressed(SDL_SCANCODE_S)) {
        m_selectedIndex = (m_selectedIndex + 1) % OPTION_COUNT;
        if (audio) audio->playSFX(SFX::UIConfirm);
    }

    // Select with Enter, Space, or Return
    if (input.isKeyPressed(SDL_SCANCODE_RETURN) || input.isKeyPressed(SDL_SCANCODE_SPACE)) {
        if (audio) audio->playSFX(SFX::UIConfirm);
        switch (m_selectedIndex) {
            case 0:
                close();
                return PauseAction::Resume;
            case 1:
                // Keep pause menu open so user can see banner and continue managing game
                return PauseAction::SaveNow;
            case 2:
                return PauseAction::ViewStats;
            case 3:
                close();
                return PauseAction::QuitToTitle;
            case 4:
                close();
                return PauseAction::QuitGame;
            default:
                break;
        }
    }

    return PauseAction::None;
}

void PauseUI::render(Renderer& renderer) const {
    if (!m_isOpen) return;

    // 1. Semi-transparent dark overlay
    renderer.drawRect(0, 0, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, SDL_Color{10, 8, 8, 175});

    // 2. Centered panel
    constexpr int WIN_W = 260;
    constexpr int WIN_H = 260;
    constexpr int WIN_X = (Config::SCREEN_WIDTH - WIN_W) / 2;
    constexpr int WIN_Y = (Config::SCREEN_HEIGHT - WIN_H) / 2;

    bool hasPanel = renderer.drawNineSlice("ui_panel.png", WIN_X, WIN_Y, WIN_W, WIN_H, 3, 3, 3, 3);
    if (!hasPanel) {
        renderer.drawRect(WIN_X, WIN_Y, WIN_W, WIN_H, SDL_Color{35, 27, 22, 252});
        renderer.drawOutline(WIN_X, WIN_Y, WIN_W, WIN_H, SDL_Color{195, 160, 105, 255});
        renderer.drawOutline(WIN_X + 2, WIN_Y + 2, WIN_W - 4, WIN_H - 4, SDL_Color{115, 85, 50, 255});
    }

    // 3. Header title
    std::string title = "PAUSED";
    int titleW = BitmapFont::getTextWidth(title, 2);
    int titleX = WIN_X + (WIN_W - titleW) / 2;
    SDL_Color titleCol = hasPanel ? SDL_Color{60, 36, 22, 255} : SDL_Color{255, 230, 130, 255};
    BitmapFont::drawText(renderer, title, titleX, WIN_Y + 16, titleCol, 2);

    renderer.drawLine(WIN_X + 24, WIN_Y + 36, WIN_X + WIN_W - 24, WIN_Y + 36,
                      hasPanel ? SDL_Color{160, 130, 85, 180} : SDL_Color{115, 85, 50, 255});

    // 4. Menu options
    static const char* const OPTIONS[OPTION_COUNT] = {
        "RESUME",
        "SAVE NOW",
        "VIEW STATS",
        "QUIT TO TITLE",
        "QUIT GAME"
    };

    constexpr int BTN_W = 190;
    constexpr int BTN_H = 28;
    constexpr int BTN_X = WIN_X + (WIN_W - BTN_W) / 2;
    int btnY = WIN_Y + 48;
    constexpr int BTN_SPACING = 34;

    for (int i = 0; i < OPTION_COUNT; ++i) {
        bool selected = (i == m_selectedIndex);
        const char* btnSprite = selected ? "ui_button_hover.png" : "ui_button.png";

        if (!renderer.drawNineSlice(btnSprite, BTN_X, btnY, BTN_W, BTN_H, 8, 6, 8, 6)) {
            SDL_Color bgCol = selected ? SDL_Color{75, 55, 35, 255} : SDL_Color{45, 35, 28, 255};
            renderer.drawRect(BTN_X, btnY, BTN_W, BTN_H, bgCol);
            renderer.drawOutline(BTN_X, btnY, BTN_W, BTN_H,
                                 selected ? SDL_Color{255, 220, 100, 255} : SDL_Color{140, 110, 80, 255});
        }

        if (selected) {
            // Draw cursor indicator on left of selected button
            if (!renderer.drawSprite("ui_cursor.png", BTN_X - 18, btnY + 2, 20, 20)) {
                BitmapFont::drawChar(renderer, '>', BTN_X - 14, btnY + 8, SDL_Color{255, 220, 70, 255}, 2);
            }
        }

        std::string optText = OPTIONS[i];
        int optW = BitmapFont::getTextWidth(optText, 2);
        int optX = BTN_X + (BTN_W - optW) / 2;
        SDL_Color textCol = selected
            ? (hasPanel ? SDL_Color{60, 36, 22, 255} : SDL_Color{255, 240, 120, 255})
            : (hasPanel ? SDL_Color{100, 75, 55, 255} : SDL_Color{200, 180, 150, 255});

        BitmapFont::drawText(renderer, optText, optX, btnY + 8, textCol, 2);

        btnY += BTN_SPACING;
    }

    // 5. Controls hint
    std::string hint = "[ESC] RESUME";
    int hintW = BitmapFont::getTextWidth(hint, 2);
    int hintX = WIN_X + (WIN_W - hintW) / 2;
    SDL_Color hintCol = hasPanel ? SDL_Color{110, 85, 65, 255} : SDL_Color{180, 160, 130, 255};
    BitmapFont::drawText(renderer, hint, hintX, WIN_Y + WIN_H - 22, hintCol, 2);
}
