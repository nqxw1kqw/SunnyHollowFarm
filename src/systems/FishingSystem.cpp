#include "systems/FishingSystem.h"
#include "ui/BitmapFont.h"
#include <cmath>
#include <iostream>

FishingSystem::FishingSystem()
    : m_rng(42) // Deterministic default seed
{
}

void FishingSystem::cancel() {
    m_state = FishingState::None;
    m_waterTileX = -1;
    m_waterTileY = -1;
    m_stateTimer = 0.0f;
    m_injectedWaitDuration = -1.0f;
}

bool FishingSystem::startFishing(int waterTileX, int waterTileY, Player& player, HUD* hud, AudioSystem* audio) {
    if (player.getEnergy() < Config::ENERGY_COST_FISH) {
        if (hud) {
            hud->showFeedback("NOT ENOUGH ENERGY!");
        }
        std::cout << "[Fishing] Action refused: Not enough energy to fish! (Need "
                  << Config::ENERGY_COST_FISH << ", have " << player.getEnergy() << ")" << std::endl;
        return false;
    }

    player.consumeEnergy(Config::ENERGY_COST_FISH);
    if (audio) {
        audio->playSFX(SFX::Water);
    }

    m_state = FishingState::Waiting;
    m_waterTileX = waterTileX;
    m_waterTileY = waterTileY;
    m_stateTimer = 0.0f;

    if (m_injectedWaitDuration > 0.0f) {
        m_waitDuration = m_injectedWaitDuration;
        m_injectedWaitDuration = -1.0f;
    } else {
        std::uniform_real_distribution<float> dist(2.0f, 5.0f);
        m_waitDuration = dist(m_rng);
    }

    if (hud) {
        hud->showFeedback("CAST LINE... WAITING FOR A BITE...", 1.5f);
    }
    std::cout << "[Fishing] Cast line at (" << waterTileX << ", " << waterTileY
              << ") [Wait: " << m_waitDuration << "s] [Energy: "
              << player.getEnergy() << "/" << player.getMaxEnergy() << "]" << std::endl;
    return true;
}

bool FishingSystem::handleActionInput(Inventory& inventory, HUD* hud, AudioSystem* audio) {
    if (m_state == FishingState::Waiting) {
        // Player pressed E too early: scares fish, cancels session with no energy refund
        m_state = FishingState::None;
        if (hud) {
            hud->showFeedback("TOO EARLY! FISH SCARED AWAY!");
        }
        std::cout << "[Fishing] Pressed too early! The fish got scared away (no cost refund)." << std::endl;
        return true;
    }

    if (m_state == FishingState::Bite) {
        // Pressed inside the ~800 ms bite window
        if (!inventory.hasSpaceFor(ItemType::Fish, 1)) {
            // Respect inventory capacity rule: full bag = fish escapes with a banner
            m_state = FishingState::None;
            if (hud) {
                hud->showFeedback("INVENTORY FULL! FISH ESCAPED!");
            }
            std::cout << "[Fishing] Inventory is full! Fish escaped." << std::endl;
            return true;
        }

        // Catch 1 Fish
        inventory.addItem(ItemType::Fish, 1);
        m_state = FishingState::None;
        if (audio) {
            audio->playSFX(SFX::Harvest);
        }
        if (hud) {
            hud->showFeedback("CAUGHT 1 FISH!");
        }
        std::cout << "[Fishing] Caught 1x Fish! [Total Fish in inventory: "
                  << inventory.getItemCount(ItemType::Fish) << "]" << std::endl;
        return true;
    }

    return false;
}

void FishingSystem::update(float dt, HUD* hud) {
    if (m_state == FishingState::Waiting) {
        m_stateTimer += dt;
        if (m_stateTimer >= m_waitDuration) {
            m_state = FishingState::Bite;
            m_stateTimer = 0.0f;
            std::cout << "[Fishing] Bite! Window open for ~800ms at ("
                      << m_waterTileX << ", " << m_waterTileY << ")." << std::endl;
        }
    } else if (m_state == FishingState::Bite) {
        m_stateTimer += dt;
        if (m_stateTimer >= m_biteWindowDuration) {
            // Bite window expired: fish escapes
            m_state = FishingState::None;
            if (hud) {
                hud->showFeedback("THE FISH ESCAPED!");
            }
            std::cout << "[Fishing] Bite window expired! The fish escaped." << std::endl;
        }
    }
}

void FishingSystem::render(Renderer& renderer) const {
    if (m_state == FishingState::None || m_waterTileX < 0 || m_waterTileY < 0) {
        return;
    }

    const int px = m_waterTileX * Map::TILE_SIZE;
    const int py = m_waterTileY * Map::TILE_SIZE;

    if (m_state == FishingState::Waiting) {
        // Subtle animated bobbing float on water
        float bob = std::sin(m_stateTimer * 6.0f) * 2.0f;
        int bobY = py + 14 + static_cast<int>(bob);

        // Water ripple rings
        renderer.drawOutline(px + 8, bobY + 1, 16, 5, SDL_Color{160, 220, 245, 140});

        // Red and white fishing bobber
        renderer.drawRect(px + 13, bobY - 2, 6, 3, SDL_Color{225, 45, 35, 255});
        renderer.drawRect(px + 13, bobY + 1, 6, 3, SDL_Color{245, 245, 245, 255});
        renderer.drawOutline(px + 12, bobY - 3, 8, 8, SDL_Color{40, 25, 20, 200});
        renderer.drawRect(px + 15, bobY - 4, 2, 2, SDL_Color{30, 20, 15, 255});
    } else if (m_state == FishingState::Bite) {
        // Water splash ripples
        renderer.drawOutline(px + 4, py + 12, 24, 10, SDL_Color{220, 245, 255, 220});
        renderer.drawOutline(px + 7, py + 14, 18, 6, SDL_Color{180, 230, 255, 180});

        // 1. fx_sparkle on the water tile
        if (!renderer.drawSprite("fx_sparkle.png", px, py, 32, 32)) {
            // Graceful sparkle placeholder fallback
            renderer.drawRect(px + 12, py + 8, 8, 8, SDL_Color{255, 240, 100, 255});
            renderer.drawRect(px + 8, py + 12, 16, 2, SDL_Color{255, 255, 255, 255});
            renderer.drawRect(px + 15, py + 5, 2, 16, SDL_Color{255, 255, 255, 255});
        }

        // 2. "!" prompt above the water tile
        const int promptX = px + 9;
        const int promptY = py - 18;
        renderer.drawRect(promptX, promptY, 14, 18, SDL_Color{255, 255, 255, 245});
        renderer.drawOutline(promptX, promptY, 14, 18, SDL_Color{45, 30, 20, 255});
        renderer.drawRect(promptX + 4, promptY + 18, 6, 2, SDL_Color{255, 255, 255, 245});
        BitmapFont::drawText(renderer, "!", promptX + 4, promptY + 2, SDL_Color{220, 35, 25, 255}, 2);
    }
}
