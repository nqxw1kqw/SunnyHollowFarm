#pragma once

#include "core/Renderer.h"
#include "core/Input.h"

class AudioSystem;

enum class PauseAction {
    None,
    Resume,
    SaveNow,
    ViewStats,
    QuitToTitle,
    QuitGame
};

// Pause menu overlay providing immediate save, stats viewing, and title quit.
class PauseUI {
public:
    static constexpr int OPTION_COUNT = 5;

    PauseUI() = default;

    void open();
    void close();
    bool isOpen() const { return m_isOpen; }

    int getSelectedIndex() const { return m_selectedIndex; }
    void setSelectedIndex(int idx) { m_selectedIndex = idx; }

    // Handles pause navigation (arrows, W/S, Enter, Esc)
    PauseAction handleInput(const Input& input, AudioSystem* audio = nullptr);

    // Renders the pause menu overlay
    void render(Renderer& renderer) const;

private:
    bool m_isOpen = false;
    int m_selectedIndex = 0;
};
