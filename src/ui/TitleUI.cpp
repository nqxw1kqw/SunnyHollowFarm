#include "ui/TitleUI.h"
#include "ui/BitmapFont.h"
#include "core/GameConfig.h"
#include "systems/AudioSystem.h"
#include <vector>

void TitleUI::init(bool hasSave) {
    m_hasSave = hasSave;
    m_selectedIndex = 0;
    m_confirmOverwrite = false;
}

void TitleUI::open(bool hasSave) {
    init(hasSave);
}

void TitleUI::refreshSaveExists(bool hasSave) {
    m_hasSave = hasSave;
    if (m_selectedIndex >= getOptionCount()) {
        m_selectedIndex = 0;
    }
}

TitleAction TitleUI::handleInput(const Input& input, AudioSystem* audio) {
    // If overwrite confirmation prompt is visible:
    if (m_confirmOverwrite) {
        if (input.isKeyPressed(SDL_SCANCODE_RETURN) || input.isKeyPressed(SDL_SCANCODE_SPACE)) {
            m_confirmOverwrite = false;
            if (audio) audio->playSFX(SFX::UIConfirm);
            return TitleAction::NewGame;
        }
        if (input.isKeyPressed(SDL_SCANCODE_ESCAPE)) {
            m_confirmOverwrite = false;
            if (audio) audio->playSFX(SFX::UIConfirm);
            return TitleAction::None;
        }
        return TitleAction::None;
    }

    const int count = getOptionCount();

    // Navigate with Up/Down or W/S
    if (input.isKeyPressed(SDL_SCANCODE_UP) || input.isKeyPressed(SDL_SCANCODE_W)) {
        m_selectedIndex = (m_selectedIndex - 1 + count) % count;
        if (audio) audio->playSFX(SFX::UIConfirm);
    }
    if (input.isKeyPressed(SDL_SCANCODE_DOWN) || input.isKeyPressed(SDL_SCANCODE_S)) {
        m_selectedIndex = (m_selectedIndex + 1) % count;
        if (audio) audio->playSFX(SFX::UIConfirm);
    }

    // Select with Enter, Space, or Return
    if (input.isKeyPressed(SDL_SCANCODE_RETURN) || input.isKeyPressed(SDL_SCANCODE_SPACE)) {
        if (m_hasSave) {
            if (m_selectedIndex == 0) {
                if (audio) audio->playSFX(SFX::UIConfirm);
                return TitleAction::Continue;
            } else if (m_selectedIndex == 1) {
                // New Game when save exists asks overwrite confirmation
                m_confirmOverwrite = true;
                if (audio) audio->playSFX(SFX::UIOpen);
                return TitleAction::None;
            } else if (m_selectedIndex == 2) {
                if (audio) audio->playSFX(SFX::UIConfirm);
                return TitleAction::Quit;
            }
        } else {
            if (m_selectedIndex == 0) {
                if (audio) audio->playSFX(SFX::UIConfirm);
                return TitleAction::NewGame;
            } else if (m_selectedIndex == 1) {
                if (audio) audio->playSFX(SFX::UIConfirm);
                return TitleAction::Quit;
            }
        }
    }

    return TitleAction::None;
}

