#pragma once

#include "Common.h"

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

    public:
        // 基础构造函数
        // 小怪：staggerDuration = 0，maxStaggerPoints = 0
        // Boss：传入具体数值
        Enemy(std::string name, int health, int attack, int defense, int exp, int gold,
              int maxStaggerPoints, int staggerDuration = 0);

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