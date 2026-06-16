#include "../include/Item.h"

Item::Item(std::string name, int value) : name(name), value(value) {}

std::string Item::getName() const { return name; }
int Item::getValue() const { return value; }

bool Item::operator==(const Item& other) const {
    return name == other.name && value == other.value;
}

std::stringstream Item::Show() const {
    std::stringstream ss;
    ss << name << " " << value << std::endl;
    return ss;
}

Food::Food(std::string name, int value, int HpRecovery) : Item(name, value), HpRecovery(HpRecovery) {}

int Food::GetHpRecovery() const { return HpRecovery; }
int Food::GetAtkBuff() const { return AtkBuff; }
int Food::GetDefBuff() const { return DefBuff; }
int Food::GetDuration() const { return Duration; }

std::stringstream Food::Show() const {
    std::stringstream ss;
    ss << getName() << " " << getValue() << " " << GetHpRecovery() << std::endl;
    return ss;
}
