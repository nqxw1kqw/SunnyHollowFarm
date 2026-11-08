#pragma once

#include <SDL.h>
#include <string>

class AssetManager;

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

    void setAssets(AssetManager* assets) { m_assets = assets; }
    // Returns false when art is missing so callers can keep their placeholder.
    bool drawSprite(const std::string& filename, int x, int y, int w, int h,
                    const SDL_Rect* source = nullptr);
    bool drawNineSlice(const std::string& filename, int x, int y, int w, int h,
                       int left, int top, int right, int bottom,
                       const SDL_Rect* sourceSubRect = nullptr);
    bool drawDialogue(int x, int y, int w, int h, bool withTail = false);

    SDL_Renderer* getSDLRenderer() const { return m_renderer; }

    // World-space coordinate offset
    void setOffset(int ox, int oy) { m_offsetX = ox; m_offsetY = oy; }
    int getOffsetX() const { return m_offsetX; }
    int getOffsetY() const { return m_offsetY; }

private:
    SDL_Renderer* m_renderer = nullptr;
    AssetManager* m_assets = nullptr;
    int m_offsetX = 0;
    int m_offsetY = 0;
};
