#pragma once
#include "Common.h"
#include "Character.h"
#include "Enemy.h"
#include "Shop.h"
#include <optional>

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
    Battle,         // 回合制战斗界面
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
    // 根据职业编号创建对应派生类角色，成功后状态变为 InGame。
    // classType: 1=体育生(Athlete)  2=学霸(Nerd)  3=普通学生(Steve)
    std::string CreatePlayer(const std::string& name, int classType = 3);

    // 返回职业选择说明文本（纯数据，不阻塞）；供 UI 在创建角色前展示
    std::string GetClassSelectionText() const;

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

    // 出售背包物品给系统（1-based 索引），返回出售结果文本
    // 出售价格 = 原价 × 50%（向下取整），最低 1 金币
    std::string SellBackpackItem(int backpackIndex);

    // 离开商店，状态回到 InGame
    std::string LeaveShop();

    // ── 存档 API ──────────────────────────────
    std::string SaveGame();
    std::string LoadGame();

    // ── 战斗 API ──────────────────────────────
    // 开启战斗：传入目标敌人，切换至 Battle 状态。
    // 触发条件由外部（地图/UI）判断（如玩家距 Boss ≤5 单位），引擎本身不感知坐标。
    // 调用失败（如状态不对）时返回错误文本。
    std::string StartBattle(Enemy targetEnemy);

    // 玩家攻击：扣除敌人生命值，之后触发敌人反击（若敌人存活且未瘫痪）。
    // 返回本回合完整的结算文本。
    std::string BattlePlayerAttack();

    // 玩家在战斗中使用背包物品（1-based 索引），之后触发敌人反击。
    // 返回本回合完整的结算文本。
    std::string BattlePlayerUseItem(int itemIndex);

    // 玩家逃跑（按 P）：以失败结算退出战斗，状态回到 InGame。
    std::string BattleFlee();

    // 查询当前交战敌人（仅 Battle 状态下有效，否则返回 nullptr）
    const Enemy* GetCurrentEnemy() const;

    // ── 退出 API ──────────────────────────────
    // 将状态设为 GameOver
    std::string QuitGame();

    // ════════════════════════════════════════════
    // Qt MVC 纯数据接口（不返回任何排版字符串）
    // Qt 控件仅调用这些函数绘制血条、图标、列表，
    // 无需解析任何 std::string。
    // ════════════════════════════════════════════

    // ── 玩家属性 ──────────────────────────────
    // 以下函数在 m_player == nullptr 时返回安全默认值
    int         GetPlayerHp()             const;  // 当前 HP
    int         GetPlayerMaxHp()          const;  // 最大 HP
    int         GetPlayerAttack()         const;  // 总攻击（含装备+食物buff）
    int         GetPlayerDefense()        const;  // 总防御（含装备）
    int         GetPlayerGold()           const;  // 持有金币
    int         GetPlayerLevel()          const;  // 当前等级
    int         GetPlayerExp()            const;  // 当前经验值
    int         GetPlayerExpToNext()      const;  // 升到下一级所需经验
    double      GetPlayerDodgeRate()      const;  // 闪避率
    int         GetPlayerStaggerPoint()   const;  // 玩家破韧值
    int         GetPlayerFoodBuffAtk()    const;  // 食物攻击 buff（0=无）
    int         GetPlayerFoodBuffRounds() const;  // 食物 buff 剩余回合数
    CharacterClass GetPlayerClass()       const;  // 职业枚举（供 Qt 切换图标）
    std::string GetPlayerClassName()      const;  // 职业中文名（"体育生"/"学霸"/"普通学生"）
    // ── 背包（供 Qt 物品格子使用）─────────────
    // 返回背包物品 const 引用，可 dynamic_cast 判断子类型
    const std::vector<std::unique_ptr<Item>>& GetBackpackItems() const;

    // ── 商店（供 Qt 商品列表使用）─────────────
    // 返回商店全部 ShopItem 的 const 引用
    const std::vector<ShopItem>& GetShopItemList() const;

    // ── 战斗中敌人数据（仅 Battle 状态有效）──
    // 函数内部安全检查，非战斗状态返回 -1 / false / ""
    int         GetBattleEnemyHp()          const;
    int         GetBattleEnemyAtk()         const;
    int         GetBattleEnemyDef()         const;
    bool        GetBattleEnemyIsStaggered() const;
    std::string GetBattleEnemyName()        const;

private:
    GameState                   m_state;
    std::shared_ptr<Character>  m_player;
    Shop                        m_shop;

    // ── 战斗状态 ──────────────────────────────
    std::optional<Enemy>        m_currentEnemy; // 当前交战的敌人副本
    bool                        m_inBattle = false;

    // 内部辅助：执行敌人回合（若敌人存活且未瘫痪则反击）
    // 返回敌人回合的文本；同时处理玩家死亡 → GameOver
    std::string _EnemyTurn();

    // 内部辅助：结算战斗胜利（经验 / 金币 / 掉落），切回 InGame
    std::string _SettleVictory();
};
