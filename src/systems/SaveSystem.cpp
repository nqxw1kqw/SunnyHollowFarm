#include "systems/SaveSystem.h"
#include "core/Game.h"
#include "world/TileType.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <vector>
#include <utility>

namespace {

enum class JsonType { Null, Bool, Number, String, Array, Object };

struct JsonValue {
    JsonType type = JsonType::Null;
    bool boolVal = false;
    double numVal = 0.0;
    std::string strVal;
    std::vector<JsonValue> arrVal;
    std::vector<std::pair<std::string, JsonValue>> objVal;

    bool isNull() const { return type == JsonType::Null; }
    bool isBool() const { return type == JsonType::Bool; }
    bool isNumber() const { return type == JsonType::Number; }
    bool isString() const { return type == JsonType::String; }
    bool isArray() const { return type == JsonType::Array; }
    bool isObject() const { return type == JsonType::Object; }

    int asInt(int defaultVal = 0) const { return isNumber() ? static_cast<int>(std::round(numVal)) : defaultVal; }
    float asFloat(float defaultVal = 0.0f) const { return isNumber() ? static_cast<float>(numVal) : defaultVal; }
    bool asBool(bool defaultVal = false) const { return isBool() ? boolVal : defaultVal; }
    std::string asString(const std::string& defaultVal = "") const { return isString() ? strVal : defaultVal; }

    const JsonValue* get(const std::string& key) const {
        if (!isObject()) return nullptr;
        for (const auto& kv : objVal) {
            if (kv.first == key) return &kv.second;
        }
        return nullptr;
    }
};

class JsonParser {
public:
    explicit JsonParser(const std::string& text) : m_text(text) {}

    JsonValue parse() {
        skipWhitespace();
        if (m_pos >= m_text.size()) return JsonValue{};
        return parseValue();
    }

private:
    const std::string& m_text;
    size_t m_pos = 0;

    void skipWhitespace() {
        while (m_pos < m_text.size() && (std::isspace(static_cast<unsigned char>(m_text[m_pos])) || m_text[m_pos] == '\0')) {
            m_pos++;
        }
    }

    JsonValue parseValue() {
        skipWhitespace();
        if (m_pos >= m_text.size()) return JsonValue{};

        char c = m_text[m_pos];
        if (c == '"') return parseString();
        if (c == '{') return parseObject();
        if (c == '[') return parseArray();
        if (c == 't' || c == 'f') return parseBool();
        if (c == 'n') return parseNull();
        if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) return parseNumber();

        return JsonValue{};
    }

    JsonValue parseString() {
        JsonValue val;
        val.type = JsonType::String;
        m_pos++; // skip opening quote

        std::string s;
        while (m_pos < m_text.size()) {
            char c = m_text[m_pos++];
            if (c == '"') {
                val.strVal = s;
                return val;
            }
            if (c == '\\' && m_pos < m_text.size()) {
                char esc = m_text[m_pos++];
                switch (esc) {
                    case '"': s += '"'; break;
                    case '\\': s += '\\'; break;
                    case '/': s += '/'; break;
                    case 'b': s += '\b'; break;
                    case 'f': s += '\f'; break;
                    case 'n': s += '\n'; break;
                    case 'r': s += '\r'; break;
                    case 't': s += '\t'; break;
                    default: s += esc; break;
                }
            } else {
                s += c;
            }
        }
        val.strVal = s;
        return val;
    }

    JsonValue parseNumber() {
        JsonValue val;
        val.type = JsonType::Number;
        size_t start = m_pos;
        if (m_text[m_pos] == '-') m_pos++;
        while (m_pos < m_text.size() && (std::isdigit(static_cast<unsigned char>(m_text[m_pos])) || m_text[m_pos] == '.' ||
                                         m_text[m_pos] == 'e' || m_text[m_pos] == 'E' || m_text[m_pos] == '+' || m_text[m_pos] == '-')) {
            m_pos++;
        }
        std::string numStr = m_text.substr(start, m_pos - start);
        try {
            val.numVal = std::stod(numStr);
        } catch (...) {
            val.numVal = 0.0;
        }
        return val;
    }

