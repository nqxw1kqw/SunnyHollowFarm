#include "core/Input.h"

void Input::pollEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            m_quitRequested = true;
        } else if (event.type == SDL_KEYDOWN) {
            const SDL_Scancode scancode = event.key.keysym.scancode;
            if (scancode >= 0 && scancode < SDL_NUM_SCANCODES) {
                if (event.key.repeat == 0 && !m_keysDown[scancode]) {
                    m_keysPressed[scancode] = true;
                }
                m_keysDown[scancode] = true;
            }
        } else if (event.type == SDL_KEYUP) {
            const SDL_Scancode scancode = event.key.keysym.scancode;
            if (scancode >= 0 && scancode < SDL_NUM_SCANCODES) {
                m_keysDown[scancode] = false;
            }
        } else if (event.type == SDL_MOUSEBUTTONDOWN) {
            Uint8 btn = event.button.button;
            if (btn < m_mouseDown.size()) {
                if (!m_mouseDown[btn]) {
                    m_mousePressed[btn] = true;
                }
                m_mouseDown[btn] = true;
            }
            m_mouseX = event.button.x;
            m_mouseY = event.button.y;
        } else if (event.type == SDL_MOUSEBUTTONUP) {
            Uint8 btn = event.button.button;
            if (btn < m_mouseDown.size()) {
                m_mouseDown[btn] = false;
            }
            m_mouseX = event.button.x;
            m_mouseY = event.button.y;
        } else if (event.type == SDL_MOUSEMOTION) {
            m_mouseX = event.motion.x;
            m_mouseY = event.motion.y;
        }
    }
}

void Input::clearFramePressed() {
    m_keysPressed.fill(false);
    m_mousePressed.fill(false);
}

bool Input::isKeyDown(SDL_Scancode key) const {
    if (key < 0 || key >= SDL_NUM_SCANCODES) {
        return false;
    }
    return m_keysDown[key];
}

bool Input::isKeyPressed(SDL_Scancode key) const {
    if (key < 0 || key >= SDL_NUM_SCANCODES) {
        return false;
    }
    return m_keysPressed[key];
}

bool Input::isMouseButtonDown(Uint8 button) const {
    if (button >= m_mouseDown.size()) return false;
    return m_mouseDown[button];
}

bool Input::isMouseButtonPressed(Uint8 button) const {
    if (button >= m_mousePressed.size()) return false;
    return m_mousePressed[button];
}
