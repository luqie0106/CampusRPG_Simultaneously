// main.cpp — 控制台驱动层（Console UI）
// 未来接入 Qt 后，此文件可直接替换为 Qt 的 main.cpp，
// GameEngine 本身无需任何改动。
#include "include/GameEngine.h"
#include <iostream>

int main() {
    GameEngine engine;
    std::cout << engine.Init();

    // ── 主菜单阶段 ──────────────────────────────────────────
    while (engine.GetState() == GameState::MainMenu) {
        std::cout << engine.GetMainMenuText();
        std::cout << "请输入选择: ";
        int choice;
        std::cin >> choice;

        std::string result = engine.HandleMainMenuChoice(choice);
        std::cout << result;

        // 选了"新游戏"时，引擎还在 MainMenu 状态，等待 CreatePlayer 被调用
        if (engine.GetState() == GameState::MainMenu && choice == 1) {
            std::cout << "角色名称: ";
            std::string name;
            std::cin >> name;
            std::cout << engine.CreatePlayer(name);
        }
    }

    // ── 游戏内阶段 ──────────────────────────────────────────
    while (engine.GetState() == GameState::InGame   ||
           engine.GetState() == GameState::Backpack  ||
           engine.GetState() == GameState::Shop) {

        // ── 游戏内主菜单 ─────────────────────────────
        if (engine.GetState() == GameState::InGame) {
            std::cout << engine.GetInGameMenuText();
            std::cout << "请选择操作: ";
            int choice;
            std::cin >> choice;
            std::cout << engine.HandleInGameMenuChoice(choice);

        // ── 背包界面 ─────────────────────────────────
        } else if (engine.GetState() == GameState::Backpack) {
            // 显示已在 EnterBackpack / UseBackpackItem / RemoveBackpackItem 末尾刷新
            std::cout << "操作 (u=使用, d=丢弃, 0=返回): ";

            char op;
            std::cin >> op;

            if (op == '0') {
                std::cout << engine.LeaveBackpack();
            } else if (op == 'u' || op == 'U') {
                std::cout << "输入物品序号: ";
                int idx;
                std::cin >> idx;
                std::cout << engine.UseBackpackItem(idx);
            } else if (op == 'd' || op == 'D') {
                std::cout << "输入物品序号: ";
                int idx;
                std::cin >> idx;
                std::cout << engine.RemoveBackpackItem(idx);
            } else {
                std::cout << "无效操作，请输入 u / d / 0。\n";
                std::cout << engine.GetBackpackText();
            }

        // ── 商店界面 ─────────────────────────────────
        } else if (engine.GetState() == GameState::Shop) {
            std::cout << "输入商品编号购买，输入 0 离开商店: ";
            int idx;
            std::cin >> idx;
            if (idx == 0) {
                std::cout << engine.LeaveShop();
            } else {
                std::cout << engine.BuyItem(idx - 1); // 转为 0-based
            }
        }
    }

    return 0;
}