    JsonValue parseBool() {
        JsonValue val;
        val.type = JsonType::Bool;
        if (m_text.compare(m_pos, 4, "true") == 0) {
            val.boolVal = true;
            m_pos += 4;
        } else if (m_text.compare(m_pos, 5, "false") == 0) {
            val.boolVal = false;
            m_pos += 5;
        }
        return val;
    }

    JsonValue parseNull() {
        JsonValue val;
        val.type = JsonType::Null;
        if (m_text.compare(m_pos, 4, "null") == 0) {
            m_pos += 4;
        }
        return val;
    }

    JsonValue parseArray() {
        JsonValue val;
        val.type = JsonType::Array;
        m_pos++; // skip '['
        skipWhitespace();

        if (m_pos < m_text.size() && m_text[m_pos] == ']') {
            m_pos++;
            return val;
        }

        while (m_pos < m_text.size()) {
            val.arrVal.push_back(parseValue());
            skipWhitespace();
            if (m_pos < m_text.size() && m_text[m_pos] == ',') {
                m_pos++;
                skipWhitespace();
            } else if (m_pos < m_text.size() && m_text[m_pos] == ']') {
                m_pos++;
                break;
            } else {
                break;
            }
        }
        return val;
    }

    JsonValue parseObject() {
        JsonValue val;
        val.type = JsonType::Object;
        m_pos++; // skip '{'
        skipWhitespace();

        if (m_pos < m_text.size() && m_text[m_pos] == '}') {
            m_pos++;
            return val;
        }

        while (m_pos < m_text.size()) {
            skipWhitespace();
            if (m_pos >= m_text.size() || m_text[m_pos] != '"') break;
            JsonValue keyVal = parseString();
            skipWhitespace();
            if (m_pos < m_text.size() && m_text[m_pos] == ':') {
                m_pos++;
            } else {
                break;
            }
            skipWhitespace();
            JsonValue memberVal = parseValue();
            val.objVal.emplace_back(keyVal.strVal, memberVal);

            skipWhitespace();
            if (m_pos < m_text.size() && m_text[m_pos] == ',') {
                m_pos++;
                skipWhitespace();
            } else if (m_pos < m_text.size() && m_text[m_pos] == '}') {
                m_pos++;
                break;
            } else {
                break;
            }
        }
        return val;
    }
};

std::string directionToString(Direction dir) {
    switch (dir) {
        case Direction::Down: return "Down";
        case Direction::Left: return "Left";
        case Direction::Right: return "Right";
        case Direction::Up: return "Up";
    }
    return "Down";
}

Direction directionFromString(const std::string& str) {
    if (str == "Up") return Direction::Up;
    if (str == "Left") return Direction::Left;
    if (str == "Right") return Direction::Right;
    return Direction::Down;
}

std::string tileTypeToString(TileType type) {
    switch (type) {
        case TileType::Dirt: return "Dirt";
        case TileType::TilledSoil: return "TilledSoil";
        case TileType::WateredSoil: return "WateredSoil";
        case TileType::Grass: return "Grass";
        case TileType::Path: return "Path";
        case TileType::Water: return "Water";
        default: return "Dirt";
    }
}

} // namespace

bool SaveSystem::exists(const std::string& filepath) {
    std::ifstream f(filepath);
    return f.good();
}

bool SaveSystem::removeSave(const std::string& filepath) {
    if (!exists(filepath)) return true;
    return std::remove(filepath.c_str()) == 0;
}

