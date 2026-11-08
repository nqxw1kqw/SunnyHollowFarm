#pragma once

#include "core/Renderer.h"
#include "core/Input.h"

class AudioSystem;
class StatsSystem;

// Overlay modal displaying lifetime player statistics and farm metrics.
class StatsUI {
public:
    StatsUI() = default;

    void open(bool fromPause = false);
    void close();
    void toggle();
    bool isOpen() const { return m_isOpen; }
    bool isFromPause() const { return m_fromPause; }

    // Handles input for navigating / closing the stats view. Returns true if closed.
    bool handleInput(const Input& input, AudioSystem* audio = nullptr);

    // Renders the statistics dialog overlay
    void render(Renderer& renderer, int day, int wallet, const StatsSystem& stats, int chickensOwned) const;

private:
    bool m_isOpen = false;
    bool m_fromPause = false;
};
