#include "entities/NPC.h"
#include "world/Map.h"
#include <cmath>

NPC::NPC(int tileX, int tileY, NPCType type, const std::string& name)
    : m_tileX(tileX), m_tileY(tileY), m_type(type), m_name(name) {}

void NPC::update(float dt) {
    m_animTimer += dt;
}

int NPC::getIdleBob() const {
    // Subtle idle breathing bob (±1 px sine wave)
    float phase = (m_type == NPCType::Buyer) ? 1.5f : 0.0f;
    return static_cast<int>(std::round(std::sin(m_animTimer * 2.5f + phase)));
}

void NPC::render(Renderer& renderer) const {
    const int px = m_tileX * Map::TILE_SIZE;
    const int basePy = m_tileY * Map::TILE_SIZE;
    const int bob = getIdleBob();

    // Scale up NPC from 32x32 to 48x48 (1.5x), breaking free of the 1-block grid constraint
    constexpr int NPC_WIDTH = 48;
    constexpr int NPC_HEIGHT = 48;
    const int renderX = px + (Map::TILE_SIZE - NPC_WIDTH) / 2;
    const int renderY = (basePy + Map::TILE_SIZE) - NPC_HEIGHT + bob;

    // Ground entity drop shadow anchored to tile ground
    renderer.drawRect(px + 1, basePy + 24, 30, 6, SDL_Color{20, 20, 20, 95});
    renderer.drawRect(px + 4, basePy + 25, 24, 4, SDL_Color{10, 10, 10, 65});

    if (renderer.drawSprite(m_type == NPCType::Seller ? "npc_seller.png" : "npc_buyer.png",
                            renderX, renderY, NPC_WIDTH, NPC_HEIGHT)) return;

    if (m_type == NPCType::Seller) {
        // Seller (Bram): Green merchant apron, shopkeeper cap, seed pouch (scaled 48x48)

        // Legs/Pants
        renderer.drawRect(renderX + 15, renderY + 33, 8, 12, SDL_Color{55, 60, 75, 255});
        renderer.drawRect(renderX + 25, renderY + 33, 8, 12, SDL_Color{55, 60, 75, 255});

        // Torso / Green Apron
        renderer.drawRect(renderX + 12, renderY + 18, 24, 18, SDL_Color{45, 135, 75, 255});
        renderer.drawRect(renderX + 15, renderY + 15, 18, 5, SDL_Color{235, 230, 215, 255}); // cream undershirt
        renderer.drawRect(renderX + 16, renderY + 22, 16, 12, SDL_Color{35, 115, 60, 255});  // apron pocket
        // Yellow coin emblem on apron
        renderer.drawRect(renderX + 22, renderY + 25, 5, 5, SDL_Color{245, 215, 50, 255});

        // Face & Hair
        renderer.drawRect(renderX + 15, renderY + 6, 18, 12, SDL_Color{255, 215, 175, 255});
        // Dark brown hair
        renderer.drawRect(renderX + 13, renderY + 3, 22, 6, SDL_Color{90, 50, 25, 255});
        // Green merchant cap visor
        renderer.drawRect(renderX + 10, renderY + 5, 28, 4, SDL_Color{35, 115, 60, 255});
        // Friendly eyes
        renderer.drawRect(renderX + 18, renderY + 10, 3, 3, SDL_Color{30, 30, 40, 255});
        renderer.drawRect(renderX + 27, renderY + 10, 3, 3, SDL_Color{30, 30, 40, 255});
        // Cheerful smile
        renderer.drawRect(renderX + 21, renderY + 15, 6, 2, SDL_Color{180, 70, 70, 255});
    } else {
        // Buyer (Silas): Straw hat, rustic brown vest, blue jeans, ledger notebook (scaled 48x48)

        // Blue jeans
        renderer.drawRect(renderX + 15, renderY + 33, 8, 12, SDL_Color{50, 95, 160, 255});
        renderer.drawRect(renderX + 25, renderY + 33, 8, 12, SDL_Color{50, 95, 160, 255});

        // Rustic vest over cream shirt
        renderer.drawRect(renderX + 12, renderY + 18, 24, 18, SDL_Color{155, 95, 45, 255});
        renderer.drawRect(renderX + 18, renderY + 18, 12, 18, SDL_Color{240, 235, 220, 255}); // cream shirt center
        renderer.drawRect(renderX + 15, renderY + 16, 18, 3, SDL_Color{120, 70, 30, 255});   // collar

        // Ledger notebook in hand (left side)
        renderer.drawRect(renderX + 7, renderY + 24, 8, 11, SDL_Color{220, 215, 200, 255});
        renderer.drawRect(renderX + 5, renderY + 24, 3, 11, SDL_Color{140, 40, 35, 255}); // red book spine

        // Face & Straw hat
        renderer.drawRect(renderX + 15, renderY + 7, 18, 12, SDL_Color{255, 215, 175, 255});
        // Large wide-brimmed golden straw hat
        renderer.drawRect(renderX + 7, renderY + 4, 34, 6, SDL_Color{230, 195, 70, 255});
        renderer.drawRect(renderX + 13, renderY, 22, 6, SDL_Color{245, 210, 85, 255});
        renderer.drawRect(renderX + 13, renderY + 4, 22, 3, SDL_Color{160, 45, 40, 255}); // red hat ribbon
        // Eyes
        renderer.drawRect(renderX + 18, renderY + 12, 3, 3, SDL_Color{30, 30, 40, 255});
        renderer.drawRect(renderX + 27, renderY + 12, 3, 3, SDL_Color{30, 30, 40, 255});
        // Mustache
        renderer.drawRect(renderX + 20, renderY + 15, 9, 3, SDL_Color{100, 65, 35, 255});
    }
}
