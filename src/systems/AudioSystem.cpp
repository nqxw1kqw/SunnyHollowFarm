#include "systems/AudioSystem.h"
#include <iostream>

AudioSystem::~AudioSystem() {
    cleanUp();
}

bool AudioSystem::init() {
    if (SDL_WasInit(SDL_INIT_AUDIO) == 0) {
        if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
            std::cout << "[AudioSystem] SDL audio subsystem unavailable (" << SDL_GetError() << "). Running silently." << std::endl;
            m_available = false;
            return true;
        }
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024) != 0) {
        std::cout << "[AudioSystem] Failed to open audio device (" << Mix_GetError() << "). Running silently." << std::endl;
        m_available = false;
        return true;
    }

    m_available = true;
    Mix_AllocateChannels(16);
    Mix_VolumeMusic(MIX_MAX_VOLUME / 2);
    Mix_Volume(-1, MIX_MAX_VOLUME);

    loadAssets();

    std::cout << "[AudioSystem] SDL2_mixer initialized successfully (44.1kHz, 16-bit stereo)." << std::endl;

    if (!m_muted) {
        playMusic();
    }
    return true;
}

void AudioSystem::loadAssets() {
    if (!m_available) return;

    // Load background music loop
    m_music = Mix_LoadMUS("assets/audio/music.wav");
    if (!m_music) {
        std::cout << "[AudioSystem] Notice: Could not load music.wav: " << Mix_GetError() << std::endl;
    }

    // Load sound effects
    struct SFXMapping {
        SFX type;
        const char* path;
    };

    const SFXMapping mappings[] = {
        { SFX::Till,        "assets/audio/till.wav" },
        { SFX::Water,       "assets/audio/water.wav" },
        { SFX::Plant,       "assets/audio/plant.wav" },
        { SFX::Harvest,     "assets/audio/harvest.wav" },
        { SFX::CookSuccess, "assets/audio/cook_success.wav" },
        { SFX::CookFail,    "assets/audio/cook_fail.wav" },
        { SFX::Buy,         "assets/audio/buy.wav" },
        { SFX::Sell,        "assets/audio/sell.wav" },
        { SFX::Sleep,       "assets/audio/sleep.wav" },
        { SFX::UIOpen,      "assets/audio/ui_open.wav" },
        { SFX::UIConfirm,   "assets/audio/ui_confirm.wav" },
        { SFX::Door,        "assets/audio/door.wav" },
    };

    for (const auto& item : mappings) {
        Mix_Chunk* chunk = Mix_LoadWAV(item.path);
        if (chunk) {
            m_sfx[item.type] = chunk;
        } else {
            std::cout << "[AudioSystem] Notice: Could not load " << item.path << ": " << Mix_GetError() << std::endl;
        }
    }
}

void AudioSystem::cleanUp() {
    if (m_available) {
        stopMusic();
        for (auto& pair : m_sfx) {
            if (pair.second) {
                Mix_FreeChunk(pair.second);
                pair.second = nullptr;
            }
        }
        m_sfx.clear();

        if (m_music) {
            Mix_FreeMusic(m_music);
            m_music = nullptr;
        }

        Mix_CloseAudio();
        m_available = false;
        std::cout << "[AudioSystem] AudioSystem cleaned up cleanly." << std::endl;
    }
}

void AudioSystem::playMusic(int loops) {
    if (!m_available || m_muted || !m_music) return;
    Mix_PlayMusic(m_music, loops);
}

void AudioSystem::stopMusic() {
    if (!m_available) return;
    Mix_HaltMusic();
}

void AudioSystem::playSFX(SFX type) {
    if (!m_available || m_muted) return;
    auto it = m_sfx.find(type);
    if (it != m_sfx.end() && it->second) {
        Mix_PlayChannel(-1, it->second, 0);
    }
}

void AudioSystem::setMuted(bool muted) {
    m_muted = muted;
    if (!m_available) return;

    if (m_muted) {
        Mix_VolumeMusic(0);
        Mix_Volume(-1, 0);
        Mix_PauseMusic();
    } else {
        Mix_VolumeMusic(MIX_MAX_VOLUME / 2);
        Mix_Volume(-1, MIX_MAX_VOLUME);
        if (Mix_PausedMusic()) {
            Mix_ResumeMusic();
        } else if (m_music && Mix_PlayingMusic() == 0) {
            Mix_PlayMusic(m_music, -1);
        }
    }
}

bool AudioSystem::toggleMute() {
    setMuted(!m_muted);
    return m_muted;
}
