#include "systems/CookingSystem.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

namespace {
    std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, (last - first + 1));
    }
}

bool CookingSystem::loadRecipes(const std::string& filePath) {
    m_recipes.clear();

    // Check multiple candidate file paths so running from repo root or build/ works seamlessly
    std::vector<std::string> candidatePaths = {
        filePath,
        "../" + filePath,
        "assets/data/recipes.txt",
        "../assets/data/recipes.txt",
        "/home/aika/projects/SunnyHollowFarm/assets/data/recipes.txt"
    };

    std::ifstream file;
    std::string usedPath;
    for (const auto& path : candidatePaths) {
        file.open(path);
        if (file.is_open()) {
            usedPath = path;
            break;
        }
    }

    if (!file.is_open()) {
        std::cerr << "[CookingSystem] Warning: Unable to open recipes file (" << filePath
                  << "). Setting up built-in default recipes." << std::endl;
        // Built-in fallback
        m_recipes.push_back({ItemType::Salad, "Salad", {{ItemType::Tomato, 2}}});
        m_recipes.push_back({ItemType::Bread, "Bread", {{ItemType::Wheat, 3}}});
        m_recipes.push_back({ItemType::GrilledFish, "Grilled Fish", {{ItemType::Fish, 1}}});
        return false;
    }

    std::string line;
    int lineNum = 0;
    while (std::getline(file, line)) {
        lineNum++;
        std::string trimmed = trim(line);
        if (trimmed.empty() || trimmed[0] == '#') {
            continue; // Ignore blank lines and comments
        }

        // Expected format: <DishName> = <Ing1>:<Count>, <Ing2>:<Count>
        size_t eqPos = trimmed.find('=');
        if (eqPos == std::string::npos) {
            std::cerr << "[CookingSystem] Warning: Malformed recipe at line " << lineNum << ": " << line << std::endl;
            continue;
        }

        std::string dishName = trim(trimmed.substr(0, eqPos));
        std::string ingList = trim(trimmed.substr(eqPos + 1));

        ItemType resultType = itemTypeFromString(dishName);
        if (resultType == ItemType::Empty) {
            std::cerr << "[CookingSystem] Warning: Unknown dish item '" << dishName
                      << "' at line " << lineNum << std::endl;
            continue;
        }

        Recipe recipe;
        recipe.result = resultType;
        recipe.dishName = dishName;

        // Parse comma-separated ingredients
        std::stringstream ss(ingList);
        std::string itemToken;
        bool valid = true;

        while (std::getline(ss, itemToken, ',')) {
            std::string trimmedToken = trim(itemToken);
            if (trimmedToken.empty()) continue;

            size_t colonPos = trimmedToken.find(':');
            size_t xPos = (colonPos == std::string::npos) ? trimmedToken.find('x') : std::string::npos;
            if (xPos == std::string::npos && colonPos == std::string::npos) {
                xPos = trimmedToken.find('X');
            }
            std::string ingName;
            int count = 1;

            if (colonPos != std::string::npos) {
                ingName = trim(trimmedToken.substr(0, colonPos));
                try {
                    count = std::stoi(trim(trimmedToken.substr(colonPos + 1)));
                } catch (...) {
                    count = 1;
                }
            } else if (xPos != std::string::npos) {
                ingName = trim(trimmedToken.substr(0, xPos));
                try {
                    count = std::stoi(trim(trimmedToken.substr(xPos + 1)));
                } catch (...) {
                    count = 1;
                }
            } else {
                ingName = trimmedToken;
            }

            ItemType ingType = itemTypeFromString(ingName);
            if (ingType == ItemType::Empty) {
                std::cerr << "[CookingSystem] Warning: Unknown ingredient '" << ingName
                          << "' in recipe " << dishName << std::endl;
                valid = false;
                break;
            }

            recipe.ingredients.push_back({ingType, count});
        }

        if (valid && !recipe.ingredients.empty()) {
            m_recipes.push_back(recipe);
            std::cout << "[CookingSystem] Loaded recipe: " << dishName << " (";
            for (size_t i = 0; i < recipe.ingredients.size(); ++i) {
                if (i > 0) std::cout << ", ";
                std::cout << itemTypeToString(recipe.ingredients[i].item) << " x" << recipe.ingredients[i].count;
            }
            std::cout << ")" << std::endl;
        }
    }

    std::cout << "[CookingSystem] Successfully loaded " << m_recipes.size()
              << " recipe(s) from " << usedPath << std::endl;
    return true;
}

