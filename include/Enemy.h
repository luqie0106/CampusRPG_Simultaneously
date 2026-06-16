#pragma once

#include "Common.h"

class Enemy {
    private:
        std::string name;
        int health; //当前健康值
        int attack; //攻击力
        int defense; //防御力
        int exp; //击败获得经验
        int gold; //击败获得金币
        int maxStaggerPoints; //最大韧性值
        int currentStaggerPoints; // 当前韧性值
        std::atomic<bool> isStaggered; // 是否处于瘫痪状态
        std::atomic<float> staggerTimer; // 瘫痪剩余时间计时器
    public:
        Enemy(std::string name, int health, int attack, int defense, int exp, int gold, 
            int maxStaggerPoints, int currentStaggerPoints, bool isStaggered, float staggerTimer);

        std::string DisplayStatus() const;

        int GetHealth() const;
        void TakeDamage(int damage);
        void TakeToughnessDamage(int toughnessDamage);
        int GetAttack() const;
        int GetDefense() const;
        int GetExp() const;
        int GetGold() const;
};