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

    // Mouse coordinates and button states
    int getMouseX() const { return m_mouseX; }
    int getMouseY() const { return m_mouseY; }
    bool isMouseButtonDown(Uint8 button = SDL_BUTTON_LEFT) const;
    bool isMouseButtonPressed(Uint8 button = SDL_BUTTON_LEFT) const;

    // Simulation / testing setters
    void setKeyDown(SDL_Scancode key, bool down) {
        if (static_cast<size_t>(key) < m_keysDown.size()) {
            m_keysDown[key] = down;
        }
    }
    void setKeyPressed(SDL_Scancode key, bool pressed) {
        if (static_cast<size_t>(key) < m_keysPressed.size()) {
            m_keysPressed[key] = pressed;
        }
    }
    void setMousePos(int x, int y) {
        m_mouseX = x;
        m_mouseY = y;
    }
    void setMouseButtonPressed(Uint8 button, bool pressed) {
        if (button < m_mousePressed.size()) {
            m_mousePressed[button] = pressed;
        }
    }

private:
    bool m_quitRequested = false;
    std::array<bool, SDL_NUM_SCANCODES> m_keysDown{};
    std::array<bool, SDL_NUM_SCANCODES> m_keysPressed{};
    int m_mouseX = 0;
    int m_mouseY = 0;
    std::array<bool, 8> m_mouseDown{};
    std::array<bool, 8> m_mousePressed{};
};