const Recipe* CookingSystem::getRecipe(int index) const {
    if (index >= 0 && index < static_cast<int>(m_recipes.size())) {
        return &m_recipes[index];
    }
    return nullptr;
}

const Recipe* CookingSystem::findRecipeByDish(ItemType dish) const {
    for (const auto& r : m_recipes) {
        if (r.result == dish) {
            return &r;
        }
    }
    return nullptr;
}

bool CookingSystem::canCook(const Recipe& recipe, const Inventory& inventory, std::string* missingInfo) const {
    for (const auto& ing : recipe.ingredients) {
        int have = inventory.getItemCount(ing.item);
        if (have < ing.count) {
            if (missingInfo) {
                *missingInfo = "Missing " + std::to_string(ing.count - have) + " " +
                               itemTypeToString(ing.item) + "!";
            }
            return false;
        }
    }
    return true;
}

#include "systems/AudioSystem.h"

bool CookingSystem::cook(int recipeIndex, Inventory& inventory, HUD* hud, AudioSystem* audio) {
    const Recipe* r = getRecipe(recipeIndex);
    if (!r) {
        std::cerr << "[CookingSystem] Error: Invalid recipe index " << recipeIndex << std::endl;
        if (audio) audio->playSFX(SFX::CookFail);
        return false;
    }

    std::string missingInfo;
    if (!canCook(*r, inventory, &missingInfo)) {
        std::cout << "[CookingSystem] Cannot cook " << r->dishName << ": " << missingInfo << std::endl;
        if (hud) {
            hud->showFeedback("MISSING INGREDIENTS!");
        }
        if (audio) audio->playSFX(SFX::CookFail);
        return false;
    }

    bool willFreeSlot = false;
    for (const auto& ing : r->ingredients) {
        if (inventory.getItemCount(ing.item) == ing.count) {
            willFreeSlot = true;
            break;
        }
    }
    if (!inventory.hasSpaceFor(r->result, 1) && !willFreeSlot) {
        std::cout << "[CookingSystem] Cannot cook " << r->dishName << ": Inventory full!" << std::endl;
        if (hud) {
            hud->showFeedback("INVENTORY FULL!");
        }
        if (audio) audio->playSFX(SFX::CookFail);
        return false;
    }

    // Deduct ingredients
    for (const auto& ing : r->ingredients) {
        inventory.removeItem(ing.item, ing.count);
    }

    // Add resulting dish
    inventory.addItem(r->result, 1);

    std::cout << "[CookingSystem] Successfully cooked 1x " << r->dishName << "!" << std::endl;
    if (audio) audio->playSFX(SFX::CookSuccess);
    if (hud) {
        std::string msg = "COOKED: " + r->dishName + "!";
        std::transform(msg.begin(), msg.end(), msg.begin(), ::toupper);
        hud->showFeedback(msg);
    }

    return true;
}

bool CookingSystem::cook(ItemType dish, Inventory& inventory, HUD* hud, AudioSystem* audio) {
    for (int i = 0; i < static_cast<int>(m_recipes.size()); ++i) {
        if (m_recipes[i].result == dish) {
            return cook(i, inventory, hud, audio);
        }
    }
    std::cerr << "[CookingSystem] Error: No recipe found for " << itemTypeToString(dish) << std::endl;
    if (audio) audio->playSFX(SFX::CookFail);
    return false;
}
