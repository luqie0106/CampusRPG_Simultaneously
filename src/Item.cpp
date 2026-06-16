#include "../include/Item.h"

Item::Item(std::string name, int value) : name(name), value(value) {}

std::string Item::getName() const { return name; }
int Item::getValue() const { return value; }

bool Item::operator==(const Item& other) const {
    return name == other.name && value == other.value;
}

std::stringstream Item::Show() const {
    std::stringstream ss;
    ss << name << " " << value << '\n';
    return ss;
}

Food::Food(std::string name, int value, int HpRecovery) : Item(name, value), HpRecovery(HpRecovery) {}

int Food::GetHpRecovery() const { return HpRecovery; }
int Food::GetAtkBuff() const { return AtkBuff; }
int Food::GetDefBuff() const { return DefBuff; }
int Food::GetDuration() const { return Duration; }

std::stringstream Food::Show() const {
    std::stringstream ss;
    ss << "【食物】" << getName() << " | 效果: 恢复 " << HpRecovery << " 点生命值";
    return ss;
}

Medicine::Medicine(std::string name, int value, int MpRecovery) : Item(name, value), MpRecovery(MpRecovery) {}

int Medicine::GetMpRecovery() const { return MpRecovery; }

std::stringstream Medicine::Show() const {
    std::stringstream ss;
    ss << "【药品】" << getName() << " | 效果: 恢复 " << MpRecovery << " 点精力";
    return ss;
}

Equipment::Equipment(std::string name, int value, int defense_bonus, int attack_bonus) : Item(name, value), defense_bonus(defense_bonus), attack_bonus(attack_bonus) {}

int Equipment::GetDefenseBonus() const { return defense_bonus; }
int Equipment::GetAttackBonus() const { return attack_bonus; }

std::stringstream Equipment::Show() const {
    std::stringstream ss;
    ss << "【装备】" << getName() << " | 效果: 攻击力+" << attack_bonus << " 防御力+" << defense_bonus;
    return ss;
}
