#pragma once

#include "entities/Inventory.h"
#include "ui/HUD.h"
#include <vector>
#include <string>

// Represents an individual ingredient and quantity required by a recipe.
struct RecipeIngredient {
    ItemType item = ItemType::Empty;
    int count = 1;
};

// Represents a cooking recipe loaded from assets/data/recipes.txt.
struct Recipe {
    ItemType result = ItemType::Empty;
    std::string dishName;
    std::vector<RecipeIngredient> ingredients;
};

class AudioSystem;

#include <functional>

// Cooking system that parses recipes.txt, verifies ingredients, and executes cooking.
class CookingSystem {
public:
    using OnCookSuccessCallback = std::function<void(int count)>;

    CookingSystem() = default;

    void setOnCookSuccessCallback(OnCookSuccessCallback cb) { m_onCookSuccessCallback = cb; }

    // Parses recipes from the given file path (supporting relative path fallbacks).
    bool loadRecipes(const std::string& filePath = "assets/data/recipes.txt");

    const std::vector<Recipe>& getRecipes() const { return m_recipes; }
    int getRecipeCount() const { return static_cast<int>(m_recipes.size()); }
    const Recipe* getRecipe(int index) const;
    const Recipe* findRecipeByDish(ItemType dish) const;

    // Checks if the player possesses all ingredients needed for the recipe.
    bool canCook(const Recipe& recipe, const Inventory& inventory, std::string* missingInfo = nullptr) const;

    // Attempts to cook the recipe at recipeIndex. Consumes ingredients and adds the dish on success.
    // Provides clear on-screen HUD feedback and console logging on missing ingredients or success.
    bool cook(int recipeIndex, Inventory& inventory, HUD* hud = nullptr, AudioSystem* audio = nullptr);

    // Convenience overload to cook by dish ItemType.
    bool cook(ItemType dish, Inventory& inventory, HUD* hud = nullptr, AudioSystem* audio = nullptr);

private:
    std::vector<Recipe> m_recipes;
    OnCookSuccessCallback m_onCookSuccessCallback;
};
