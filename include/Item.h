#pragma once
#include "Common.h"

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
        int HpRecovery = 0; // 可以保留微量回血（吃东西总会饱嘛）
        int AtkBuff = 20;    // 临时增加的攻击力
        int DefBuff = 5;    // 临时增加的防御力
        int Duration = 5;   // Buff 持续时间（如果是回合制就是回合数，如果是即时制就是秒数）
    public:
        Food(std::string name, int value, int HpRecovery);
        ~Food() = default;
        int GetHpRecovery() const;
        int GetAtkBuff() const;
        int GetDefBuff() const;
        int GetDuration() const;
        std::stringstream Show() const override;
};

class Medicine : public Item {
    private:
        int MpRecovery;
    public:
        Medicine(std::string name, int value, int MpRecovery);
        ~Medicine() = default;
        int GetMpRecovery() const;
        std::stringstream Show() const override;
};

class Equipment : public Item {
    private:
        int defense_bonus;
        int attack_bonus;
    public:
        Equipment(std::string name, int value, int defense_bonus, int attack_bonus);
        ~Equipment() = default;
        int GetDefenseBonus() const;
        int GetAttackBonus() const;
        std::stringstream Show() const override;
};