bool SaveSystem::save(const Game& game, const std::string& filepath) {
    std::ofstream f(filepath);
    if (!f.is_open()) {
        std::cerr << "[SaveSystem] Error: Unable to open " << filepath << " for writing." << std::endl;
        return false;
    }

    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"version\": 1,\n";
    ss << "  \"day\": " << game.getTimeSystem().getDay() << ",\n";
    ss << "  \"wallet\": " << game.getInventory().getMoney() << ",\n";
    ss << "  \"energy\": " << game.getPlayer().getEnergy() << ",\n";

    // Player
    ss << "  \"player\": {\n";
    ss << "    \"x\": " << game.getPlayer().getX() << ",\n";
    ss << "    \"y\": " << game.getPlayer().getY() << ",\n";
    ss << "    \"facing\": \"" << directionToString(game.getPlayer().getFacing()) << "\",\n";
    ss << "    \"location\": \"" << (game.getLocation() == Location::Farm ? "Farm" : "FarmhouseInterior") << "\"\n";
    ss << "  },\n";

    // Inventory
    ss << "  \"inventory\": {\n";
    ss << "    \"activeTool\": " << static_cast<int>(game.getInventory().activeTool) << ",\n";
    ss << "    \"slots\": [\n";
    const auto& slots = game.getInventory().getSlots();
    for (size_t i = 0; i < slots.size(); ++i) {
        ss << "      {\"type\": \"" << itemTypeToString(slots[i].type) << "\", \"count\": " << slots[i].count << "}";
        if (i + 1 < slots.size()) ss << ",";
        ss << "\n";
    }
    ss << "    ]\n";
    ss << "  },\n";

    // Crop field state (per plot: soil state, crop type, growth stage, watered flag)
    ss << "  \"crop_field\": [\n";
    const Map& map = game.getMap();
    const FarmSystem& farm = game.getFarmSystem();

    std::vector<std::string> plotEntries;
    for (int ty = 0; ty < Map::HEIGHT; ++ty) {
        for (int tx = 0; tx < Map::WIDTH; ++tx) {
            const Tile& tile = map.getTile(tx, ty);
            const Crop* crop = farm.findCrop(tx, ty);

            // Record all plots in farming area (5..12, 4..9) or any plot with tilled/watered soil or a crop
            bool isFarmPlot = (tx >= 5 && tx <= 12 && ty >= 4 && ty <= 9);
            if (isFarmPlot || tile.isTilled() || crop != nullptr) {
                std::ostringstream pss;
                std::string soilStr = tileTypeToString(tile.getType());
                bool isWatered = tile.isWatered();
                std::string cropStr = crop ? itemTypeToString(crop->getCropType()) : "None";
                int stage = crop ? static_cast<int>(crop->getStage()) : 0;
                float timer = crop ? crop->getStageTimer() : 0.0f;

                pss << "    {\"x\": " << tx << ", \"y\": " << ty
                    << ", \"soil_state\": \"" << soilStr << "\""
                    << ", \"watered\": " << (isWatered ? "true" : "false")
                    << ", \"crop_type\": \"" << cropStr << "\""
                    << ", \"growth_stage\": " << stage
                    << ", \"stage_timer\": " << timer << "}";
                plotEntries.push_back(pss.str());
            }
        }
    }

    for (size_t i = 0; i < plotEntries.size(); ++i) {
        ss << plotEntries[i];
        if (i + 1 < plotEntries.size()) ss << ",";
        ss << "\n";
    }
    ss << "  ]\n";
    ss << "}\n";

    f << ss.str();
    f.close();
    std::cout << "[SaveSystem] Game state saved successfully to " << filepath << std::endl;
    return true;
}

