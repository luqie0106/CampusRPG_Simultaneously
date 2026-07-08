#pragma once

#include "Common.h"
#include "StatusEffect.h"
#include "RNG.h"

class Character;
class Item;

class Enemy {
    private:
        std::string name;
        int maxHealth;
        int health;
        int attack;
        int defense;
        int exp;
        int gold;
        std::shared_ptr<Item> dropItem;

        double maxStaggerPoints;     // 最大韧性值（0 表示小怪，无韧度机制）
        double currentStaggerPoints;
        int staggerDuration;      // 瘫瘪持续回合数（小怪无意义）
        bool isStaggered;
        int staggerRoundsLeft;

        // Boss 专属负面状态效果配置（小怪匹配项均为 0/None）
        StatusEffectType m_debuffType     = StatusEffectType::None;
        int              m_debuffChance   = 0;   // 触发概率（整数百分毒，30–80）
        int              m_debuffValue    = 0;   // debuff 每回合效果量
        int              m_debuffDuration = 0;   // 持续回合数（1–3）

    public:
        // 基础构造函数
        // 小怪：staggerDuration = 0，maxStaggerPoints = 0
        // Boss：传入具体数值
        Enemy(std::string name, int health, int attack, int defense, int exp, int gold,
              double maxStaggerPoints, int staggerDuration = 0,
              StatusEffectType debuffType = StatusEffectType::None,
              int debuffChance = 0, int debuffValue = 0, int debuffDuration = 0);

        std::string DisplayStatus() const;

        int  GetMaxHealth() const;
        int  GetHealth() const;
        std::string GetName() const;  // 故人名称（供 Qt 直接显示品属标签）
        double GetMaxStaggerPoints() const;
        double GetCurrentStaggerPoints() const;
        bool IsStaggered() const; // 是否处于瘫瘪状态
        bool TickStagger(); // 瘫瘪状态回合推进
        void TakeDamage(int damage); // 受到伤害
        void TakeToughnessDamage(double toughnessDamage); // 受到破韧值伤害
        std::string Attack(Character& target); // 攻击玩家
        int  GetAttack() const; // 获取攻击力
        int  GetDefense() const; // 获取防御力
        int  GetExp() const; // 获取经验值
        int  GetGold() const; // 获取金币
        bool IsBoss() const;  // maxStaggerPoints > 0 即为 Boss
        std::shared_ptr<Item> GetDropItem() const; // 获取掉落物
        void SetDropItem(std::shared_ptr<Item> item); // 设置掉落物

        // ========== 小怪工厂 ==========
        static Enemy Bully();           // 校园混混
        static Enemy Skipper();         // 逃课大神
        static Enemy Cheater();         // 考试黄牛
        static Enemy GangMember();      // 小弟弟
        static Enemy ForestMonster1();  // 树林野兽
        static Enemy ForestMonster2();  // 幽暗黑影

        // ========== Boss 工厂 ==========
        static Enemy DeanOfStudents();  // 教导主任  (瘫瘪 1 回合)
        static Enemy PECommittee();     // 体育委员长  (瘫瘪 2 回合)
        static Enemy Principal();       // 校长      (瘫瘪 3 回合)
        static Enemy ForestBoss();      // 树林霸主

        // ========== 夜晚专属怪物工厂 ==========
        // 宿管阿姨：超高血量高防御 Boss，高破韧伤害 + 凋零 debuff
        static Enemy DormGuard();
        // 告夜卷王幽灵：血少但攻击极高的小怪，高年夜刷分奖励
        static Enemy MidnightNerd();
};
