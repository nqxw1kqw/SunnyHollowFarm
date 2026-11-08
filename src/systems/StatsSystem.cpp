#include "systems/StatsSystem.h"
#include "ui/HUD.h"
#include <iostream>

void StatsSystem::reset() {
    m_stats = LifetimeStats{};
}

void StatsSystem::recordSale(int goldEarned, HUD* hud) {
    if (goldEarned <= 0) return;
    m_stats.lifetimeEarnedGold += goldEarned;
    checkGoal(hud);
}

void StatsSystem::recordHarvest(int count) {
    if (count > 0) {
        m_stats.cropsHarvested += count;
    }
}

void StatsSystem::recordCatchFish(int count) {
    if (count > 0) {
        m_stats.fishCaught += count;
    }
}

void StatsSystem::recordEggCollected(int count) {
    if (count > 0) {
        m_stats.eggsCollected += count;
    }
}

void StatsSystem::recordDishCooked(int count) {
    if (count > 0) {
        m_stats.dishesCooked += count;
    }
}

void StatsSystem::checkGoal(HUD* hud) {
    if (!m_stats.goalReached && m_stats.lifetimeEarnedGold >= GOAL_THRESHOLD) {
        m_stats.goalReached = true;
        if (hud) {
            hud->showFeedback("FARM GOAL REACHED: 1000G lifetime earnings!", 5.0f);
        }
        std::cout << "[Goal] FARM GOAL REACHED: 1000G lifetime earnings!" << std::endl;
    }
}
