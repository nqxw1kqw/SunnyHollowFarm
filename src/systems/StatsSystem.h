#pragma once

#include <string>

class HUD;

// Aggregated lifetime metrics tracked across the player's career.
struct LifetimeStats {
    int lifetimeEarnedGold = 0; // Sum of all income from selling items to Silas
    int cropsHarvested = 0;     // Total number of mature crops picked
    int fishCaught = 0;         // Total number of fish caught from pond
    int eggsCollected = 0;      // Total number of chicken eggs collected from pen
    int dishesCooked = 0;       // Total number of meals successfully cooked at stoves
    bool goalReached = false;   // True once lifetime earnings reached 1000G
};

// System managing lifetime statistics and the 1000G milestone goal.
class StatsSystem {
public:
    static constexpr int GOAL_THRESHOLD = 1000;

    StatsSystem() = default;

    // Resets all stats counters to zero and clears goal status
    void reset();

    // Event point increment hooks
    void recordSale(int goldEarned, HUD* hud = nullptr);
    void recordHarvest(int count = 1);
    void recordCatchFish(int count = 1);
    void recordEggCollected(int count = 1);
    void recordDishCooked(int count = 1);

    // Goal evaluation (triggers one-time celebration banner when reaching 1000G)
    void checkGoal(HUD* hud = nullptr);

    // Accessors
    int getLifetimeEarnedGold() const { return m_stats.lifetimeEarnedGold; }
    void setLifetimeEarnedGold(int val) { m_stats.lifetimeEarnedGold = val; }

    int getCropsHarvested() const { return m_stats.cropsHarvested; }
    void setCropsHarvested(int val) { m_stats.cropsHarvested = val; }

    int getFishCaught() const { return m_stats.fishCaught; }
    void setFishCaught(int val) { m_stats.fishCaught = val; }

    int getEggsCollected() const { return m_stats.eggsCollected; }
    void setEggsCollected(int val) { m_stats.eggsCollected = val; }

    int getDishesCooked() const { return m_stats.dishesCooked; }
    void setDishesCooked(int val) { m_stats.dishesCooked = val; }

    bool isGoalReached() const { return m_stats.goalReached; }
    void setGoalReached(bool reached) { m_stats.goalReached = reached; }

    const LifetimeStats& getStats() const { return m_stats; }
    void setStats(const LifetimeStats& stats) { m_stats = stats; }

private:
    LifetimeStats m_stats;
};
