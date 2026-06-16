#pragma once
#include "Common.h"
#include "Character.h"
#include "Shop.h"

// ─────────────────────────────────────────────
// 游戏阶段枚举
// UI 层可通过 GetState() 查询当前处于哪个界面，
// 并据此显示对应的 Qt 控件组合。
// ─────────────────────────────────────────────
enum class GameState {
    Uninitialized,  // 引擎尚未初始化
    MainMenu,       // 主菜单（新游戏 / 读档 / 退出）
    InGame,         // 游戏内主菜单（状态 / 背包 / 商店 / 保存）
    Backpack,       // 背包界面
    Shop,           // 商店界面
    GameOver        // 游戏结束
};

class GameEngine {
public:
    GameEngine();
    ~GameEngine();

    // ── 生命周期 ──────────────────────────────
    // 初始化引擎，返回初始化日志。调用后状态变为 MainMenu。
    std::string Init();

    // 查询当前游戏阶段，UI 据此切换界面
    GameState GetState() const;

    // 查询当前玩家（可能为 nullptr）
    std::shared_ptr<Character> GetPlayer() const;

    // ── 主菜单 API ────────────────────────────
    // 返回主菜单的展示文本（供 UI 渲染菜单内容）
    std::string GetMainMenuText() const;

    // 处理主菜单选项：1=新游戏  2=读档  3=退出
    std::string HandleMainMenuChoice(int choice);

    // ── 创建角色 API ──────────────────────────
    // 用给定名字创建新角色，成功后状态变为 InGame
    std::string CreatePlayer(const std::string& name);

    // ── 游戏内主菜单 API ──────────────────────
    // 返回游戏内主菜单的展示文本
    std::string GetInGameMenuText() const;

    // 处理游戏内选项：1=查看状态  2=背包  3=商店  4=保存并退出
    std::string HandleInGameMenuChoice(int choice);

    // ── 角色状态 API ──────────────────────────
    std::string GetPlayerStatus() const;

    // ── 背包 API ────────────────────────────
    // 进入背包界面，返回背包内容并切换状态到 Backpack
    std::string EnterBackpack();

    // 使用物品（index 为 1-based）并返回结果文本
    std::string UseBackpackItem(int index);

    // 丢弃物品（index 为 1-based）并返回结果文本
    std::string RemoveBackpackItem(int index);

    // 离开背包，状态回到 InGame
    std::string LeaveBackpack();

    // 返回当前背包展示内容（不切换状态）
    std::string GetBackpackText() const;

    // ── 商店 API ──────────────────────────────
    // 返回商店展示文本，并将状态切换到 Shop
    std::string EnterShop();

    // 购买商品（0-based 索引），返回购买结果文本
    std::string BuyItem(int itemIndex);

    // 离开商店，状态回到 InGame
    std::string LeaveShop();

    // ── 存档 API ──────────────────────────────
    std::string SaveGame();
    std::string LoadGame();

    // ── 退出 API ──────────────────────────────
    // 将状态设为 GameOver
    std::string QuitGame();

private:
    GameState                   m_state;
    std::shared_ptr<Character>  m_player;
    Shop                        m_shop;
};
