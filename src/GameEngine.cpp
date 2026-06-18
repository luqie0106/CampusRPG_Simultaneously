#include "../include/GameEngine.h"
#include "../include/Exceptions.h"

// ─────────────────────────────────────────────────────────────────────────────
// 构造 / 析构
// ─────────────────────────────────────────────────────────────────────────────

GameEngine::GameEngine()
    : m_state(GameState::Uninitialized), m_player(nullptr), m_inBattle(false) {}

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

std::string GameEngine::GetClassSelectionText() const {
    std::stringstream ss;
    ss << "=== 职业选择 ===\n";
    ss << "1. 体育生 (Athlete)\n";
    ss << "   HP: 35  攻击: 16  防御: 3  闪避: 5%  破韧: 5  金币: 150\n";
    ss << "   特色：高血量高输出，适合正面硬刚；破韧值低，难以打瘫痪 Boss。\n\n";
    ss << "2. 学霸 (Nerd)\n";
    ss << "   HP: 14  攻击: 8   防御: 10  闪避: 25%  破韧: 12  金币: 250\n";
    ss << "   特色：高防御高闪避，持久耐打；血量脆，需善用道具。\n\n";
    ss << "3. 普通学生 (Steve)\n";
    ss << "   HP: 20  攻击: 10  防御: 5   闪避: 12%  破韧: 10  金币: 200\n";
    ss << "   特色：全能均衡，上手容易，适合初次游玩。\n";
    return ss.str();
}