bool SaveSystem::load(Game& game, const std::string& filepath) {
    std::ifstream f(filepath);
    if (!f.is_open()) {
        std::cerr << "[SaveSystem] Error: Unable to open " << filepath << " for reading." << std::endl;
        return false;
    }

    std::stringstream buffer;
    buffer << f.rdbuf();
    std::string text = buffer.str();
    f.close();

    JsonParser parser(text);
    JsonValue root = parser.parse();
    if (!root.isObject()) {
        std::cerr << "[SaveSystem] Error: Save file root is not a valid JSON object." << std::endl;
        return false;
    }

    // 1. Restore day
    if (const JsonValue* dayVal = root.get("day")) {
        game.getTimeSystem().setDay(dayVal->asInt(1));
    }

    // 2. Restore wallet
    if (const JsonValue* walletVal = root.get("wallet")) {
        game.getInventory().setMoney(walletVal->asInt(Config::INITIAL_MONEY));
    }

    // 3. Restore player energy
    if (const JsonValue* energyVal = root.get("energy")) {
        game.getPlayer().setEnergy(energyVal->asInt(Config::INITIAL_ENERGY));
    }

    // 4. Restore player position, facing, location
    if (const JsonValue* playerObj = root.get("player")) {
        if (playerObj->isObject()) {
            float px = 96.0f, py = 192.0f;
            if (const JsonValue* xVal = playerObj->get("x")) px = xVal->asFloat(px);
            if (const JsonValue* yVal = playerObj->get("y")) py = yVal->asFloat(py);
            game.getPlayer().setPosition(px, py);

            if (const JsonValue* facingVal = playerObj->get("facing")) {
                game.getPlayer().setFacing(directionFromString(facingVal->asString("Down")));
            }

            if (const JsonValue* locVal = playerObj->get("location")) {
                std::string locStr = locVal->asString("Farm");
                if (locStr == "FarmhouseInterior") {
                    game.setLocation(Location::FarmhouseInterior);
                } else {
                    game.setLocation(Location::Farm);
                }
            }
        }
    }

    // 5. Restore inventory
    if (const JsonValue* invObj = root.get("inventory")) {
        if (invObj->isObject()) {
            if (const JsonValue* toolVal = invObj->get("activeTool")) {
                game.getInventory().activeTool = static_cast<ToolType>(toolVal->asInt(0));
            }

            if (const JsonValue* slotsArr = invObj->get("slots")) {
                if (slotsArr->isArray()) {
                    auto& slots = game.getInventory().getSlots();
                    slots.clear();
                    for (const auto& itemVal : slotsArr->arrVal) {
                        if (itemVal.isObject()) {
                            std::string typeStr = "Empty";
                            int count = 0;
                            if (const JsonValue* t = itemVal.get("type")) typeStr = t->asString("Empty");
                            if (const JsonValue* c = itemVal.get("count")) count = c->asInt(0);

                            ItemType itemType = itemTypeFromString(typeStr);
                            slots.push_back(InventorySlot{itemType, count});
                        }
                    }
                    if (slots.empty()) {
                        slots.resize(Config::INVENTORY_SLOT_COUNT);
                    }
                }
            }
        }
    }

    // 6. Restore crop field state
    game.getMap().initDefault();
    game.getFarmSystem().clearCrops();

    if (const JsonValue* fieldArr = root.get("crop_field")) {
        if (fieldArr->isArray()) {
            for (const auto& plotVal : fieldArr->arrVal) {
                if (plotVal.isObject()) {
                    int tx = 0, ty = 0;
                    std::string soilStr = "Dirt";
                    bool isWatered = false;
                    std::string cropStr = "None";
                    int stage = 0;
                    float timer = 0.0f;

                    if (const JsonValue* v = plotVal.get("x")) tx = v->asInt(0);
                    if (const JsonValue* v = plotVal.get("y")) ty = v->asInt(0);
                    if (const JsonValue* v = plotVal.get("soil_state")) soilStr = v->asString("Dirt");
                    if (const JsonValue* v = plotVal.get("watered")) isWatered = v->asBool(false);
                    if (const JsonValue* v = plotVal.get("crop_type")) cropStr = v->asString("None");
                    if (const JsonValue* v = plotVal.get("growth_stage")) stage = v->asInt(0);
                    if (const JsonValue* v = plotVal.get("stage_timer")) timer = v->asFloat(0.0f);

                    if (game.getMap().inBounds(tx, ty)) {
                        if (isWatered || soilStr == "WateredSoil") {
                            game.getMap().setTileType(tx, ty, TileType::WateredSoil);
                        } else if (soilStr == "TilledSoil") {
                            game.getMap().setTileType(tx, ty, TileType::TilledSoil);
                        } else if (soilStr == "Dirt") {
                            game.getMap().setTileType(tx, ty, TileType::Dirt);
                        }

                        if (cropStr != "None" && cropStr != "Empty" && !cropStr.empty() && stage > 0) {
                            ItemType cropType = itemTypeFromString(cropStr);
                            CropStage cropStage = static_cast<CropStage>(stage);
                            game.getFarmSystem().addCrop(Crop(tx, ty, cropType, cropStage, timer));
                        }
                    }
                }
            }
        }
    }

    std::cout << "[SaveSystem] Game state successfully loaded from " << filepath << std::endl;
    return true;
}
