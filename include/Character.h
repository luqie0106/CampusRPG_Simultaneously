#pragma once
#include <string>
#include "Backpack.h"
#include "Item.h"

class Character {
private:
    std::string name;
    int health;
    int maxHealth;
    int attack;
    int defense;
    int gold = 200;
    int dodge_rate = 0.0;
    int exp = 0;
    
    Backpack<Item> backpack;

public:
    Character() = default;
    Character(std::string name, int health, int attack, int defense, int gold, double dodge_rate) : 
    name(name), health(health), maxHealth(health), attack(attack), defense(defense), gold(gold), dodge_rate(dodge_rate) {}

    void Show() const {
        std::cout << "角色信息：" << std::endl;
        std::cout << "名字：" << name << std::endl;
        std::cout << "生命值：" << health << "/" << maxHealth << std::endl;
        std::cout << "攻击力：" << attack << std::endl;
        std::cout << "防御力：" << defense << std::endl;
        std::cout << "金币：" << gold << std::endl;
        std::cout << "闪避率：" << dodge_rate << std::endl;
    }
};

class Steve : public Character {
public:
    Steve() : Character("Steve", 100, 10, 5, 200, 0.12) {}
};