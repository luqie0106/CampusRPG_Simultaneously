#include "Common.h"

#include "../include/GameEngine.h"
#include "../include/Exceptions.h"
#include "../include/RNG.h"
#include "../include/SaveSys.h"

// ─────────────────────────────────────────────────────────────────────────────
// 构造 / 析构
// ─────────────────────────────────────────────────────────────────────────────

GameEngine::GameEngine()
    : m_state(GameState::Uninitialized), m_player(nullptr), m_inBattle(false) {}

GameEngine::~GameEngine() {
    m_clock.Stop();  // 确保析构时后台时钟线程已安全退出
}

// ─────────────────────────────────────────────────────────────────────────────
// 生命周期
// ─────────────────────────────────────────────────────────────────────────────

std::string GameEngine::Init() {
    m_state = GameState::MainMenu;
    RNG::Init(); // 程序启动时初始化全局 mt19937 种子

    m_shop.SetEngine(this);
    m_taskManager.InitTasks();

    // ── 启动游戏内时钟并注册昼夜回调 ────────────────────
    m_clock.SetOnDayToNight([this]() { _OnDayToNight(); });
    m_clock.SetOnNightToDay([this]() { _OnNightToDay(); });
    m_clock.Start();

    return "游戏引擎初始化完成。\n当前时间：" + GetGameTimeText() + "\n";
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
    ss << "=== 主菜单 " << GetGameTimeText() << " ===\n";
    ss << "1. 查看状态\n";
    ss << "2. 查看背包\n";
    ss << "3. 商店";
    if (IsNight()) ss << " (🌙夜晚小卖部闭店中)";
    ss << "\n";
    if (IsNight() && m_blackMarketSpawned) {
        ss << "4. 🌙 黑市（夜间专属）\n";
        ss << "5. 保存并退出\n";
    } else {
        ss << "4. 保存并退出\n";
    }
    return ss.str();
}

