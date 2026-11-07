#pragma once

#include <SDL.h>

// Tracks frame elapsed time and drives a fixed-timestep update accumulator.
class Time {
public:
    // Simulation runs at a steady 60 updates per second regardless of frame rate.
    static constexpr float FIXED_DT = 1.0f / 60.0f;

    // Clamp overly long frames (e.g., when dragging the window) so the loop does not spiral.
    static constexpr float MAX_FRAME_DELTA = 0.25f;

    Time();

    // Call once at the start of each frame to measure elapsed real time and add to accumulator.
    float beginFrame();

    // Returns true and consumes FIXED_DT if another fixed simulation step is due.
    bool consumeFixedStep();

private:
    Uint64 m_lastCounter = 0;
    Uint64 m_frequency = 1;
    float m_accumulator = 0.0f;
};
