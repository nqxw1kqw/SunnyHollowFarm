#pragma once

#include "core/Renderer.h"
#include <string>

// NPC merchant roles.
enum class NPCType {
    Seller, // Sells seeds, seedlings, and pantry basics
    Buyer   // Buys harvested farm crops and prepared dishes
};

// Represents an interactive non-player character in Sunny Hollow Farm.
class NPC {
public:
    NPC(int tileX, int tileY, NPCType type, const std::string& name);

    int getTileX() const { return m_tileX; }
    int getTileY() const { return m_tileY; }
    NPCType getType() const { return m_type; }
    const std::string& getName() const { return m_name; }

    // Updates idle motion animation timer
    void update(float dt);
    float getAnimTimer() const { return m_animTimer; }
    void setAnimTimer(float t) { m_animTimer = t; }
    int getIdleBob() const;

    // Renders custom placeholder pixel art for the NPC
    void render(Renderer& renderer) const;

private:
    int m_tileX = 0;
    int m_tileY = 0;
    NPCType m_type = NPCType::Seller;
    std::string m_name;
    float m_animTimer = 0.0f;
};
