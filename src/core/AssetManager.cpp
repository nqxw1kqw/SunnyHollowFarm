#include "core/AssetManager.h"
#include <iostream>

AssetManager::~AssetManager() {
    cleanUp();
}

bool AssetManager::init() {
    if (m_initialized) {
        return true;
    }

    // Initialize SDL_image with PNG support
    int imgFlags = IMG_INIT_PNG;
    if (!(IMG_Init(imgFlags) & imgFlags)) {
        std::cerr << "IMG_Init warning: " << IMG_GetError() << " (will fallback to placeholders)" << std::endl;
    }

    // Initialize SDL_ttf for text rendering
    if (TTF_Init() == -1) {
        std::cerr << "TTF_Init warning: " << TTF_GetError() << " (will fallback to placeholders)" << std::endl;
    }

    m_initialized = true;
    return true;
}

void AssetManager::cleanUp() {
    for (auto& pair : m_textures) {
        if (pair.second) {
            SDL_DestroyTexture(pair.second);
        }
    }
    m_textures.clear();

    if (m_initialized) {
        TTF_Quit();
        IMG_Quit();
        m_initialized = false;
    }
}

SDL_Texture* AssetManager::getTexture(const std::string& path, SDL_Renderer* renderer) {
    if (!renderer) {
        return nullptr;
    }

    auto it = m_textures.find(path);
    if (it != m_textures.end()) {
        return it->second;
    }

    // Try loading texture from disk
    SDL_Surface* surface = IMG_Load(path.c_str());
    if (!surface) {
        // Expected when assets are not yet present; return nullptr to use placeholder
        return nullptr;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    if (texture) {
        m_textures[path] = texture;
    }
    return texture;
}
