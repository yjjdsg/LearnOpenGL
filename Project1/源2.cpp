// 源2.cpp
// -----------------------------------------------------------------------------
// Entry point only. Game is declared in game.h and implemented in game.cpp -
// both are compiled by the project, so this file must NOT include game.cpp.
// Including it would give every Game member a second definition (LNK2005).
// -----------------------------------------------------------------------------

#include "game.h"

#include <iostream>

int main()
{
    Game game;
    if (!game.init())
    {
        std::cout << "Game initialisation failed" << std::endl;
        return -1;
    }

    game.run();
    return 0;
}