void TitleUI::render(Renderer& renderer) const {
    // 1. Title menu panel art
    constexpr int WIN_W = 340;
    constexpr int WIN_H = 290;
    constexpr int WIN_X = (Config::SCREEN_WIDTH - WIN_W) / 2;
    constexpr int WIN_Y = (Config::SCREEN_HEIGHT - WIN_H) / 2;

    bool hasPanel = renderer.drawNineSlice("ui_panel.png", WIN_X, WIN_Y, WIN_W, WIN_H, 3, 3, 3, 3);
    if (!hasPanel) {
        renderer.drawRect(WIN_X, WIN_Y, WIN_W, WIN_H, SDL_Color{35, 27, 22, 252});
        renderer.drawOutline(WIN_X, WIN_Y, WIN_W, WIN_H, SDL_Color{195, 160, 105, 255});
        renderer.drawOutline(WIN_X + 2, WIN_Y + 2, WIN_W - 4, WIN_H - 4, SDL_Color{115, 85, 50, 255});
    }

    // 2. Game Title Name
    std::string title = "SUNNY HOLLOW FARM";
    int titleW = BitmapFont::getTextWidth(title, 2);
    int titleX = WIN_X + (WIN_W - titleW) / 2;
    SDL_Color titleCol = hasPanel ? SDL_Color{60, 36, 22, 255} : SDL_Color{255, 230, 130, 255};
    BitmapFont::drawText(renderer, title, titleX, WIN_Y + 22, titleCol, 2);

    // Decorative separator
    renderer.drawLine(WIN_X + 28, WIN_Y + 46, WIN_X + WIN_W - 28, WIN_Y + 46,
                      hasPanel ? SDL_Color{160, 130, 85, 180} : SDL_Color{115, 85, 50, 255});

    // 3. Menu Options
    std::vector<std::string> options;
    if (m_hasSave) {
        options = {"CONTINUE", "NEW GAME", "QUIT"};
    } else {
        options = {"NEW GAME", "QUIT"};
    }

    constexpr int BTN_W = 200;
    constexpr int BTN_H = 32;
    constexpr int BTN_X = WIN_X + (WIN_W - BTN_W) / 2;
    int btnY = WIN_Y + 64;
    const int count = static_cast<int>(options.size());
    int spacing = (count == 3) ? 44 : 52;

    for (int i = 0; i < count; ++i) {
        bool selected = (i == m_selectedIndex);
        const char* btnSprite = selected ? "ui_button_hover.png" : "ui_button.png";

        if (!renderer.drawNineSlice(btnSprite, BTN_X, btnY, BTN_W, BTN_H, 8, 6, 8, 6)) {
            SDL_Color bgCol = selected ? SDL_Color{75, 55, 35, 255} : SDL_Color{45, 35, 28, 255};
            renderer.drawRect(BTN_X, btnY, BTN_W, BTN_H, bgCol);
            renderer.drawOutline(BTN_X, btnY, BTN_W, BTN_H,
                                 selected ? SDL_Color{255, 220, 100, 255} : SDL_Color{140, 110, 80, 255});
        }

        if (selected) {
            if (!renderer.drawSprite("ui_cursor.png", BTN_X - 20, btnY + 5, 22, 22)) {
                BitmapFont::drawChar(renderer, '>', BTN_X - 16, btnY + 10, SDL_Color{255, 220, 70, 255}, 2);
            }
        }

        const std::string& optText = options[i];
        int optW = BitmapFont::getTextWidth(optText, 2);
        int optX = BTN_X + (BTN_W - optW) / 2;
        SDL_Color textCol = selected
            ? (hasPanel ? SDL_Color{60, 36, 22, 255} : SDL_Color{255, 240, 120, 255})
            : (hasPanel ? SDL_Color{100, 75, 55, 255} : SDL_Color{200, 180, 150, 255});

        BitmapFont::drawText(renderer, optText, optX, btnY + 10, textCol, 2);

        btnY += spacing;
    }

    // Controls hint
    std::string hint = "[W/S / ARROWS] SELECT   [ENTER] CHOOSE";
    int hintW = BitmapFont::getTextWidth(hint, 1);
    int hintX = WIN_X + (WIN_W - hintW) / 2;
    SDL_Color hintCol = hasPanel ? SDL_Color{110, 85, 65, 255} : SDL_Color{180, 160, 130, 255};
    BitmapFont::drawText(renderer, hint, hintX, WIN_Y + WIN_H - 22, hintCol, 1);

    // 4. Overwrite confirmation modal prompt
    if (m_confirmOverwrite) {
        // Dark backdrop over title
        renderer.drawRect(0, 0, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, SDL_Color{0, 0, 0, 175});

        constexpr int MODAL_W = 380;
        constexpr int MODAL_H = 110;
        constexpr int MODAL_X = (Config::SCREEN_WIDTH - MODAL_W) / 2;
        constexpr int MODAL_Y = (Config::SCREEN_HEIGHT - MODAL_H) / 2;

        bool hasModal = renderer.drawNineSlice("ui_panel.png", MODAL_X, MODAL_Y, MODAL_W, MODAL_H, 3, 3, 3, 3);
        if (!hasModal) {
            renderer.drawRect(MODAL_X, MODAL_Y, MODAL_W, MODAL_H, SDL_Color{35, 20, 18, 252});
            renderer.drawOutline(MODAL_X, MODAL_Y, MODAL_W, MODAL_H, SDL_Color{230, 100, 80, 255});
            renderer.drawOutline(MODAL_X + 2, MODAL_Y + 2, MODAL_W - 4, MODAL_H - 4, SDL_Color{150, 50, 40, 255});
        }

        std::string line1 = "OVERWRITE SAVE?";
        int l1W = BitmapFont::getTextWidth(line1, 2);
        int l1X = MODAL_X + (MODAL_W - l1W) / 2;
        BitmapFont::drawText(renderer, line1, l1X, MODAL_Y + 24,
                             hasModal ? SDL_Color{60, 36, 22, 255} : SDL_Color{255, 230, 130, 255}, 2);

        std::string line2 = "ENTER = YES, ESC = NO";
        int l2W = BitmapFont::getTextWidth(line2, 2);
        int l2X = MODAL_X + (MODAL_W - l2W) / 2;
        BitmapFont::drawText(renderer, line2, l2X, MODAL_Y + 60,
                             hasModal ? SDL_Color{120, 60, 20, 255} : SDL_Color{255, 200, 100, 255}, 2);
    }
}