std::string GameEngine::HandleInGameMenuChoice(int choice) {
    if (!m_player) {
        return "错误：尚未创建角色。\n";
    }
    // 如果夜晚且黑市已生成，菜单层级重排：1-3 不变，4=黑市，5=退出
    bool nightBlackMarketAvail = IsNight() && m_blackMarketSpawned;
    switch (choice) {
        case 1:
            return GetPlayerStatus();
        case 2:
            return EnterBackpack();
        case 3:
            return EnterShop();
        case 4: {
            if (nightBlackMarketAvail) {
                return EnterBlackMarket();
            }
            // 白天：4 = 保存并退出
            std::string saveResult = SaveGame();
            QuitGame();
            return saveResult + "已退出游戏。\n";
        }
        case 5: {
            if (nightBlackMarketAvail) {
                std::string saveResult = SaveGame();
                QuitGame();
                return saveResult + "已退出游戏。\n";
            }
            return "无效的选择！\n";
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
        return result;
    } catch (const std::exception& e) {
        return std::string("使用失败：") + e.what() + "\n";
    }
}

std::string GameEngine::RemoveBackpackItem(int index) {
    if (!m_player) return "错误：尚未创建角色。\n";
    try {
        std::string result = m_player->GetBackpack().RemoveItem(index);
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
    // 夜晚门禁：22:00–06:00 拒绝进入普通商店
    if (IsNight()) {
        return "🌙 夜深了，小卖部大门紧闭……神秘黑市也许在地图某处游荡！\n";
    }
    m_state = GameState::Shop;
    return m_shop.DisplayShop(m_player).str();
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
    if (!m_player) return "错误：尚未创建角色，无法保存。\n";
    try {
        SaveSys saver;
        saver.initDatabase();
        saver.savePlayer(*m_player, m_worldMap.GetPlayerPos(), m_clock.GetTime(), m_taskManager.SerializeTasks());
        return "存档成功！(" + m_player->GetName() + ")\n";
    } catch (const std::exception& e) {
        return std::string("存档失败：") + e.what() + "\n";
    }
}

std::string GameEngine::LoadGame() {
    try {
        SaveSys saver;
        saver.initDatabase();

        std::shared_ptr<Character> loadedPlayer;
        GamePoint pos;
        GameTime time;
        std::string tasksJson;
        if (saver.loadLatestSave(loadedPlayer, pos, time, tasksJson)) {
            m_taskManager.InitTasks(); // 先重置所有任务
            m_taskManager.DeserializeTasks(tasksJson); // 再注入存档状态
            m_player = loadedPlayer;
            m_worldMap.SetPlayerPos(pos);
            m_clock.SetTime(time);
            m_state = GameState::InGame;
            return "读档成功！欢迎回来，" + m_player->GetName() + "。\n";
        } else {
            return "没有找到任何存档记录。\n";
        }
    } catch (const std::exception& e) {
        return std::string("读档失败：") + e.what() + "\n";
    }
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

    // 玩家回合开始：先处理玩家身上的状态效果（凋零/中毒/生命回复等）
    std::string effectText = m_player->TickStatusEffects();
    if (!effectText.empty()) {
        ss << "\n【回合开始 - 状态效果】\n" << effectText;
        // 检查玩家是否因状态效果「死亡」→ 送往校医务室复活
        if (m_player->GetHealth() <= 0) {
            m_inBattle     = false;
            m_currentEnemy = std::nullopt;
            m_state        = GameState::InGame;   // 返回游戏内，不进入 GameOver
            // 复活：血量回满，清除负面状态
            m_player->HealHp(m_player->GetMaxHealth());
            m_player->ClearNegativeEffects();
            ResetPlayerToSpawn();   // ← Bug2 修复：重置坐标至出生点(0,0)，防止无限战斗
            ss << "\n💀 状态效果导致你倒下！眼前一黑...\n"
               << "🏥 你在校医务室的病床上醒来，并且恢复了全部状态！\n";
            return ss.str();
        }
    }

    // 食物 buff 回合推进（每回合敌人出招前计一次）
    if (m_player) m_player->TickFoodBuff();

    // ── Bug3 修复：先用 IsStaggered() 决定本回合是否跳过攻击 ──────────
    // TickStagger() 统一移到回合末尾执行，避免 Off-by-one：
    // 若在回合开始先 Tick，瘫痪1回合会立即归零并解除，Boss 紧接着发起攻击。
    if (m_currentEnemy->IsStaggered()) {
        // 瘫痪中：敌人跳过本回合
        ss << m_currentEnemy->DisplayStatus().substr(
               m_currentEnemy->DisplayStatus().find("名字：")); // 截取名字行作前缀
        ss << "💥 敌人处于瘫痪状态，跳过本回合！\n";
    } else {
        // 敌人正常攻击
        ss << "\n【敌人回合】\n";
        ss << m_currentEnemy->Attack(*m_player);

        // 检查玩家是否「死亡」→ 送往校医务室复活
        if (m_player->GetHealth() <= 0) {
            m_inBattle     = false;
            m_currentEnemy = std::nullopt;
            m_state        = GameState::InGame;   // 返回游戏内，不进入 GameOver
            // 复活：血量回满，清除负面状态（等级/经验/金币/背包全部保留）
            m_player->HealHp(m_player->GetMaxHealth());
            m_player->ClearNegativeEffects();
            ResetPlayerToSpawn();   // ← Bug2 修复：重置坐标至出生点，防止无限战斗
            ss << "\n💀 你被击败了！眼前一黑...\n"
               << "🏥 你在校医务室的病床上醒来，并且恢复了全部状态！\n";
            return ss.str();
        } else {
            ss << "你的生命值：" << m_player->GetHealth()
               << "/" << m_player->GetMaxHealth() << "\n";
        }
    }

    // 回合末尾统一递减瘫痪计时（无论本回合是否攻击）
    m_currentEnemy->TickStagger();

    return ss.str();
}

// ── 内部辅助：结算胜利 ────────────────────────────────────────────
std::string GameEngine::_SettleVictory() {
    if (!m_currentEnemy || !m_player) return "";

    std::stringstream ss;
    std::string enemyName = m_currentEnemy->GetName(); // 先记录名字，后面 optional 清空后就拿不到了
    int gainExp  = m_currentEnemy->GetExp();
    int gainGold = m_currentEnemy->GetGold();

    ss << "\n🏆 战斗胜利！\n";
    ss << "获得经验值：+" << gainExp << "\n";
    ss << "获得金币：  +" << gainGold << "\n";

    // ── 通知任务系统：怪物已被击败（任务5 修复：之前缺少此调用导致杀怪任务永远无法完成）
    m_taskManager.OnEnemyKilled(enemyName);

    bool leveled = m_player->AddExp(gainExp);
    m_player->AddGold(gainGold);

    // Boss 掉落物处理
    if (m_currentEnemy->GetDropItem()) {
        int dropId = GenerateItemId();
        GamePoint pPos = GetPlayerPos();
        InteractableInfo dropInfo = InteractableInfo::MakeGroundItem(
            dropId, pPos, m_currentEnemy->GetDropItem()->getName(), m_currentEnemy->GetDropItem());
        AddMapInteractable(dropInfo);
        ss << "🎁 掉落物：【" << m_currentEnemy->GetDropItem()->getName() << "】落在你脚下了！\n";
    }

    if (leveled) {
        ss << "🌟 恭喜升级！当前等级：" << m_player->GetLevel() << "\n";
    }

    m_inBattle     = false;
    m_currentEnemy = std::nullopt;
    m_player->ClearNegativeEffects();
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

    // 显示当前状态效果（如果有）
    std::string statusText = m_player->GetStatusEffectText();
    if (statusText != "无异常状态\n") {
        ss << "玩家当前状态：" << statusText;
    }

    // 计算伤害：玩家攻击 - 敌人防御，最低 1 点
    int rawAtk    = m_player->GetAttack(); // 已内含 Weakness 惩罚
    int enemyDef  = m_currentEnemy->GetDefense();
    int damage    = std::max(1, rawAtk - enemyDef);

    // Slow 状态：伤害减半（一次性）
    if (m_player->HasSlow()) {
        damage = std::max(1, damage / 2);
        ss << "🐢 《迟缓》状态导致这次攻击伤害已减半！\n";
        m_player->ClearSlow();
    }

    m_currentEnemy->TakeDamage(damage);

    // 玩家攻击同时对 Boss 造成破韧伤害
    // 基础破韧值 = 玩家职业破韧点；装备武器时叠加武器的 stagger_bonus
    {
        double staggerDmg = m_player->GetStaggerPoint();
        auto weapon = m_player->GetEquipmentAt(EquipSlot::Weapon);
        if (weapon) staggerDmg += weapon->GetStaggerBonus();
        m_currentEnemy->TakeToughnessDamage(staggerDmg);
    }

    ss << m_player->GetName() << " 攻击了敌人，造成 " << damage << " 点伤害！\n";
    ss << "敌人剩余生命值：" << m_currentEnemy->GetHealth() << "\n";

    // ── Bug2 修复：扣减武器耐久度 ─────────────────────────────────────
    auto weapon = m_player->GetEquipmentAt(EquipSlot::Weapon);
    if (weapon) {
        weapon->ReduceDurability(1);
        if (weapon->GetDurability() <= 0) {
            ss << "⚠️ 【" << weapon->getName() << "】已损坏！\n";
            m_player->UnequipItem(EquipSlot::Weapon);
        }
    }

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

// 玩家回合前使用物品（不消耗行动次数，不触发敌人回合）
std::string GameEngine::BattleUseItemBeforeAction(int itemIndex) {
    if (m_state != GameState::Battle || !m_player) {
        return "错误：当前不在战斗状态。\n";
    }

    std::stringstream ss;
    ss << "【战斗前使用物品】\n";

    try {
        std::string useResult = m_player->GetBackpack().UseItem(itemIndex, *m_player);
        ss << useResult << "\n";
    } catch (const std::exception& e) {
        ss << "使用失败：" << e.what() << "\n";
    }

    // 显示当前玩家状态，便于 UI 刷新
    ss << "当前生命值：" << m_player->GetHealth()
       << "/" << m_player->GetMaxHealth() << "\n";
    ss << m_player->GetStatusEffectText();
    ss << "\n请选择行动：[攻击] [继续使用物品] [P] 逃跑\n";
    return ss.str();
}

std::string GameEngine::BattleFlee() {
    if (m_state != GameState::Battle) {
        return "错误：当前不在战斗状态。\n";
    }

    m_inBattle     = false;
    m_currentEnemy = std::nullopt;
    m_player->ClearNegativeEffects();
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
double GameEngine::GetPlayerStaggerPoint() const { return m_player ? m_player->GetStaggerPoint() : 0.0; }
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
const std::vector<ShopItem>& GameEngine::GetShopItemList() {
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

// ─────────────────────────────────────────────────────────────────────────────
// 地图探索接口实现
// ─────────────────────────────────────────────────────────────────────────────

// 【摄像机接口】返回玩家当前世界格子坐标
// Qt 以此为屏幕中心点，偏移渲染周围地图瓦片
GamePoint GameEngine::GetPlayerPos() const {
    return m_worldMap.GetPlayerPos();
}

// 【视野接口】返回以玩家为中心的 (2*halfRadius+1)^2 格地图信息
// 包含地形类型（0=空地/1=障碍）和该格是否有可交互实体
std::vector<MapTileInfo> GameEngine::GetVisibleMap(int halfRadius) const {
    return m_worldMap.GetVisibleMap(halfRadius);
}

// 【移动接口】仅在 InGame 状态下允许移动
// 内部调用 WorldMap::MovePlayer()，已含碰撞检测
bool GameEngine::MovePlayer(int dx, int dy) {
    if (m_state != GameState::InGame) {
        return false;   // 战斗中 / 商店中 / 背包中均不允许移动
    }
    return m_worldMap.MovePlayer(dx, dy);
}

// 【附近实体检测】主动交互前的轮询函数
// 返回与玩家曼哈顿距离 ≤ radius 的全部实体；Qt 据此决定是否弹出交互按钮
std::vector<InteractableInfo> GameEngine::CheckNearbyInteractables(int radius) const {
    auto list = m_worldMap.CheckNearbyInteractables(radius);
    if (!IsNight()) {
        auto it = std::remove_if(list.begin(), list.end(), [](const InteractableInfo& info) {
            return info.isNightOnly;
        });
        list.erase(it, list.end());
    }
    return list;
}

// 【交互执行】玩家主动点击确认后的核心入口
// 负责：查找目标实体 → 安全校验 → 驱动状态机切换
std::string GameEngine::ExecuteInteraction(InteractionType type, int targetId) {
    if (!m_player) {
        return "错误：尚未创建角色。\n";
    }
    if (m_state != GameState::InGame) {
        return "错误：只能在探索状态（InGame）下发起交互。\n";
    }

    const InteractableInfo* info = m_worldMap.GetInteractableById(targetId);
    if (!info) {
        return "错误：找不到目标实体（id=" + std::to_string(targetId) + "）。\n";
    }

    switch (type) {
        case InteractionType::StartBattle: {
            // 必须有绑定的敌人模板
            if (!info->enemyTemplate.has_value()) {
                // 智能兜底：如果实体名字匹配上了特定 Boss，强行注入模板并开战
                if (info->displayName == "校长") {
                    Enemy boss = Enemy::Principal();
                    m_worldMap.MarkInteractableDead(targetId, GetGameTime().GetTotalMinutes());
                    return StartBattle(boss);
                } else if (info->displayName == "教导主任") {
                    Enemy boss = Enemy::DeanOfStudents();
                    m_worldMap.MarkInteractableDead(targetId, GetGameTime().GetTotalMinutes());
                    return StartBattle(boss);
                } else if (info->displayName == "体育委员长") {
                    Enemy boss = Enemy::PECommittee();
                    m_worldMap.MarkInteractableDead(targetId, GetGameTime().GetTotalMinutes());
                    return StartBattle(boss);
                } else if (info->displayName == "宿管阿姨") {
                    Enemy boss = Enemy::DormGuard();
                    m_worldMap.MarkInteractableDead(targetId, GetGameTime().GetTotalMinutes());
                    return StartBattle(boss);
                }
                return "错误：该实体未绑定敌人模板，无法开始战斗。\n";
            }
            // 战斗一旦触发，将实体从地图移除（战斗结束前不再显示为可交互）
            // 若战斗胜利则永久移除；若逃跑则已消失（简化设计，可后续扩展为"重置"）
            Enemy enemyCopy = info->enemyTemplate.value();
            m_worldMap.MarkInteractableDead(targetId, GetGameTime().GetTotalMinutes());
            return StartBattle(enemyCopy);
        }

        case InteractionType::EnterShop: {
            // 夜晚门禁
            if (IsNight()) {
                return "🌙 夜深了，小卖部大门紧闭……\n";
            }
            return EnterShop();
        }

        case InteractionType::EnterBlackMarket: {
            return EnterBlackMarket();
        }

        case InteractionType::TalkToNPC: {
            return m_taskManager.GetNPCInteractionText(info->displayName, m_player.get());
        }

        case InteractionType::PickUpItem: {
            if (!info->itemData) {
                return "错误：该掉落物数据为空。\n";
            }
            try {
                // 放入背包
                m_player->GetBackpack().AddItem(info->itemData->Clone());
                std::string itemName = info->itemData->getName();
                m_worldMap.RemoveInteractable(targetId);
                return "【拾取】你捡起了 " + itemName + "。\n";
            } catch (const std::exception& e) {
                return std::string("拾取失败：") + e.what() + "\n";
            }
        }

        default:
            return "错误：未知的交互类型。\n";
    }
}

// 【出生点重置】新游戏开始 / 读档后调用
// CreatePlayer() 成功后，Qt 应调用此函数将玩家归位到地图出生坐标
void GameEngine::ResetPlayerToSpawn() {
    m_worldMap.ResetPlayerToSpawn();
}

// 【地图实体注册入口】由地图同学在地图初始化阶段调用
// 通过引擎统一注册，避免外部直接操作 WorldMap 内部状态
void GameEngine::AddMapInteractable(InteractableInfo info) {
    m_worldMap.AddInteractable(std::move(info));
}

int GameEngine::GenerateItemId() {
    return m_nextItemEntityId++;
}

// 【WorldMap 直接访问】供 QtMapLoader 在地图加载阶段使用
WorldMap& GameEngine::GetWorldMap() {
    return m_worldMap;
}

// ─────────────────────────────────────────────────────────────────────────────
// 游戏时间 API 实现
// ─────────────────────────────────────────────────────────────────────────────

GameTime GameEngine::GetGameTime() const {
    return m_clock.GetTime();  // 内部已加锁
}

bool GameEngine::IsNight() const {
    return m_clock.IsNight();  // 内部已加锁
}

std::string GameEngine::GetGameTimeText() const {
    GameTime t = GetGameTime();
    return t.ToString() + (t.IsNight() ? " 🌙" : " ☀️");
}

// ─────────────────────────────────────────────────────────────────────────────
// 黑市 API 实现
// ─────────────────────────────────────────────────────────────────────────────

std::string GameEngine::EnterBlackMarket() {
    if (!m_player) return "错误：尚未创建角色。\n";
    if (!IsNight()) {
        return "黑市只在夜间出现，天亮后商人便消失无踪……\n";
    }
    if (!m_blackMarketSpawned) {
        return "🌙 今晚黑市商人还未现身，稍后再来或在地图上寻找他吧。\n";
    }
    m_state = GameState::Shop;  // 复用 Shop 状态，UI 通过 GetBlackMarketItems 区分
    return m_blackMarket.DisplayBlackMarket().str();
}

std::string GameEngine::BuyBlackMarketItem(int itemIndex) {
    if (!m_player) return "错误：尚未创建角色。\n";
    try {
        return m_blackMarket.BuyItem(m_player, itemIndex).str();
    } catch (const std::exception& e) {
        return std::string("黑市购买失败：") + e.what() + "\n";
    }
}

std::string GameEngine::LeaveBlackMarket() {
    m_state = GameState::InGame;
    return "🌙 你离开了黑市，回到了黑暗的校园。\n";
}

const std::vector<BlackMarketItem>& GameEngine::GetBlackMarketItems() const {
    return m_blackMarket.GetItems();
}

// ─────────────────────────────────────────────────────────────────────────────
// 昼夜切换回调（由 GameClock 后台线程触发）
// 注意：这些函数在后台线程中执行，WorldMap 操作已通过 m_mapMutex 保护
// ─────────────────────────────────────────────────────────────────────────────

// 白天 → 夜晚：生成黑市商人 + 随机夜晚怪物
void GameEngine::_OnDayToNight() {
    // 重置并初始化黑市货架（每夜全新商品）
    m_blackMarket.InitItems();

    m_blackMarketNpcIds.clear();
    m_blackMarketSpawned = true;

    m_blackMarketSpawned = true;
}

// 夜晚 → 白天：从地图移除所有黑市商人
void GameEngine::_OnNightToDay() {
    for (int id : m_blackMarketNpcIds) {
        m_worldMap.RemoveInteractable(id);
    }
    m_blackMarketNpcIds.clear();
    m_blackMarketSpawned = false;
}

// (Removed _SpawnNightEnemies as night monsters are now placed via QtMapLoader and visibility is toggled)

// ─────────────────────────────────────────────────────────────────────────────
// 测试专用：L 键快捷升到最高等级
// ─────────────────────────────────────────────────────────────────────────────
bool GameEngine::CheatLevelUp() {
    if (!m_cheatEnabled) return false;
    if (!m_player) return false;
    // 反复调用 AddExp 直至到达 MAX_LEVEL
    while (m_player->GetLevel() < Character::MAX_LEVEL) {
        // 每次加足够多的经验确保升一级
        m_player->AddExp(m_player->ExpToNextLevel() * 2 + 9999);
    }
    // 顺手回满血
    m_player->HealHp(m_player->GetMaxHealth());
    return true;
}
