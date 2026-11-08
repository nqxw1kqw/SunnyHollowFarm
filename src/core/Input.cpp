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
        }
    }
}

void Input::clearFramePressed() {
    m_keysPressed.fill(false);
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
