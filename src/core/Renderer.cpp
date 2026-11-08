#include "core/Renderer.h"
#include "core/AssetManager.h"
#include <iostream>

Renderer::~Renderer() {
    cleanUp();
}

bool Renderer::init(SDL_Window* window) {
    if (!window) {
        std::cerr << "Renderer::init failed: null window passed" << std::endl;
        return false;
    }

    // Use hardware acceleration and enable VSync if available
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    m_renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!m_renderer) {
        // Fallback to software renderer (useful for headless / dummy driver)
        m_renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    }

    if (!m_renderer) {
        std::cerr << "SDL_CreateRenderer error: " << SDL_GetError() << std::endl;
        return false;
    }

    // Set blend mode for transparent drawing (e.g. selection highlight outlines)
    SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
    return true;
}

void Renderer::cleanUp() {
    if (m_renderer) {
        SDL_DestroyRenderer(m_renderer);
        m_renderer = nullptr;
    }
}

void Renderer::clear(SDL_Color color) {
    if (!m_renderer) return;
    SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, color.a);
    SDL_RenderClear(m_renderer);
}

void Renderer::present() {
    if (!m_renderer) return;
    SDL_RenderPresent(m_renderer);
}

void Renderer::drawRect(int x, int y, int w, int h, SDL_Color color, bool fill) {
    if (!m_renderer) return;
    SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, color.a);
    SDL_Rect rect{x, y, w, h};
    if (fill) {
        SDL_RenderFillRect(m_renderer, &rect);
    } else {
        SDL_RenderDrawRect(m_renderer, &rect);
    }
}

void Renderer::drawOutline(int x, int y, int w, int h, SDL_Color color) {
    drawRect(x, y, w, h, color, false);
}

void Renderer::drawLine(int x1, int y1, int x2, int y2, SDL_Color color) {
    if (!m_renderer) return;
    SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, color.a);
    SDL_RenderDrawLine(m_renderer, x1, y1, x2, y2);
}

bool Renderer::drawSprite(const std::string& filename, int x, int y, int w, int h,
                          const SDL_Rect* source) {
    if (!m_assets || !m_renderer) return false;
    SDL_Texture* texture = m_assets->getTexture("assets/sprites/" + filename, m_renderer);
    if (!texture) return false;
    const SDL_Rect destination{x, y, w, h};
    return SDL_RenderCopy(m_renderer, texture, source, &destination) == 0;
}

bool Renderer::drawNineSlice(const std::string& filename, int x, int y, int w, int h,
                             int left, int top, int right, int bottom,
                             const SDL_Rect* sourceSubRect) {
    if (!m_assets || !m_renderer) return false;
    SDL_Texture* texture = m_assets->getTexture("assets/sprites/" + filename, m_renderer);
    if (!texture) return false;

    int tw = 0, th = 0;
    if (SDL_QueryTexture(texture, nullptr, nullptr, &tw, &th) != 0) return false;

    int sx = 0, sy = 0, sw = tw, sh = th;
    if (sourceSubRect) {
        sx = sourceSubRect->x;
        sy = sourceSubRect->y;
        sw = sourceSubRect->w;
        sh = sourceSubRect->h;
    }

    if (sw < left + right || sh < top + bottom || w <= 0 || h <= 0) return false;

    int src_w[3] = {left, sw - left - right, right};
    int src_h[3] = {top, sh - top - bottom, bottom};
    int src_x[3] = {sx, sx + left, sx + sw - right};
    int src_y[3] = {sy, sy + top, sy + sh - bottom};

    int dst_w_left = left;
    int dst_w_right = right;
    int dst_w_center = w - left - right;
    if (dst_w_center < 0) {
        dst_w_left = (left + right > 0) ? (w * left / (left + right)) : 0;
        dst_w_right = w - dst_w_left;
        dst_w_center = 0;
    }

    int dst_h_top = top;
    int dst_h_bottom = bottom;
    int dst_h_center = h - top - bottom;
    if (dst_h_center < 0) {
        dst_h_top = (top + bottom > 0) ? (h * top / (top + bottom)) : 0;
        dst_h_bottom = h - dst_h_top;
        dst_h_center = 0;
    }

    int dst_w[3] = {dst_w_left, dst_w_center, dst_w_right};
    int dst_h[3] = {dst_h_top, dst_h_center, dst_h_bottom};
    int dst_x[3] = {x, x + dst_w_left, x + w - dst_w_right};
    int dst_y[3] = {y, y + dst_h_top, y + h - dst_h_bottom};

    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            if (src_w[c] > 0 && src_h[r] > 0 && dst_w[c] > 0 && dst_h[r] > 0) {
                const SDL_Rect sRect{src_x[c], src_y[r], src_w[c], src_h[r]};
                const SDL_Rect dRect{dst_x[c], dst_y[r], dst_w[c], dst_h[r]};
                SDL_RenderCopy(m_renderer, texture, &sRect, &dRect);
            }
        }
    }
    return true;
}

bool Renderer::drawDialogue(int x, int y, int w, int h, bool withTail) {
    const SDL_Rect bodySource{0, 0, 96, 42};
    if (!drawNineSlice("ui_dialogue.png", x, y, w, h, 6, 5, 6, 5, &bodySource)) {
        return false;
    }
    if (withTail) {
        const SDL_Rect tailSource{40, 37, 16, 11};
        drawSprite("ui_dialogue.png", x + (w - 16) / 2, y + h - 5, 16, 11, &tailSource);
    }
    return true;
}