std::string GameEngine::CreatePlayer(const std::string& name, int classType) {
    if (name.empty()) {
        return "错误：角色名称不能为空。\n";
    }

    std::string className;
    switch (classType) {
        case 1:
            m_player  = std::make_shared<Athlete>(name);
            className = "体育生";
            break;
        case 2:
            m_player  = std::make_shared<Nerd>(name);
            className = "学霸";
            break;
        case 3:
            m_player  = std::make_shared<Steve>(name);
            className = "普通学生";
            break;
        default:
            return "错误：无效的职业编号（1=体育生, 2=学霸, 3=普通学生）。\n";
    }

    m_state = GameState::InGame;
    std::stringstream ss;
    ss << "角色「" << name << "」（" << className << "）创建成功！冒险开始！\n";
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

std::string GameEngine::SellBackpackItem(int backpackIndex) {
    if (!m_player) {
        return "错误：尚未创建角色。\n";
    }
    // 出售行为在商店界面或背包界面均合法；
    // 若需严格限制只能在 Shop 状态下出售，取消下方注释即可。
    // if (m_state != GameState::Shop) {
    //     return "错误：只能在商店界面出售物品。\n";
    // }

    Backpack& bp = m_player->GetBackpack();

    // 先读取名称和价值（移除后指针失效）
    int rawValue = bp.GetItemValue(backpackIndex);
    if (rawValue < 0) {
        // GetItemValue 返回 -1 表示索引无效
        return "错误：无效的物品位置（共 " +
               std::to_string(bp.GetSize()) + " 件物品）。\n";
    }

    std::string itemName = bp.GetItemName(backpackIndex);

    // 出售价格 = 原价 × 50%，向下取整，最低 1 金币
    int sellPrice = std::max(1, rawValue / 2);

    // 移除物品（RemoveItem 内部会再次校验索引，失败时抛异常）
    try {
        bp.RemoveItem(backpackIndex);
    } catch (const std::exception& e) {
        return std::string("出售失败：") + e.what() + "\n";
    }

    // 将金币加到玩家身上
    m_player->AddGold(sellPrice);

    std::stringstream ss;
    ss << "成功售出【" << itemName << "】，"
       << "获得 " << sellPrice << " 金币"
       << "（原价 " << rawValue << " × 50%）。\n";
    ss << "当前持有金币：" << m_player->GetGold() << "\n";
    return ss.str();
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

// ─────────────────────────────────────────────────────────────────────────────
// 战斗系统
// ─────────────────────────────────────────────────────────────────────────────

std::string GameEngine::StartBattle(Enemy targetEnemy) {
    if (!m_player) {
        return "错误：尚未创建角色。\n";
    }
    if (m_state != GameState::InGame) {
        return "错误：只能从游戏内（InGame）状态进入战斗。\n";
    }

    m_currentEnemy = targetEnemy;   // 按值保存敌人副本
    m_inBattle     = true;
    m_state        = GameState::Battle;

    std::stringstream ss;
    ss << "⚔️  战斗开始！\n";
    ss << m_currentEnemy->DisplayStatus();
    ss << "\n你的行动：[攻击] [使用物品] [P] 逃跑\n";
    return ss.str();
}

// ── 内部辅助：敌人回合 ────────────────────────────────────────────
std::string GameEngine::_EnemyTurn() {
    // 不应在此状态以外调用，但做防御性检查
    if (!m_currentEnemy || !m_player) return "";

    std::stringstream ss;

    // 食物 buff 回合推进（每回合敌人出招前计一次）
    if (m_player) m_player->TickFoodBuff();

    // 每回合开始先 tick 瘫痪计时
    bool stillStaggered = m_currentEnemy->TickStagger();

    if (stillStaggered || m_currentEnemy->IsStaggered()) {
        // 瘫痪中：敌人跳过本回合
        ss << m_currentEnemy->DisplayStatus().substr(
               m_currentEnemy->DisplayStatus().find("名字：")); // 截取名字行作前缀
        ss << "💥 敌人处于瘫痪状态，跳过本回合！\n";
    } else {
        // 敌人正常攻击
        ss << "\n【敌人回合】\n";
        ss << m_currentEnemy->Attack(*m_player);

        // 检查玩家是否死亡
        if (m_player->GetHealth() <= 0) {
            m_inBattle     = false;
            m_currentEnemy = std::nullopt;
            m_state        = GameState::GameOver;
            ss << "\n💀 你被击败了！游戏结束。\n";
        } else {
            ss << "你的生命值：" << m_player->GetHealth()
               << "/" << m_player->GetMaxHealth() << "\n";
        }
    }
    return ss.str();
}

// ── 内部辅助：结算胜利 ────────────────────────────────────────────
std::string GameEngine::_SettleVictory() {
    if (!m_currentEnemy || !m_player) return "";

    std::stringstream ss;
    int gainExp  = m_currentEnemy->GetExp();
    int gainGold = m_currentEnemy->GetGold();

    ss << "\n🏆 战斗胜利！\n";
    ss << "获得经验值：+" << gainExp << "\n";
    ss << "获得金币：  +" << gainGold << "\n";

    // TODO: 接入掉落物系统后在此随机掉落物品

    bool leveled = m_player->AddExp(gainExp);
    m_player->AddGold(gainGold);

    if (leveled) {
        ss << "🌟 恭喜升级！当前等级：" << m_player->GetLevel() << "\n";
    }

    m_inBattle     = false;
    m_currentEnemy = std::nullopt;
    m_state        = GameState::InGame;
    ss << "\n战斗结束，返回游戏主菜单。\n";
    return ss.str();
}

// ── 公开 API ──────────────────────────────────────────────────────

std::string GameEngine::BattlePlayerAttack() {
    if (m_state != GameState::Battle || !m_currentEnemy || !m_player) {
        return "错误：当前不在战斗状态。\n";
    }

    std::stringstream ss;
    ss << "【玩家回合 - 攻击】\n";

    // 计算伤害：玩家攻击 - 敌人防御，最低 1 点
    int rawAtk    = m_player->GetAttack();
    int enemyDef  = m_currentEnemy->GetDefense();
    int damage    = std::max(1, rawAtk - enemyDef);

    m_currentEnemy->TakeDamage(damage);

    // 玩家攻击同时对 Boss 造成破韧伤害（固定 1 点，可后续扩展为武器属性）
    m_currentEnemy->TakeToughnessDamage(1);

    ss << m_player->GetName() << " 攻击了敌人，造成 " << damage << " 点伤害！\n";
    ss << "敌人剩余生命值：" << m_currentEnemy->GetHealth() << "\n";

    // 检查敌人是否死亡
    if (m_currentEnemy->GetHealth() <= 0) {
        ss << _SettleVictory();
        return ss.str();
    }

    // 敌人存活，触发敌人回合
    ss << _EnemyTurn();
    return ss.str();
}

std::string GameEngine::BattlePlayerUseItem(int itemIndex) {
    if (m_state != GameState::Battle || !m_currentEnemy || !m_player) {
        return "错误：当前不在战斗状态。\n";
    }

    std::stringstream ss;
    ss << "【玩家回合 - 使用物品】\n";

    try {
        std::string useResult = m_player->GetBackpack().UseItem(itemIndex, *m_player);
        ss << useResult << "\n";
    } catch (const std::exception& e) {
        // 使用失败（索引越界、不可用等），本回合仍消耗，接着触发敌人攻击
        ss << "使用物品失败：" << e.what() << "\n";
    }

    // 使用物品后，检查敌人是否已死（理论上不会，但保持一致性）
    if (m_currentEnemy->GetHealth() <= 0) {
        ss << _SettleVictory();
        return ss.str();
    }

    // 触发敌人回合
    ss << _EnemyTurn();
    return ss.str();
}

std::string GameEngine::BattleFlee() {
    if (m_state != GameState::Battle) {
        return "错误：当前不在战斗状态。\n";
    }

    m_inBattle     = false;
    m_currentEnemy = std::nullopt;
    m_state        = GameState::InGame;

    return "🏃 你选择了逃跑……战斗以失败告终。\n返回游戏主菜单。\n";
}

const Enemy* GameEngine::GetCurrentEnemy() const {
    if (m_state != GameState::Battle || !m_currentEnemy.has_value()) {
        return nullptr;
    }
    return &m_currentEnemy.value();
}

// ─────────────────────────────────────────────────────────────────────────────
// Qt MVC 纯数据接口实现
// ─────────────────────────────────────────────────────────────────────────────

// ── 玩家属性 ──────────────────────────────────────────────────────
int    GameEngine::GetPlayerHp()           const { return m_player ? m_player->GetHealth()           : 0; }
int    GameEngine::GetPlayerMaxHp()        const { return m_player ? m_player->GetMaxHealth()        : 0; }
int    GameEngine::GetPlayerAttack()       const { return m_player ? m_player->GetAttack()           : 0; }
int    GameEngine::GetPlayerDefense()      const { return m_player ? m_player->GetDefense()          : 0; }
int    GameEngine::GetPlayerGold()         const { return m_player ? m_player->GetGold()             : 0; }
int    GameEngine::GetPlayerLevel()        const { return m_player ? m_player->GetLevel()            : 0; }
int    GameEngine::GetPlayerExp()          const { return m_player ? m_player->GetExp()              : 0; }
int    GameEngine::GetPlayerExpToNext()    const { return m_player ? m_player->ExpToNextLevel()      : 0; }
double GameEngine::GetPlayerDodgeRate()    const { return m_player ? m_player->GetDodgeRate()        : 0.0; }
int    GameEngine::GetPlayerStaggerPoint() const { return m_player ? m_player->GetStaggerPoint()     : 0; }
int    GameEngine::GetPlayerFoodBuffAtk()  const { return m_player ? m_player->GetFoodBuffAtk()      : 0; }
int    GameEngine::GetPlayerFoodBuffRounds() const { return m_player ? m_player->GetFoodBuffRoundsLeft() : 0; }

// ── 背包 ────────────────────────────────────────────────────────
const std::vector<std::unique_ptr<Item>>& GameEngine::GetBackpackItems() const {
    // 返回一个静态空 vector，防止在 m_player为空时返回悬空引用
    static const std::vector<std::unique_ptr<Item>> empty;
    if (!m_player) return empty;
    return m_player->GetBackpack().GetItems();
}

// ── 商店 ────────────────────────────────────────────────────────
const std::vector<ShopItem>& GameEngine::GetShopItemList() const {
    return m_shop.GetShopItems();
}

// ── 战斗敌人 ────────────────────────────────────────────────────
int  GameEngine::GetBattleEnemyHp()          const { return m_currentEnemy ? m_currentEnemy->GetHealth()     : -1; }
int  GameEngine::GetBattleEnemyAtk()         const { return m_currentEnemy ? m_currentEnemy->GetAttack()     : -1; }
int  GameEngine::GetBattleEnemyDef()         const { return m_currentEnemy ? m_currentEnemy->GetDefense()    : -1; }
bool GameEngine::GetBattleEnemyIsStaggered() const { return m_currentEnemy ? m_currentEnemy->IsStaggered()   : false; }
std::string GameEngine::GetBattleEnemyName() const { return m_currentEnemy ? m_currentEnemy->GetName()       : ""; }

// ── 玩家职业 ────────────────────────────────────────────────────
CharacterClass GameEngine::GetPlayerClass()     const {
    return m_player ? m_player->GetClass() : CharacterClass::Student;
}
std::string GameEngine::GetPlayerClassName()    const {
    return m_player ? m_player->GetClassName() : "";
}
