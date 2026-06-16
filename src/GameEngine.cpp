#include "../include/GameEngine.h"
#include "Character.h"
#include <iostream>

GameEngine::GameEngine() : m_isRunning(false), m_player(nullptr) {}

GameEngine::~GameEngine() {}

std::stringstream GameEngine::Init() {
    std::stringstream ss;
    ss << "初始化游戏..." << '\n';
    m_isRunning = true;
    return ss;
}

std::stringstream GameEngine::Run() {
    std::stringstream ss;
    ShowMainMenu();
    while (m_isRunning) {
        MainLoop();
    }
    ss << "游戏结束，感谢游玩！" << '\n';
    return ss;
}

std::stringstream GameEngine::ShowMainMenu() {
    int choice;
    std::stringstream ss;
    ss << "=== 校园RPG ===" << '\n';
    ss << "1. 新的游戏" << '\n';
    ss << "2. 读取存档" << '\n';
    ss << "3. 退出游戏" << '\n';
    ss << "请输入选择: ";
    std::cin >> choice;

    switch (choice) {
        case 1:
            NewGame();
            break;
        case 2:
            LoadGame();
            break;
        case 3:
        default:
            m_isRunning = false;
            break;
    }
    return ss;

}

std::stringstream GameEngine::NewGame() {
    std::stringstream ss;
    std::string name;
    ss << "创建新角色..." << '\n';
    ss << "请输入你的名字: ";
    std::cin >> name;
    m_player = std::make_shared<Character>(name);
    ss << "创建新角色成功！" << '\n';
    return ss;
}

std::stringstream GameEngine::LoadGame() {
    std::stringstream ss;
    ss << "读取存档中..." << '\n';
    // TODO: 实现读取逻辑
    return ss;
}

std::stringstream GameEngine::SaveGame() {
    std::stringstream ss;
    ss << "保存存档中..." << '\n';
    // TODO: 实现保存逻辑
    return ss;
}

std::stringstream GameEngine::MainLoop() {
    std::stringstream ss;
    if (!m_isRunning || !m_player) return ss;

    ss << "\n=== 主菜单 ===" << '\n';
    ss << "1. 查看状态" << '\n';
    ss << "2. 查看背包" << '\n';
    ss << "3. 商店" << '\n';
    ss << "4. 保存并退出" << '\n';
    ss << "请选择操作: ";
    
    int choice;
    std::cin >> choice;
    
    switch (choice) {
        case 1:
            m_player->DisplayStatus();
            break;
        case 2:
            // TODO: 调用背包UI
            ss << "打开背包..." << '\n';
            break;
        case 3:
            // TODO: 调用商店UI
            ss << "进入商店..." << '\n';
            break;
        case 4:
            SaveGame();
            m_isRunning = false;
            break;
        default:
            ss << "无效的选择！" << '\n';
            break;
    }
    return ss;
}
