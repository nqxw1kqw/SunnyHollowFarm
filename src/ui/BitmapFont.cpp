#include "ui/BitmapFont.h"
#include <cctype>

// Returns the 15-bit pattern (row-major 3 cols x 5 rows) for supported characters.
static uint16_t getGlyphPattern(char ch) {
    ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));

    if (ch >= '0' && ch <= '9') {
        static const uint16_t DIGITS[10] = {
            0x7B6F, // 0: 111 101 101 101 111
            0x2492, // 1: 010 010 010 010 010
            0x73E7, // 2: 111 001 111 100 111
            0x73CF, // 3: 111 001 111 001 111
            0x5BC9, // 4: 101 101 111 001 001
            0x79CF, // 5: 111 100 111 001 111
            0x79EF, // 6: 111 100 111 101 111
            0x7249, // 7: 111 001 010 010 010
            0x7BEF, // 8: 111 101 111 101 111
            0x7BCF  // 9: 111 101 111 001 111
        };
        return DIGITS[ch - '0'];
    }

    if (ch >= 'A' && ch <= 'Z') {
        static const uint16_t LETTERS[26] = {
            0x2BF5, // A: 010 101 111 101 101
            0x6DB6, // B: 110 101 110 101 110
            0x7927, // C: 111 100 100 100 111
            0x6B6E, // D: 110 101 101 101 110
            0x79A7, // E: 111 100 110 100 111
            0x79A4, // F: 111 100 110 100 100
            0x795F, // G: 111 100 101 101 111
            0x5BF5, // H: 101 101 111 101 101
            0x724F, // I: 111 010 010 010 111
            0x126A, // J: 001 001 001 101 010
            0x5DC5, // K: 101 110 100 110 101
            0x4927, // L: 100 100 100 100 111
            0x5EF5, // M: 101 111 101 101 101
            0x6B6D, // N: 110 101 101 101 101
            0x7B6F, // O: 111 101 101 101 111
            0x7BE4, // P: 111 101 111 100 100
            0x7B71, // Q: 111 101 101 111 001
            0x7BA5, // R: 111 101 110 101 101
            0x79CF, // S: 111 100 111 001 111
            0x7249, // T: 111 010 010 010 010
            0x5B6F, // U: 101 101 101 101 111
            0x5B6A, // V: 101 101 101 101 010
            0x5B7D, // W: 101 101 101 111 101
            0x5A95, // X: 101 101 010 101 101
            0x5A49, // Y: 101 101 010 010 010
            0x72A7  // Z: 111 001 010 100 111
        };
        return LETTERS[ch - 'A'];
    }

    switch (ch) {
        case '!': return 0x2482; // 010 010 010 000 010
        case ':': return 0x0410; // 000 010 000 010 000
        case '/': return 0x12A4; // 001 001 010 100 100
        case '-': return 0x01C0; // 000 000 111 000 000
        case '+': return 0x05D0; // 000 010 111 010 000
        case '[': return 0x6926; // 110 100 100 100 110
        case ']': return 0x324B; // 011 001 001 001 011
        case '.': return 0x0002; // 000 000 000 000 010
        case '$': return 0x2FEA; // 010 111 111 101 010
        default:  return 0x0000; // Space or unsupported
    }
}

void BitmapFont::drawChar(Renderer& renderer, char c, int x, int y, SDL_Color color, int scale) {
    uint16_t pattern = getGlyphPattern(c);
    if (pattern == 0) return;

    for (int row = 0; row < 5; ++row) {
        for (int col = 0; col < 3; ++col) {
            int bitIndex = 14 - (row * 3 + col);
            if ((pattern >> bitIndex) & 1) {
                renderer.drawRect(x + col * scale, y + row * scale, scale, scale, color);
            }
        }
    }
}

void BitmapFont::drawText(Renderer& renderer, const std::string& text, int x, int y, SDL_Color color, int scale) {
    int curX = x;
    int charAdvance = (3 * scale) + (1 * scale); // 3-pixel width + 1-pixel gap

    for (char c : text) {
        drawChar(renderer, c, curX, y, color, scale);
        curX += charAdvance;
    }
}

void BitmapFont::drawNumber(Renderer& renderer, int number, int x, int y, SDL_Color color, int scale) {
    drawText(renderer, std::to_string(number), x, y, color, scale);
}

int BitmapFont::getTextWidth(const std::string& text, int scale) {
    if (text.empty()) return 0;
    int charAdvance = (3 * scale) + (1 * scale);
    return static_cast<int>(text.length()) * charAdvance;
}
