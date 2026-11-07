#pragma once

#include <SDL.h>

// Thin wrapper around SDL_Renderer providing primitives for placeholder graphics.
class Renderer {
public:
    Renderer() = default;
    ~Renderer();

    // Initializes the SDL hardware/accelerated renderer for the given window.
    bool init(SDL_Window* window);
    void cleanUp();

    // Clears the entire render target using the specified color.
    void clear(SDL_Color color);

    // Displays the rendered frame onto the screen.
    void present();

    // Draws a filled or outlined rectangle.
    void drawRect(int x, int y, int w, int h, SDL_Color color, bool fill = true);

    // Draws an outline rectangle.
    void drawOutline(int x, int y, int w, int h, SDL_Color color);

    // Draws a line between two points.
    void drawLine(int x1, int y1, int x2, int y2, SDL_Color color);

    SDL_Renderer* getSDLRenderer() const { return m_renderer; }

private:
    SDL_Renderer* m_renderer = nullptr;
};
