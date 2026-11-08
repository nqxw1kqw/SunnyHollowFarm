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
        ItemType::Salad, "Garden Salad", "Fresh garden vegetables tossed together.", 50, 40, false, true, 25
    };
    static const ItemDefinition breadDef{
        ItemType::Bread, "Fresh Bread", "A warm, crusty loaf baked from milled wheat.", 60, 50, false, true, 35
    };
    static const ItemDefinition fishDef{
        ItemType::Fish, "Fish", "A fresh fish caught from the pond.", 0, 30, false, false, 0
    };
    static const ItemDefinition grilledFishDef{
        ItemType::GrilledFish, "Grilled Fish", "A hot grilled fish with crispy skin.", 0, 55, false, false, 0
    };
    static const ItemDefinition eggDef{
        ItemType::Egg, "Egg", "A fresh farm egg laid by a happy chicken.", 0, 15, false, false, 0
    };
    static const ItemDefinition omeletteDef{
        ItemType::Omelette, "Omelette", "A hot, fluffy golden omelette cooked from fresh farm eggs.", 0, 50, false, true, 40
    };
    static const ItemDefinition chickenDef{
        ItemType::Chicken, "Chicken", "A healthy egg-laying hen for the chicken pen.", 100, 0, false, false, 0
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
        case ItemType::Fish:         return fishDef;
        case ItemType::GrilledFish:  return grilledFishDef;
        case ItemType::Egg:          return eggDef;
        case ItemType::Omelette:     return omeletteDef;
        case ItemType::Chicken:      return chickenDef;
        default:                     return emptyDef;
    }
}

void drawItemPlaceholder(Renderer& renderer, ItemType type, int x, int y, int size) {
    if (type == ItemType::Empty) return;

    const char* sprite = nullptr;
    switch (type) {
        case ItemType::TomatoSeeds: sprite = "item_seed_packet.png"; break;
        case ItemType::Tomato: sprite = "item_tomato.png"; break;
        case ItemType::Carrot: sprite = "item_carrot.png"; break;
        case ItemType::Wheat: sprite = "item_wheat.png"; break;
        case ItemType::Salad: sprite = "item_salad.png"; break;
        case ItemType::Bread: sprite = "item_bread.png"; break;
        case ItemType::Fish: sprite = "item_fish.png"; break;
        case ItemType::GrilledFish: sprite = "item_grilled_fish.png"; break;
        case ItemType::Egg: sprite = "item_egg.png"; break;
        case ItemType::Omelette: sprite = "item_omelette.png"; break;
        case ItemType::Chicken: sprite = "chicken_idle.png"; break;
        default: break;
    }
    if (sprite && renderer.drawSprite(sprite, x, y, size, size)) return;

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

        case ItemType::Fish: {
            // Fish body: silvery-blue curved fish
            renderer.drawRect(x + S(6), y + S(11), S(16), S(10), SDL_Color{70, 135, 195, 255});
            renderer.drawRect(x + S(8), y + S(9), S(12), S(14), SDL_Color{90, 160, 225, 255});
            // Light belly
            renderer.drawRect(x + S(10), y + S(16), S(8), S(5), SDL_Color{205, 235, 250, 255});
            // Orange-red tail fin
            renderer.drawRect(x + S(22), y + S(8), S(4), S(16), SDL_Color{235, 110, 45, 255});
            renderer.drawRect(x + S(25), y + S(6), S(3), S(20), SDL_Color{215, 90, 35, 255});
            // Eye
            renderer.drawRect(x + S(8), y + S(12), S(3), S(3), SDL_Color{250, 250, 250, 255});
            renderer.drawRect(x + S(8), y + S(13), S(2), S(2), SDL_Color{20, 25, 30, 255});
            // Dorsal fin
            renderer.drawRect(x + S(11), y + S(6), S(6), S(3), SDL_Color{235, 110, 45, 255});
            break;
        }

        case ItemType::GrilledFish: {
            // Wooden skewer stick
            renderer.drawRect(x + S(3), y + S(24), S(26), S(3), SDL_Color{175, 120, 65, 255});
            // Golden-brown grilled fish body
            renderer.drawRect(x + S(6), y + S(11), S(16), S(10), SDL_Color{195, 125, 55, 255});
            renderer.drawRect(x + S(8), y + S(9), S(12), S(14), SDL_Color{220, 150, 75, 255});
            // Tail fin
            renderer.drawRect(x + S(22), y + S(8), S(4), S(16), SDL_Color{165, 95, 40, 255});
            // Grill char marks
            renderer.drawRect(x + S(10), y + S(9), S(2), S(14), SDL_Color{85, 45, 20, 255});
            renderer.drawRect(x + S(14), y + S(9), S(2), S(14), SDL_Color{85, 45, 20, 255});
            renderer.drawRect(x + S(18), y + S(9), S(2), S(14), SDL_Color{85, 45, 20, 255});
            // Eye mark
            renderer.drawRect(x + S(8), y + S(12), S(2), S(2), SDL_Color{60, 30, 15, 255});
            break;
        }

        case ItemType::Egg: {
            // Egg shell shadow
            renderer.drawRect(x + S(10), y + S(22), S(12), S(3), SDL_Color{205, 195, 175, 255});
            // Oval egg body (ivory / warm cream)
            renderer.drawRect(x + S(11), y + S(8), S(10), S(4), SDL_Color{248, 244, 232, 255});
            renderer.drawRect(x + S(9), y + S(12), S(14), S(10), SDL_Color{252, 248, 238, 255});
            renderer.drawRect(x + S(10), y + S(20), S(12), S(3), SDL_Color{235, 225, 205, 255});
            // Shell highlight shine
            renderer.drawRect(x + S(12), y + S(10), S(3), S(3), SDL_Color{255, 255, 255, 255});
            break;
        }

        case ItemType::Omelette: {
            // Hot skillet / plate base
            renderer.drawRect(x + S(5), y + S(22), S(22), S(4), SDL_Color{90, 85, 95, 255});
            renderer.drawRect(x + S(3), y + S(20), S(26), S(3), SDL_Color{120, 115, 125, 255});
            // Fluffy golden folded omelette
            renderer.drawRect(x + S(6), y + S(12), S(20), S(9), SDL_Color{250, 195, 45, 255});
            renderer.drawRect(x + S(8), y + S(10), S(16), S(4), SDL_Color{255, 215, 75, 255});
            // Light browning along fold
            renderer.drawRect(x + S(9), y + S(15), S(14), S(2), SDL_Color{215, 140, 25, 255});
            // Fresh chopped green chives garnish
            renderer.drawRect(x + S(11), y + S(12), S(2), S(2), SDL_Color{45, 155, 40, 255});
            renderer.drawRect(x + S(17), y + S(13), S(2), S(2), SDL_Color{45, 155, 40, 255});
            renderer.drawRect(x + S(14), y + S(17), S(2), S(2), SDL_Color{45, 155, 40, 255});
            break;
        }

        case ItemType::Chicken: {
            // Cute mini chicken icon for shop listing
            // Body
            renderer.drawRect(x + S(9), y + S(12), S(14), S(11), SDL_Color{250, 250, 250, 255});
            // Head
            renderer.drawRect(x + S(15), y + S(8), S(8), S(8), SDL_Color{255, 255, 255, 255});
            // Red comb
            renderer.drawRect(x + S(17), y + S(5), S(5), S(3), SDL_Color{225, 40, 40, 255});
            // Yellow beak
            renderer.drawRect(x + S(23), y + S(11), S(4), S(3), SDL_Color{250, 190, 30, 255});
            // Eye
            renderer.drawRect(x + S(19), y + S(10), S(2), S(2), SDL_Color{30, 30, 30, 255});
            // Feet
            renderer.drawRect(x + S(12), y + S(23), S(3), S(4), SDL_Color{240, 150, 20, 255});
            renderer.drawRect(x + S(17), y + S(23), S(3), S(4), SDL_Color{240, 150, 20, 255});
            break;
        }

        default:
            break;
    }
}

