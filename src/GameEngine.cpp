#include "../include/GameEngine.h"
#include "../include/Exceptions.h"

// ─────────────────────────────────────────────────────────────────────────────
// 构造 / 析构
// ─────────────────────────────────────────────────────────────────────────────

GameEngine::GameEngine()
    : m_state(GameState::Uninitialized), m_player(nullptr) {}

GameEngine::~GameEngine() {}

// ─────────────────────────────────────────────────────────────────────────────
// 生命周期
// ─────────────────────────────────────────────────────────────────────────────

std::string GameEngine::Init() {
    m_state = GameState::MainMenu;
    return "游戏引擎初始化完成。\n";
}

GameState GameEngine::GetState() const {
    return m_state;
}

std::shared_ptr<Character> GameEngine::GetPlayer() const {
    return m_player;
}

// ─────────────────────────────────────────────────────────────────────────────
// 主菜单
// ─────────────────────────────────────────────────────────────────────────────

std::string GameEngine::GetMainMenuText() const {
    std::stringstream ss;
    ss << "=== 校园RPG ===\n";
    ss << "1. 新的游戏\n";
    ss << "2. 读取存档\n";
    ss << "3. 退出游戏\n";
    return ss.str();
}

std::string GameEngine::HandleMainMenuChoice(int choice) {
    switch (choice) {
        case 1:
            // UI 应在收到此响应后，切换到"输入角色名"界面并调用 CreatePlayer()
            return "请输入角色名称以开始新游戏。\n";
        case 2:
            return LoadGame();
        case 3:
            return QuitGame();
        default:
            return "无效的选择，请输入 1、2 或 3。\n";
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// 创建角色
// ─────────────────────────────────────────────────────────────────────────────

std::string GameEngine::CreatePlayer(const std::string& name) {
    if (name.empty()) {
        return "错误：角色名称不能为空。\n";
    }
    m_player = std::make_shared<Character>(name);
    m_state  = GameState::InGame;
    std::stringstream ss;
    ss << "角色「" << name << "」创建成功！冒险开始！\n";
    return ss.str();
}

// ─────────────────────────────────────────────────────────────────────────────
// 游戏内主菜单
// ─────────────────────────────────────────────────────────────────────────────

std::string GameEngine::GetInGameMenuText() const {
    std::stringstream ss;
    ss << "=== 主菜单 ===\n";
    ss << "1. 查看状态\n";
    ss << "2. 查看背包\n";
    ss << "3. 商店\n";
    ss << "4. 保存并退出\n";
    return ss.str();
}

std::string GameEngine::HandleInGameMenuChoice(int choice) {
    if (!m_player) {
        return "错误：尚未创建角色。\n";
    }
    switch (choice) {
        case 1:
            return GetPlayerStatus();
        case 2:
            return EnterBackpack();
        case 3:
            return EnterShop();
        case 4: {
            std::string saveResult = SaveGame();
            QuitGame();
            return saveResult + "已退出游戏。\n";
        }
        default:
            return "无效的选择！\n";
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// 角色状态
// ─────────────────────────────────────────────────────────────────────────────

std::string GameEngine::GetPlayerStatus() const {
    if (!m_player) {
        return "错误：尚未创建角色。\n";
    }
    return m_player->DisplayStatus().str();
}

// ───────────────────────────────────────────────────────────────────────────────
// 背包
// ───────────────────────────────────────────────────────────────────────────────

std::string GameEngine::EnterBackpack() {
    if (!m_player) return "错误：尚未创建角色。\n";
    m_state = GameState::Backpack;
    return GetBackpackText();
}

std::string GameEngine::GetBackpackText() const {
    if (!m_player) return "错误：尚未创建角色。\n";
    std::stringstream ss;
    ss << m_player->GetBackpack().GetBackpackInfo();
    ss << "\n操作: [u] 使用物品  [d] 丢弃物品  [0] 返回主菜单\n";
    return ss.str();
}

std::string GameEngine::UseBackpackItem(int index) {
    if (!m_player) return "错误：尚未创建角色。\n";
    try {
        std::string result = m_player->GetBackpack().UseItem(index, *m_player);
        // 使用后刷新展示
        result += "\n" + GetBackpackText();
        return result;
    } catch (const std::exception& e) {
        return std::string("使用失败：") + e.what() + "\n";
    }
}

std::string GameEngine::RemoveBackpackItem(int index) {
    if (!m_player) return "错误：尚未创建角色。\n";
    try {
        std::string result = m_player->GetBackpack().RemoveItem(index);
        result += "\n" + GetBackpackText();
        return result;
    } catch (const std::exception& e) {
        return std::string("丢弃失败：") + e.what() + "\n";
    }
}

std::string GameEngine::LeaveBackpack() {
    m_state = GameState::InGame;
    return "已关闭背包。\n";
}

// ─────────────────────────────────────────────────────────────────────────────
// 商店
// ─────────────────────────────────────────────────────────────────────────────

std::string GameEngine::EnterShop() {
    if (!m_player) {
        return "错误：尚未创建角色。\n";
    }
    m_state = GameState::Shop;
    return m_shop.DisplayShop().str();
}

std::string GameEngine::BuyItem(int itemIndex) {
    if (!m_player) {
        return "错误：尚未创建角色。\n";
    }
    try {
        return m_shop.BuyItem(m_player, itemIndex).str();
    } catch (const std::exception& e) {
        return std::string("购买失败：") + e.what() + "\n";
    }
}

std::string GameEngine::LeaveShop() {
    m_state = GameState::InGame;
    return "已离开商店。\n";
}

// ─────────────────────────────────────────────────────────────────────────────
// 存档
// ─────────────────────────────────────────────────────────────────────────────

std::string GameEngine::SaveGame() {
    // TODO: 接入 SaveSys 实现后替换占位文本
    return "存档保存中…（功能开发中）\n";
}

std::string GameEngine::LoadGame() {
    // TODO: 接入 SaveSys 实现后替换占位文本
    // 成功读档后应设置 m_player 并将 m_state 切换到 InGame
    return "读取存档中…（功能开发中）\n";
}

// ─────────────────────────────────────────────────────────────────────────────
// 退出
// ─────────────────────────────────────────────────────────────────────────────

std::string GameEngine::QuitGame() {
    m_state = GameState::GameOver;
    return "游戏结束，感谢游玩！\n";
}
