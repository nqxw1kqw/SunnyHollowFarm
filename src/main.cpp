#include "core/Game.h"
#include <cstring>
#include <iostream>

int main(int argc, char* argv[]) {
    bool smokeTest = false;
    bool playtest = false;

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--smoke-test") == 0 || std::strcmp(argv[i], "-t") == 0) {
            smokeTest = true;
        } else if (std::strcmp(argv[i], "--playtest") == 0 || std::strcmp(argv[i], "-p") == 0) {
            playtest = true;
        }
    }

    if (smokeTest || playtest) {
#if defined(__unix__) || defined(__linux__) || defined(__APPLE__)
        setenv("SDL_AUDIODRIVER", "dummy", 0);
#endif
    }

    Game game;
    // 20x15 tiles of 32x32 pixels = 640x480 resolution
    if (!game.init("Sunny Hollow Farm", 640, 480, smokeTest, playtest)) {
        std::cerr << "Failed to initialize game!" << std::endl;
        return 1;
    }

    game.run();
    game.cleanUp();

    std::cout << "Sunny Hollow Farm exited cleanly." << std::endl;
    return 0;
}
