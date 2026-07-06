#include "Common.h"

// main.cpp — 控制台驱动层（Console UI）
// 未来接入 Qt 后，此文件可直接替换为 Qt 的 main.cpp，
// GameEngine 本身无需任何改动。
#include "include/GameEngine.h"
// ──────────────────────────────────────────────────────────────
// SafeInputInt — 安全整数输入辅助函数
//   • 遇到非法字符（字母、特殊符号）时清空输入流并重新提示
//   • 防止 std::cin 进入失败状态导致的死循环崩溃
// ──────────────────────────────────────────────────────────────
int SafeInputInt(const std::string& prompt) {
    int value;
    while (true) {
        std::cout << prompt;
        if (std::cin >> value) {
            // 合法整数：清除行尾多余内容后返回
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return value;
        }
        // 非法输入：清除错误标志与缓冲区，并提示重新输入
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "  [!] 请输入数字，不支持字母或特殊字符，请重试。\n";
    }
}

#if 0
int main() {
    GameEngine engine;
    std::cout << engine.Init();

    // ── 主菜单阶段 ──────────────────────────────────────────────
    while (engine.GetState() == GameState::MainMenu) {
        std::cout << engine.GetMainMenuText();

        int choice = SafeInputInt("请输入选择: ");
        std::string result = engine.HandleMainMenuChoice(choice);
        std::cout << result;

        // 选了「新游戏」(choice == 1) 时，引擎仍在 MainMenu，
        // 需要依次：① 输入名字  ② 选择职业  ③ CreatePlayer
        if (engine.GetState() == GameState::MainMenu && choice == 1) {
            // ① 输入角色名称
            std::cout << "角色名称: ";
            std::string name;
            std::cin >> name;
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

            // ② 展示职业介绍并让玩家选择
            std::cout << engine.GetClassSelectionText();
            int classType = SafeInputInt("请选择职业 (1/2/3): ");
            // 校验范围
            while (classType < 1 || classType > 3) {
                std::cout << "  [!] 请输入 1、2 或 3。\n";
                classType = SafeInputInt("请选择职业 (1/2/3): ");
            }

            // ③ 创建角色
            std::cout << engine.CreatePlayer(name, classType);
        }
    }

    // ── 游戏内阶段 ──────────────────────────────────────────────
    // 包含所有可能从 InGame 跳转到的子状态，以及将来的地图探索状态
    while (engine.GetState() == GameState::InGame    ||
           engine.GetState() == GameState::Backpack   ||
           engine.GetState() == GameState::Shop       ||
           engine.GetState() == GameState::Battle) {

        // ── 游戏内主菜单 ─────────────────────────────
        if (engine.GetState() == GameState::InGame) {
            std::cout << engine.GetInGameMenuText();
            int choice = SafeInputInt("请选择操作: ");
            std::cout << engine.HandleInGameMenuChoice(choice);

        // ── 背包界面 ──────────────────────────────────
        } else if (engine.GetState() == GameState::Backpack) {
            std::cout << "操作 (u=使用, d=丢弃, 0=返回): ";
            char op;
            std::cin >> op;
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

            if (op == '0') {
                std::cout << engine.LeaveBackpack();
            } else if (op == 'u' || op == 'U') {
                int idx = SafeInputInt("输入物品序号: ");
                std::cout << engine.UseBackpackItem(idx);
            } else if (op == 'd' || op == 'D') {
                int idx = SafeInputInt("输入物品序号: ");
                std::cout << engine.RemoveBackpackItem(idx);
            } else {
                std::cout << "无效操作，请输入 u / d / 0。\n";
                std::cout << engine.GetBackpackText();
            }

        // ── 商店界面 ──────────────────────────────────
        // 支持购买（b <编号>）、出售（s <背包序号>）、离开（0）
        // 白天 = 普通商店；夜晚 = 黑市（通过 IsNight() 区分）
        } else if (engine.GetState() == GameState::Shop) {
            bool inBlackMarket = engine.IsNight() && engine.GetBlackMarketItems().size() > 0;

            // ── Bug3 修复：昼夜穿模保护 ──────────────────────────────────
            // 如果当前是普通商店，但时间已进入夜晚（时钟后台推进），强制驱逐
            if (!inBlackMarket && engine.IsNight()) {
                std::cout << "\n铛铛铛... 小卖部阿姨关灯了，把你赶了出去。\n";
                std::cout << engine.LeaveShop();
                continue;   // 回到主循环顶部，重新判断当前状态
            }

            if (inBlackMarket) {
                std::cout << "\n🌙 黑市操作: [0] 离开  [b <编号>] 购买\n";
            } else {
                std::cout << "\n商店操作: [0] 离开  [b <编号>] 购买  [s <背包序号>] 出售\n";
            }
            std::cout << "请输入指令: ";

            char op;
            std::cin >> op;
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

            if (op == '0') {
                if (inBlackMarket) {
                    std::cout << engine.LeaveBlackMarket();
                } else {
                    std::cout << engine.LeaveShop();
                }
            } else if (op == 'b' || op == 'B') {
                int idx = SafeInputInt("输入商品编号: ");
                if (inBlackMarket) {
                    std::cout << engine.BuyBlackMarketItem(idx); // 传 1-based
                } else {
                    std::cout << engine.BuyItem(idx); // 传 1-based
                }
            } else if ((op == 's' || op == 'S') && !inBlackMarket) {
                std::cout << engine.GetBackpackText();
                int idx = SafeInputInt("输入背包物品序号出售: ");
                std::cout << engine.SellBackpackItem(idx); // 1-based
            } else {
                if (inBlackMarket) {
                    std::cout << "无效指令，请输入 0 / b <编号>。\n";
                } else {
                    std::cout << "无效指令，请输入 0 / b <编号> / s <背包序号>。\n";
                }
            }

        // ── 战斗界面 ──────────────────────────────────
        } else if (engine.GetState() == GameState::Battle) {
            // 展示当前敌人信息
            const Enemy* enemy = engine.GetCurrentEnemy();
            if (enemy) {
                std::cout << "\n【战斗中】对手: " << engine.GetBattleEnemyName()
                          << "  HP: " << engine.GetBattleEnemyHp()
                          << "  ATK: " << engine.GetBattleEnemyAtk()
                          << "  DEF: " << engine.GetBattleEnemyDef();
                if (engine.GetBattleEnemyIsStaggered()) std::cout << "  [瘫痪]";
                std::cout << "\n";
            }
            std::cout << "我方 HP: " << engine.GetPlayerHp()
                      << " / " << engine.GetPlayerMaxHp() << "\n";
            std::cout << "操作: [1] 攻击  [2] 使用道具(消耗行动)  [3] 使用道具(先手)  [P] 逃跑\n";
            std::cout << "请输入: ";

            char op;
            std::cin >> op;
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

            if (op == '1') {
                // 攻击，触发敌人反击
                std::cout << engine.BattlePlayerAttack();

            } else if (op == '2' || op == '2') {
                // 使用道具（消耗行动，触发敌人反击）
                std::cout << engine.GetBackpackText();
                int idx = SafeInputInt("选择要使用的物品序号: ");
                std::cout << engine.BattlePlayerUseItem(idx);

            } else if (op == '3') {
                // 先手使用道具（不消耗行动机会，不触发敌人反击）
                std::cout << engine.GetBackpackText();
                int idx = SafeInputInt("选择先手使用的物品序号: ");
                std::cout << engine.BattleUseItemBeforeAction(idx);
                // 先手使用后仍需继续当前回合操作，不切换状态

            } else if (op == 'p' || op == 'P') {
                // 逃跑
                std::cout << engine.BattleFlee();

            } else {
                std::cout << "无效输入，请输入 1 / 2 / 3 / P。\n";
            }
        }
    }

    // ── 游戏结束 ─────────────────────────────────────────────────
    if (engine.GetState() == GameState::GameOver) {
        std::cout << "\n================ 游戏结束 ================\n";
    }

    return 0;
}
#endif

// ==========================================================
// Qt GUI 时代的入口：替换掉原先的黑框框控制台循环
// ==========================================================
#include "mainwindow.h"
int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
