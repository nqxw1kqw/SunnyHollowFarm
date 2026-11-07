#include "entities/Item.h"

const ItemDefinition& getItemDefinition(ItemType type) {
    static const ItemDefinition emptyDef{
        ItemType::Empty, "Empty", "An empty inventory slot.", 0, 0, false, false, 0
    };
    static const ItemDefinition tomatoSeedsDef{
        ItemType::TomatoSeeds, "Tomato Seeds", "Sow into tilled soil to grow juicy tomatoes.", 15, 8, true, false, 0
    };
    static const ItemDefinition carrotSeedsDef{
        ItemType::CarrotSeeds, "Carrot Seeds", "Sow into tilled soil to grow sweet carrots.", 20, 10, true, false, 0
    };
    static const ItemDefinition wheatSeedsDef{
        ItemType::WheatSeeds, "Wheat Seeds", "Sow into tilled soil to grow golden wheat.", 10, 5, true, false, 0
    };
    static const ItemDefinition tomatoDef{
        ItemType::Tomato, "Ripe Tomato", "A plump, freshly harvested red tomato.", 25, 20, false, true, 10
    };
    static const ItemDefinition carrotDef{
        ItemType::Carrot, "Crisp Carrot", "A crunchy orange root vegetable.", 30, 24, false, true, 12
    };
    static const ItemDefinition wheatDef{
        ItemType::Wheat, "Wheat Sheaf", "Golden grain ready for baking fresh bread.", 18, 14, false, false, 0
    };
    static const ItemDefinition saladDef{
        ItemType::Salad, "Garden Salad", "Fresh garden vegetables tossed together.", 50, 40, false, true, 35
    };
    static const ItemDefinition breadDef{
        ItemType::Bread, "Fresh Bread", "A warm, crusty loaf baked from milled wheat.", 60, 50, false, true, 45
    };

    switch (type) {
        case ItemType::TomatoSeeds: return tomatoSeedsDef;
        case ItemType::CarrotSeeds: return carrotSeedsDef;
        case ItemType::WheatSeeds:  return wheatSeedsDef;
        case ItemType::Tomato:       return tomatoDef;
        case ItemType::Carrot:       return carrotDef;
        case ItemType::Wheat:        return wheatDef;
        case ItemType::Salad:        return saladDef;
        case ItemType::Bread:        return breadDef;
        default:                     return emptyDef;
    }
}

void drawItemPlaceholder(Renderer& renderer, ItemType type, int x, int y, int size) {
    if (type == ItemType::Empty) return;

    // Scale offset based on standard 32x32 size
    float scale = size / 32.0f;
    auto S = [scale](int val) { return static_cast<int>(val * scale); };

    switch (type) {
        case ItemType::TomatoSeeds:
        case ItemType::CarrotSeeds:
        case ItemType::WheatSeeds: {
            // Seed packet pouch background
            renderer.drawRect(x + S(6), y + S(5), S(20), S(22), SDL_Color{225, 205, 145, 255});
            renderer.drawOutline(x + S(6), y + S(5), S(20), S(22), SDL_Color{140, 110, 70, 255});
            renderer.drawRect(x + S(9), y + S(9), S(14), S(14), SDL_Color{105, 165, 75, 255});

            // Specific seed dot emblem on packet
            SDL_Color seedColor = (type == ItemType::TomatoSeeds) ? SDL_Color{230, 45, 35, 255} :
                                  (type == ItemType::CarrotSeeds) ? SDL_Color{245, 140, 30, 255} :
                                                                    SDL_Color{240, 215, 60, 255};
            renderer.drawRect(x + S(13), y + S(13), S(6), S(6), seedColor);
            break;
        }

        case ItemType::Tomato: {
            // Round red tomato body
            renderer.drawRect(x + S(7), y + S(9), S(18), S(17), SDL_Color{225, 45, 35, 255});
            // Highlight shine
            renderer.drawRect(x + S(9), y + S(11), S(4), S(4), SDL_Color{255, 125, 110, 255});
            // Tomato shadow
            renderer.drawRect(x + S(12), y + S(23), S(12), S(3), SDL_Color{180, 30, 25, 255});
            // Green stem & leaves (calyx)
            renderer.drawRect(x + S(13), y + S(5), S(6), S(4), SDL_Color{45, 150, 40, 255});
            renderer.drawRect(x + S(10), y + S(7), S(12), S(3), SDL_Color{55, 175, 50, 255});
            break;
        }

        case ItemType::Carrot: {
            // Orange tapered body
            renderer.drawRect(x + S(10), y + S(11), S(12), S(6), SDL_Color{245, 135, 25, 255});
            renderer.drawRect(x + S(12), y + S(17), S(8), S(6), SDL_Color{235, 115, 15, 255});
            renderer.drawRect(x + S(14), y + S(23), S(4), S(5), SDL_Color{215, 95, 10, 255});
            // Green foliage top
            renderer.drawRect(x + S(12), y + S(5), S(8), S(6), SDL_Color{45, 165, 45, 255});
            renderer.drawRect(x + S(14), y + S(3), S(4), S(3), SDL_Color{65, 195, 65, 255});
            break;
        }

        case ItemType::Wheat: {
            // Golden stalk sheaf
            renderer.drawRect(x + S(13), y + S(15), S(6), S(13), SDL_Color{200, 160, 40, 255});
            // Wheat grain heads
            renderer.drawRect(x + S(9), y + S(7), S(6), S(9), SDL_Color{240, 205, 65, 255});
            renderer.drawRect(x + S(17), y + S(7), S(6), S(9), SDL_Color{240, 205, 65, 255});
            renderer.drawRect(x + S(13), y + S(4), S(6), S(10), SDL_Color{255, 225, 90, 255});
            // Tie ribbon
            renderer.drawRect(x + S(12), y + S(18), S(8), S(3), SDL_Color{180, 50, 40, 255});
            break;
        }

        case ItemType::Salad: {
            // Light wooden salad bowl
            renderer.drawRect(x + S(5), y + S(14), S(22), S(12), SDL_Color{165, 115, 65, 255});
            renderer.drawRect(x + S(7), y + S(24), S(18), S(3), SDL_Color{135, 90, 45, 255});
            // Green crisp lettuce salad
            renderer.drawRect(x + S(7), y + S(9), S(18), S(8), SDL_Color{60, 180, 55, 255});
            renderer.drawRect(x + S(9), y + S(7), S(14), S(4), SDL_Color{85, 205, 75, 255});
            // Diced tomato garnish
            renderer.drawRect(x + S(10), y + S(11), S(4), S(4), SDL_Color{230, 40, 30, 255});
            renderer.drawRect(x + S(18), y + S(10), S(4), S(4), SDL_Color{230, 40, 30, 255});
            break;
        }

        case ItemType::Bread: {
            // Crusty brown loaf
            renderer.drawRect(x + S(6), y + S(11), S(20), S(14), SDL_Color{195, 130, 55, 255});
            renderer.drawRect(x + S(8), y + S(8), S(16), S(6), SDL_Color{225, 160, 75, 255});
            // Scoring marks on crust
            renderer.drawRect(x + S(10), y + S(10), S(2), S(6), SDL_Color{140, 85, 30, 255});
            renderer.drawRect(x + S(15), y + S(10), S(2), S(6), SDL_Color{140, 85, 30, 255});
            renderer.drawRect(x + S(20), y + S(10), S(2), S(6), SDL_Color{140, 85, 30, 255});
            break;
        }

        default:
            break;
    }
}
