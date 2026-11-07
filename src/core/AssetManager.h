#pragma once

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <string>
#include <unordered_map>

// Manages texture and font asset loading and caching.
class AssetManager {
public:
    AssetManager() = default;
    ~AssetManager();

    bool init();
    void cleanUp();

    // Loads a texture or retrieves a cached copy. Returns nullptr if the asset is missing,
    // signaling to the renderer that placeholder graphics should be used.
    SDL_Texture* getTexture(const std::string& path, SDL_Renderer* renderer);

private:
    std::unordered_map<std::string, SDL_Texture*> m_textures;
    bool m_initialized = false;
};
