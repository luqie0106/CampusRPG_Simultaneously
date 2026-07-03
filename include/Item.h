#pragma once
#include "Common.h"
#include "StatusEffect.h"

enum class EquipSlot {
    Head,
    Body,
    Legs,
    Feet,
    Weapon,
    None
};

class Item {
private:
    std::string name;
    int value;

public:
    Item(std::string name, int value);
    virtual ~Item() = default;

    std::string getName() const;
    int getValue() const;

    virtual std::stringstream Show() const;
    bool operator==(const Item& other) const;
    
};

class Food : public Item {
    private:
        int HpRecovery = 0;  // 即时回血量
        int AtkBuff    = 0;  // 临时攻击力加成
        int DefBuff    = 0;  // 临时防御力加成
        int Duration   = 3;  // Buff 持续回合数（1–3 回合）
        // 附加状态效果（可选）
        StatusEffectType effectType  = StatusEffectType::None;
        int              effectValue = 0;  // 状态效果每回合的量值
    public:
        Food(std::string name, int value, int HpRecovery, int AtkBuff, int DefBuff,
             int Duration,
             StatusEffectType effectType  = StatusEffectType::None,
             int              effectValue = 0);
        ~Food() = default;
        int GetHpRecovery() const; // 获取回血量
        int GetAtkBuff() const; // 获取攻击力加成
        int GetDefBuff() const; // 获取防御力加成
        int GetDuration() const; // 获取持续时间
        StatusEffectType GetEffectType()  const;  // 附加状态效果类型（None = 无）
        int              GetEffectValue() const;  // 附加状态效果每回合量值
        std::stringstream Show() const override;

        // 食物工厂
        static std::shared_ptr<Food> GoldenApple();           // 金苹果：HpRegen 2 回合
        static std::shared_ptr<Food> EnchantedGoldenApple();  // 附魔金苹果：HpRegen 3 回合
        static std::shared_ptr<Food> Steak();                 // 熟牛排：无状态效果，3 回合
        static std::shared_ptr<Food> Pork();                  // 猪排：无状态效果，3 回合

        // ── 黑市专属食物工厂 ──────────────────────────────────────
        // 深夜烧烤：即时回 60 HP，ATK+20，持续 5 回合 + HpRegen 每回合+10
        static std::shared_ptr<Food> MidnightBBQ();
};

class Medicine : public Item {
    private:
        int HpRecovery; //回血量
    public:
        Medicine(std::string name, int value, int HpRecovery);
        ~Medicine() = default;
        int GetHpRecovery() const;
        std::stringstream Show() const override;

        // Minecraft 药品工厂
        static std::shared_ptr<Medicine> HealingPotion();
        static std::shared_ptr<Medicine> StrongHealingPotion();
        static std::shared_ptr<Medicine> RegenPotion();
};

class Equipment : public Item {
    private:
        int defense_bonus; //防御加成
        int attack_bonus;  //攻击加成
        int dodge_bonus;   //闪避率加成（整数百分比）
        double stagger_bonus; //破韧加成（叠加到玩家的破韧值上）
        int durability;    //持久度
        EquipSlot slot;    //装备部位
    public:
        // stagger_bonus 默认 0.0，对旧调用完全向后兼容
        Equipment(std::string name, int value, int defense_bonus, int attack_bonus,
                  int durability, EquipSlot slot, int dodge_bonus = 0,
                  double stagger_bonus = 0.0);
        ~Equipment() = default;
        int    GetDefenseBonus()  const;
        int    GetAttackBonus()   const;
        int    GetDodgeBonus()    const;  // 闪避率加成（整数百分比）
        double GetStaggerBonus()  const;  // 破韧加成
        int    GetDurability()    const;
        EquipSlot GetSlot()       const;
        void ReduceDurability(int amount);
        std::stringstream Show() const override;

        // Minecraft 装备工厂
        // 在普通的面向对象编程里，如果您想调用一个类的函数，必须先造出一个类的对象（实例）。
        // 加上 static 之后，这个函数就脱离了具体的对象，直接挂靠在类名上了。
        // 这就意味着，系统（比如商店刷新时，或者打怪掉落时）根本不需要提前拥有任何装备
        // 实例，而是只需要知道 "Equipment::IronSword" 这个名字，就能立刻“变”出铁甲。
        static std::shared_ptr<Equipment> IronArmor();
        static std::shared_ptr<Equipment> GoldenArmor();
        static std::shared_ptr<Equipment> DiamondArmor();
        static std::shared_ptr<Equipment> IronSword();
        static std::shared_ptr<Equipment> GoldenSword();
        static std::shared_ptr<Equipment> DiamondSword();
        static std::shared_ptr<Equipment> IronHelmet();
        static std::shared_ptr<Equipment> GoldenHelmet();
        static std::shared_ptr<Equipment> DiamondHelmet();
        static std::shared_ptr<Equipment> IronLeggings();
        static std::shared_ptr<Equipment> GoldenLeggings();
        static std::shared_ptr<Equipment> DiamondLeggings();
        static std::shared_ptr<Equipment> IronBoots();
        static std::shared_ptr<Equipment> GoldenBoots();
        static std::shared_ptr<Equipment> DiamondBoots();

        // ── 黑市专属装备工厂 ──────────────────────────────────────
        // 夜行衣：极高闪避率 +40%，DEF+5，耐久 800，身体槽
        static std::shared_ptr<Equipment> NightWalkerCloak();
};