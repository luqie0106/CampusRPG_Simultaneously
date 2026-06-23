#pragma once
#include "Backpack.h"
#include "Item.h"
#include "Common.h"
#include "StatusEffect.h"

// 职业枚举（供 Qt MVC 层查询，不需解析字符串）
enum class CharacterClass {
    Student,  // 1 — 普通学生（均衡型）
    Athlete,  // 2 — 体育生（高血量 / 高攻击）
    Nerd,     // 3 — 学霸（高防御 / 高闪避）
};

class Character {
public:
    static constexpr int MAX_LEVEL    = 20;  // 等级上限
    static constexpr int HP_PER_LEVEL = 2;   // 每级 +1 颗心 (2HP)
    static constexpr int ATK_PER_LEVEL = 2;  // 每级 +2 攻击

private:
    std::string name;
    int health;
    int maxHealth;
    int attack;
    int defense;
    int gold = 200;
    double dodge_rate = 0.0;
    int exp = 0;
    int level = 1; // 等级
    int StaggerPoint = 10; //破韧值
    CharacterClass classType = CharacterClass::Student; // 职业类型
    Backpack backpack;

    std::shared_ptr<Equipment> equippedHead;
    std::shared_ptr<Equipment> equippedBody;
    std::shared_ptr<Equipment> equippedLegs;
    std::shared_ptr<Equipment> equippedFeet;
    std::shared_ptr<Equipment> equippedWeapon;

    int foodBuffAtk       = 0;  // 当前食物攻击 buff 值
    int foodBuffRoundsLeft = 0;  // 食物 buff 剩余回合数

    // 状态效果列表（生命恢复、凋零、中毒等）
    std::vector<StatusEffect> m_effects;

public:
    Character() = default;
    Character(std::string name, int health, int attack, int defense, int gold,
              double dodge_rate, int level, int StaggerPoint,
              CharacterClass cls = CharacterClass::Student);

    explicit Character(std::string name);

    // ── 职业查询 ─────────────────────────────────────
    CharacterClass GetClass()     const;  // 返回职业枚举（供 Qt 判断图标）
    std::string    GetClassName()  const;  // 返回职业中文名称

    // 吃食物：设置回合制 buff（默认 3 回合）
    void ConsumeFood(const Food& food, int rounds = 3);

    // ── 基础属性访问 ──────────────────────────
    std::string GetName() const;
    int GetHealth() const;
    int GetMaxHealth() const;
    int GetAttack() const;
    int GetBaseAttack() const;
    int GetDefense() const;
    int GetBaseDefense() const;

    // ── 属性修改（供 UseItem 调用） ───────────
    void HealHp(int amount);            // 恢复生命值，不超过上限
    void AddAttack(int amount);         // 永久增加攻击力
    void AddDefense(int amount);        // 永久增加防御力

    // ── 食物 Buff 回合推进 ───────────────────
    // 每个回合结束时调用一次；buff 归零时自动清除
    void TickFoodBuff();

    // ── 纯数据 Get（供 Qt MVC 层使用）────
    double GetDodgeRate()       const;  // 闪避率
    int    GetStaggerPoint()    const;  // 玩家破韧值
    int    GetFoodBuffAtk()     const;  // 当前食物攻击加成（0=无 buff）
    int    GetFoodBuffRoundsLeft() const; // 食物 buff 剩余回合数

    // ── 状态效果 ──────────────────
    // 添加一个状态效果（相同类型的效果会覆盖旧的）
    void AddStatusEffect(StatusEffect effect);
    // 每回合开始时调用一次：执行 HP 增减、属性修改，并递减剩余回合
    // 返回本回合状态效果的文字描述（为空则表示无活跃效果）
    std::string TickStatusEffects();
    // 返回当前全部状态效果的文字列表（供战斗界面显示）
    std::string GetStatusEffectText() const;
    // 返回 Weakness 效果导致的攻击力惩罚和（供 GetAttack() 扣除）
    int GetStatusAtkPenalty() const;
    // 返回是否处于 Slow 状态（下一次攻击伤害减半）
    bool HasSlow() const;
    // 返回是否处于 Blind 状态（闪避率归零）
    bool HasBlind() const;
    // Slow 状态在攻击后需要消除（一次性）
    void ClearSlow();

    // ── 经验 & 升级 ──────────────────────────
    int  GetLevel() const;
    int  GetExp() const;
    int  ExpToNextLevel() const;        // 距离下一级所需经验
    bool AddExp(int amount);            // 增加经验，返回是否发生了升级
    bool LevelUp();                     // 手动触发升级，成功返回 true

    int GetGold() const;
    void AddGold(int amount);       // 增加金币（战斗奖励、捕获掉落等）
    bool SpendGold(int amount);
    Backpack& GetBackpack();
    const Backpack& GetBackpack() const;  // const 重载，供 const 语境调用

    void EquipItem(std::shared_ptr<Equipment> equip);
    void UnequipItem(EquipSlot slot);
    std::shared_ptr<Equipment> GetEquipmentAt(EquipSlot slot);
    void TakeDamage(int damage);

    std::stringstream DisplayStatus() const;
};

// ─────────────────────────────────────────────────────────────────────────────
// 职业派生类
// 所有构造函数接收 std::string name，满足 CreatePlayer(name, classType) 调用规范
// ─────────────────────────────────────────────────────────────────────────────

// 普通学生：均衡型，各项属性适中
class Steve : public Character {
public:
    explicit Steve(std::string name);
    Steve();  // 保留无参版局内测试用
};

// 体育生：高血量、高攻击、低破韧值（不易打瘻痪敌人）
class Athlete : public Character {
public:
    explicit Athlete(std::string name);
};

// 学霸：高防御、高闪避、低血量
class Nerd : public Character {
public:
    explicit Nerd(std::string name);
};
