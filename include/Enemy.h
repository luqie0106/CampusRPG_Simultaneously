#pragma once

#include "Common.h"
#include "StatusEffect.h"
#include "RNG.h"

class Character;

class Enemy {
    private:
        std::string name;
        int health;
        int attack;
        int defense;
        int exp;
        int gold;
        int maxStaggerPoints;     // 最大韧性值（0 表示小怪，无韧度机制）
        int currentStaggerPoints;
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
              int maxStaggerPoints, int staggerDuration = 0,
              StatusEffectType debuffType = StatusEffectType::None,
              int debuffChance = 0, int debuffValue = 0, int debuffDuration = 0);

        std::string DisplayStatus() const;

        int  GetHealth() const;
        std::string GetName() const;  // 故人名称（供 Qt 直接显示品属标签）
        bool IsStaggered() const;
        bool TickStagger();
        void TakeDamage(int damage);
        void TakeToughnessDamage(int toughnessDamage);
        std::string Attack(Character& target);
        int  GetAttack() const;
        int  GetDefense() const;
        int  GetExp() const;
        int  GetGold() const;
        bool IsBoss() const;  // maxStaggerPoints > 0 即为 Boss

        // ========== 小怪工厂 ==========
        static Enemy Bully();           // 校园混混
        static Enemy Skipper();         // 逃课大神
        static Enemy Cheater();         // 考试黄牛
        static Enemy GangMember();      // 小弟弟

        // ========== Boss 工厂 ==========
        static Enemy DeanOfStudents();  // 教导主任  (瘫瘪 1 回合)
        static Enemy PECommittee();     // 体育委员长  (瘫瘪 2 回合)
        static Enemy Principal();       // 校长      (瘫瘪 3 回合)
};