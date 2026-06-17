#pragma once
#include "Backpack.h"
#include "Item.h"
#include "Common.h"

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
    Backpack backpack;

    std::shared_ptr<Equipment> equippedHead;
    std::shared_ptr<Equipment> equippedBody;
    std::shared_ptr<Equipment> equippedLegs;
    std::shared_ptr<Equipment> equippedFeet;
    std::shared_ptr<Equipment> equippedWeapon;

    std::atomic<int> buffAtk{0};
    std::atomic<float> foodTimer{0.0f};

public:
    Character() = default;
    Character(std::string name, int health, int attack, int defense, int gold, double dodge_rate, int level, int StaggerPoint);

    explicit Character(std::string name);

    void ConsumeFood(const Food& food);

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

    // ── 经验 & 升级 ──────────────────────────
    int  GetLevel() const;
    int  GetExp() const;
    int  ExpToNextLevel() const;        // 距离下一级所需经验
    bool AddExp(int amount);            // 增加经验，返回是否发生了升级
    bool LevelUp();                     // 手动触发升级，成功返回 true

    int GetGold() const;
    bool SpendGold(int amount);
    Backpack& GetBackpack();

    void EquipItem(std::shared_ptr<Equipment> equip);
    void UnequipItem(EquipSlot slot);
    std::shared_ptr<Equipment> GetEquipmentAt(EquipSlot slot);
    void TakeDamage(int damage);

    std::stringstream DisplayStatus() const;
};

class Steve : public Character {
public:
    Steve();
};
