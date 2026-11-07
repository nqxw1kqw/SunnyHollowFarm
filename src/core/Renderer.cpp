#include "core/Renderer.h"
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
