#pragma once

#include "core/Renderer.h"
#include "core/Input.h"
#include <string>

class AudioSystem;

enum class TitleAction {
    None,
    Continue,
    NewGame,
    Quit
};

// Title screen UI with Continue (if save exists), New Game (with overwrite prompt), and Quit.
class TitleUI {
public:
    TitleUI() = default;

    void init(bool hasSave);
    void open(bool hasSave);
    void refreshSaveExists(bool hasSave);

    bool hasSave() const { return m_hasSave; }
    int getOptionCount() const { return m_hasSave ? 3 : 2; }
    int getSelectedIndex() const { return m_selectedIndex; }
    void setSelectedIndex(int idx) { m_selectedIndex = idx; }

    bool isConfirmingOverwrite() const { return m_confirmOverwrite; }
    void setConfirmingOverwrite(bool confirm) { m_confirmOverwrite = confirm; }

    // Handles input for menu navigation and overwrite confirmation
    TitleAction handleInput(const Input& input, AudioSystem* audio = nullptr);

    // Renders title banner, panel art, menu options, and overwrite dialog
    void render(Renderer& renderer) const;

private:
    bool m_hasSave = false;
    int m_selectedIndex = 0;
    bool m_confirmOverwrite = false;
};
