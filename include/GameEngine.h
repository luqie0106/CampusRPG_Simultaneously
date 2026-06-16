#pragma once
#include "Common.h"
#include "Character.h"
#include "Backpack.h"

class GameEngine {
public:
    GameEngine();
    ~GameEngine();

    // 初始化游戏
    std::stringstream Init();

    // 游戏主循环
    std::stringstream Run();

private:
    std::stringstream ShowMainMenu();
    std::stringstream NewGame();
    std::stringstream LoadGame();
    std::stringstream SaveGame();
    std::stringstream MainLoop();
    std::stringstream HandleEvents();

    bool m_isRunning;
    std::shared_ptr<Character> m_player;
};
