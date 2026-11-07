#include "core/Time.h"

Time::Time() {
    m_frequency = SDL_GetPerformanceFrequency();
    if (m_frequency == 0) {
        m_frequency = 1;
    }
    m_lastCounter = SDL_GetPerformanceCounter();
}

float Time::beginFrame() {
    const Uint64 now = SDL_GetPerformanceCounter();
    float frameDelta = static_cast<float>(now - m_lastCounter) / static_cast<float>(m_frequency);
    m_lastCounter = now;

    if (frameDelta > MAX_FRAME_DELTA) {
        frameDelta = MAX_FRAME_DELTA;
    }

    m_accumulator += frameDelta;
    return frameDelta;
}

bool Time::consumeFixedStep() {
    if (m_accumulator >= FIXED_DT) {
        m_accumulator -= FIXED_DT;
        return true;
    }
    return false;
}
