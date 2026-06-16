#pragma once
#include "Backpack.h"
#include "Item.h"
#include "Common.h"

class Character {
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

    std::atomic<int> buffAtk{0};
    std::atomic<float> foodTimer{0.0f};

public:
    Character() = default;
    Character(std::string name, int health, int attack, int defense, int gold, double dodge_rate, int level, int StaggerPoint);

    explicit Character(std::string name);

    void ConsumeFood(const Food& food);

    std::stringstream DisplayStatus() const;
};

class Steve : public Character {
public:
    Steve();
};
