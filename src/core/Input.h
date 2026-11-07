#pragma once

#include <SDL.h>
#include <array>

// Polls SDL events and exposes held keys as well as single-frame key presses.
class Input {
public:
    Input() = default;

    // Polls all pending SDL events for this frame.
    void pollEvents();

    // Clears per-frame pressed flags at the end of a fixed update / frame.
    void clearFramePressed();

    bool quitRequested() const { return m_quitRequested; }

    // True every frame while the key is held down.
    bool isKeyDown(SDL_Scancode key) const;

    // True only once when the key transitions from up to down (ignores OS key repeat).
    bool isKeyPressed(SDL_Scancode key) const;

private:
    bool m_quitRequested = false;
    std::array<bool, SDL_NUM_SCANCODES> m_keysDown{};
    std::array<bool, SDL_NUM_SCANCODES> m_keysPressed{};
};