ItemType itemTypeFromString(const std::string& name) {
    if (name == "TomatoSeeds" || name == "Tomato Seeds" || name == "tomato_seeds") return ItemType::TomatoSeeds;
    if (name == "CarrotSeeds" || name == "Carrot Seeds" || name == "carrot_seeds") return ItemType::CarrotSeeds;
    if (name == "WheatSeeds" || name == "Wheat Seeds" || name == "wheat_seeds") return ItemType::WheatSeeds;
    if (name == "Tomato" || name == "tomato") return ItemType::Tomato;
    if (name == "Carrot" || name == "carrot") return ItemType::Carrot;
    if (name == "Wheat" || name == "wheat") return ItemType::Wheat;
    if (name == "Salad" || name == "salad") return ItemType::Salad;
    if (name == "Bread" || name == "bread") return ItemType::Bread;
    if (name == "Fish" || name == "fish") return ItemType::Fish;
    if (name == "GrilledFish" || name == "Grilled Fish" || name == "grilled_fish" || name == "Grilled_Fish") return ItemType::GrilledFish;
    if (name == "Egg" || name == "egg") return ItemType::Egg;
    if (name == "Omelette" || name == "omelette" || name == "Omelet" || name == "omelet") return ItemType::Omelette;
    if (name == "Chicken" || name == "chicken") return ItemType::Chicken;
    return ItemType::Empty;
}

std::string itemTypeToString(ItemType type) {
    switch (type) {
        case ItemType::TomatoSeeds: return "Tomato Seeds";
        case ItemType::CarrotSeeds: return "Carrot Seeds";
        case ItemType::WheatSeeds:  return "Wheat Seeds";
        case ItemType::Tomato:       return "Tomato";
        case ItemType::Carrot:       return "Carrot";
        case ItemType::Wheat:        return "Wheat";
        case ItemType::Salad:        return "Salad";
        case ItemType::Bread:        return "Bread";
        case ItemType::Fish:         return "Fish";
        case ItemType::GrilledFish:  return "Grilled Fish";
        case ItemType::Egg:          return "Egg";
        case ItemType::Omelette:     return "Omelette";
        case ItemType::Chicken:      return "Chicken";
        default:                     return "Empty";
    }
}
