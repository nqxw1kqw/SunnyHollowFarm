#pragma once

#include <SDL.h>
#include <SDL_mixer.h>
#include <unordered_map>
#include <string>

enum class SFX {
    Till,
    Water,
    Plant,
    Harvest,
    CookSuccess,
    CookFail,
    Buy,
    Sell,
    Sleep,
    UIOpen,
    UIConfirm,
    Door
};

// Audio subsystem managing background music loops, sound effects, and mute state.
// Designed to be fully optional: if no audio device exists, runs silently without crashing.
class AudioSystem {
public:
    AudioSystem() = default;
    ~AudioSystem();

    // Initializes SDL_mixer and loads audio clips from assets/audio/
    bool init();

    // Releases audio resources and closes mixer
    void cleanUp();

    // Plays the looping background music track
    void playMusic(int loops = -1);

    // Stops background music playback
    void stopMusic();

    // Triggers a one-shot sound effect
    void playSFX(SFX type);

    // Mute controls
    bool isMuted() const { return m_muted; }
    void setMuted(bool muted);
    bool toggleMute();

    // Device availability
    bool isAvailable() const { return m_available; }
    void setAvailable(bool available) { m_available = available; }

private:
    void loadAssets();

    bool m_available = false;
    bool m_muted = false;
    Mix_Music* m_music = nullptr;
    std::unordered_map<SFX, Mix_Chunk*> m_sfx;
};
