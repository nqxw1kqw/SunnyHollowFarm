#include "entities/NPC.h"
#include "world/Map.h"

NPC::NPC(int tileX, int tileY, NPCType type, const std::string& name)
    : m_tileX(tileX), m_tileY(tileY), m_type(type), m_name(name) {}

void NPC::render(Renderer& renderer) const {
    const int px = m_tileX * Map::TILE_SIZE;
    const int py = m_tileY * Map::TILE_SIZE;

    // Soft entity drop shadow
    renderer.drawRect(px + 6, py + 26, 20, 4, SDL_Color{20, 20, 20, 100});

    if (m_type == NPCType::Seller) {
        // Seller: Green merchant apron, shopkeeper cap, seed pouch

        // Legs/Pants
        renderer.drawRect(px + 10, py + 22, 5, 7, SDL_Color{55, 60, 75, 255});
        renderer.drawRect(px + 17, py + 22, 5, 7, SDL_Color{55, 60, 75, 255});

        // Torso / Green Apron
        renderer.drawRect(px + 8, py + 12, 16, 12, SDL_Color{45, 135, 75, 255});
        renderer.drawRect(px + 10, py + 10, 12, 3, SDL_Color{235, 230, 215, 255}); // cream undershirt
        renderer.drawRect(px + 11, py + 15, 10, 8, SDL_Color{35, 115, 60, 255});   // apron pocket
        // Yellow coin emblem on apron
        renderer.drawRect(px + 15, py + 17, 3, 3, SDL_Color{245, 215, 50, 255});

        // Face & Hair
        renderer.drawRect(px + 10, py + 4, 12, 8, SDL_Color{255, 215, 175, 255});
        // Dark brown hair
        renderer.drawRect(px + 9, py + 2, 14, 4, SDL_Color{90, 50, 25, 255});
        // Green merchant cap visor
        renderer.drawRect(px + 7, py + 3, 18, 3, SDL_Color{35, 115, 60, 255});
        // Friendly eyes
        renderer.drawRect(px + 12, py + 7, 2, 2, SDL_Color{30, 30, 40, 255});
        renderer.drawRect(px + 18, py + 7, 2, 2, SDL_Color{30, 30, 40, 255});
        // Cheerful smile
        renderer.drawRect(px + 14, py + 10, 4, 1, SDL_Color{180, 70, 70, 255});
    } else {
        // Buyer: Straw hat, rustic brown vest, blue jeans, ledger notebook

        // Blue jeans
        renderer.drawRect(px + 10, py + 22, 5, 7, SDL_Color{50, 95, 160, 255});
        renderer.drawRect(px + 17, py + 22, 5, 7, SDL_Color{50, 95, 160, 255});

        // Rustic vest over cream shirt
        renderer.drawRect(px + 8, py + 12, 16, 12, SDL_Color{155, 95, 45, 255});
        renderer.drawRect(px + 12, py + 12, 8, 12, SDL_Color{240, 235, 220, 255}); // cream shirt center
        renderer.drawRect(px + 10, py + 11, 12, 2, SDL_Color{120, 70, 30, 255});   // collar

        // Ledger notebook in hand (left side)
        renderer.drawRect(px + 5, py + 16, 5, 7, SDL_Color{220, 215, 200, 255});
        renderer.drawRect(px + 4, py + 16, 2, 7, SDL_Color{140, 40, 35, 255}); // red book spine

        // Face & Straw hat
        renderer.drawRect(px + 10, py + 5, 12, 8, SDL_Color{255, 215, 175, 255});
        // Large wide-brimmed golden straw hat
        renderer.drawRect(px + 5, py + 3, 22, 4, SDL_Color{230, 195, 70, 255});
        renderer.drawRect(px + 9, py, 14, 4, SDL_Color{245, 210, 85, 255});
        renderer.drawRect(px + 9, py + 3, 14, 2, SDL_Color{160, 45, 40, 255}); // red hat ribbon
        // Eyes
        renderer.drawRect(px + 12, py + 8, 2, 2, SDL_Color{30, 30, 40, 255});
        renderer.drawRect(px + 18, py + 8, 2, 2, SDL_Color{30, 30, 40, 255});
        // Mustache
        renderer.drawRect(px + 13, py + 10, 6, 2, SDL_Color{100, 65, 35, 255});
    }
}
