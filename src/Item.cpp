#include "Common.h"

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

Food::Food(std::string name, int value, int HpRecovery, int AtkBuff, int DefBuff,
           int Duration, StatusEffectType effectType, int effectValue)
    : Item(name, value), HpRecovery(HpRecovery), AtkBuff(AtkBuff), DefBuff(DefBuff),
      Duration(Duration), effectType(effectType), effectValue(effectValue) {}

int Food::GetHpRecovery() const { return HpRecovery; }
int Food::GetAtkBuff()    const { return AtkBuff; }
int Food::GetDefBuff()    const { return DefBuff; }
int Food::GetDuration()   const { return Duration; }
StatusEffectType Food::GetEffectType()  const { return effectType;  }
int              Food::GetEffectValue() const { return effectValue; }

std::stringstream Food::Show() const {
    std::stringstream ss;
    ss << "[食物]" << getName() << " | 即时回血 " << HpRecovery;
    if (AtkBuff > 0) ss << " 攻击+" << AtkBuff;
    if (DefBuff > 0) ss << " 防御+" << DefBuff;
    ss << "  持续 " << Duration << " 回合";
    if (effectType != StatusEffectType::None) {
        StatusEffect tmp(effectType, effectValue, Duration);
        ss << " | 附加效果: " << tmp.GetName();
    }
    return ss;
}

Medicine::Medicine(std::string name, int value, int HpRecovery) : 
    Item(name, value), HpRecovery(HpRecovery) {}

int Medicine::GetHpRecovery() const { return HpRecovery; }

std::stringstream Medicine::Show() const {
    std::stringstream ss;
    ss << "【药品】" << getName() << " | 效果: 恢复 " << HpRecovery << " 点精力";
    return ss;
}

Equipment::Equipment(std::string name, int value, int defense_bonus, int attack_bonus,
                     int durability, EquipSlot slot, int dodge_bonus, double stagger_bonus)
    : Item(name, value), defense_bonus(defense_bonus), attack_bonus(attack_bonus),
      dodge_bonus(dodge_bonus), stagger_bonus(stagger_bonus),
      durability(durability), slot(slot) {}

int    Equipment::GetDefenseBonus()  const { return defense_bonus; }
int    Equipment::GetAttackBonus()   const { return attack_bonus; }
int    Equipment::GetDodgeBonus()    const { return dodge_bonus; }
double Equipment::GetStaggerBonus()  const { return stagger_bonus; }
int    Equipment::GetDurability()    const { return durability; }
EquipSlot Equipment::GetSlot()       const { return slot; }

void Equipment::ReduceDurability(int amount) {
    durability -= amount;
}

std::stringstream Equipment::Show() const {
    std::stringstream ss;
    ss << "【装备】" << getName() << " | 攻击+" << attack_bonus
       << " 防御+" << defense_bonus;
    if (dodge_bonus > 0)    ss << " 闪避+" << dodge_bonus << "%";
    if (stagger_bonus > 0)  ss << " 破韧+" << stagger_bonus;
    ss << " 耐久:" << durability;
    return ss;
}

// ========== Food 工厂 ==========
std::shared_ptr<Food> Food::GoldenApple() {
    // 金苹果：即时回8HP + ATK+5，2回合 + HpRegen(每回合+3)
    //                   name   value HpRec AtkBuf DefBuf Dur    effectType           effectVal
    return std::make_shared<Food>("金苹果", 40, 8, 5, 10, 2, StatusEffectType::HpRegen, 3);
}

std::shared_ptr<Food> Food::EnchantedGoldenApple() {
    // 附魔金苹果：即时回20HP + ATK+15，3回合 + HpRegen(每回合+8)
    return std::make_shared<Food>("附魔金苹果", 120, 20, 15, 25, 3, StatusEffectType::HpRegen, 8);
}

std::shared_ptr<Food> Food::Steak() {
    // 熟牛排：即时回15HP + ATK+3，3回合，无附加状态效果
    return std::make_shared<Food>("熟牛排", 12, 15, 3, 0, 3);
}

std::shared_ptr<Food> Food::Pork() {
    // 猪排：即时回10HP + ATK+2 DEF+1，3回合，无附加状态效果
    return std::make_shared<Food>("猪排", 16, 10, 2, 1, 3);
}

// ========== 黑市专属 Food 工厂 ==========
std::shared_ptr<Food> Food::MidnightBBQ() {
    // 深夜烧烤：即时回 60 HP + ATK+20 DEF+5，持续 5 回合 + HpRegen 每回合 +10
    //                   name          value HpRec AtkBuf DefBuf Dur    effectType           effectVal
    return std::make_shared<Food>("深夜烧烤", 280, 60, 20, 5, 5, StatusEffectType::HpRegen, 10);
}

// ========== Medicine 工厂 ==========
std::shared_ptr<Medicine> Medicine::HealingPotion() {
    // 治疗药水I: 立即回复4颗心(8HP).     name      value    HpRecovery 
    return std::make_shared<Medicine>("治疗药水", 25,       8);
}

std::shared_ptr<Medicine> Medicine::StrongHealingPotion() {
    // 治疗药水II: 立即回复8颗心(16HP)     name          value    HpRecovery 
    return std::make_shared<Medicine>("强效治疗药水",    50,        16);
}

std::shared_ptr<Medicine> Medicine::RegenPotion() {
    // 再生药水: 回复较多血量, 模拟持续再生效果   name      value    HpRecovery 
    return std::make_shared<Medicine>("再生药水", 35,       12);
}

// ========== Equipment 工厂 ==========
std::shared_ptr<Equipment> Equipment::IronArmor() {
    // 铁甲: 防御+15, 耐久250
    //      name      value    defense_bonus  attack_bonus    durability
    return std::make_shared<Equipment>("铁甲", 80,        15,      0,           250, EquipSlot::Body);
}

std::shared_ptr<Equipment> Equipment::GoldenArmor() {
    // 金甲: 防御+11但价值高, 耐久300(金较软但不至于易碎)
    return std::make_shared<Equipment>("金甲", 100,       11,      2,          300, EquipSlot::Body);
}

std::shared_ptr<Equipment> Equipment::DiamondArmor() {
    // 钻石甲: 防御+20, 耐久1200   name      value    defense_bonus  attack_bonus    durability
    return std::make_shared<Equipment>("钻石甲", 300,       20,      0,           1200, EquipSlot::Body);
}

std::shared_ptr<Equipment> Equipment::IronSword() {
    // 铁剑: 攻击+35, 耐久600, 破韧+0.5
    //                              name  value  def  atk   dur          slot   dodge  stagger
    return std::make_shared<Equipment>("铁剑", 60,   0,   35,  600, EquipSlot::Weapon,  0,  0.5);
}

std::shared_ptr<Equipment> Equipment::GoldenSword() {
    // 金剑: 攻击+15, 耐久300, 破韧+0.5(金较软但不至于易碎)
    return std::make_shared<Equipment>("金剑", 80,   0,   15,  300, EquipSlot::Weapon,  0,  0.5);
}

std::shared_ptr<Equipment> Equipment::DiamondSword() {
    // 钻石剑: 攻击+50, 耐久1500, 破韧+1.5
    return std::make_shared<Equipment>("钻石剑", 250,  0,   50, 1500, EquipSlot::Weapon,  0,  1.5);
}

std::shared_ptr<Equipment> Equipment::IronHelmet() {
    // 铁头盔: 防御+10, 耐久250   name      value    defense_bonus  attack_bonus    durability
    return std::make_shared<Equipment>("铁头盔", 50,        10,      0,           250, EquipSlot::Head);
}

std::shared_ptr<Equipment> Equipment::GoldenHelmet() {
    // 金头盔: 防御+7, 耐久300(金较软但不至于易碎)
    return std::make_shared<Equipment>("金头盔", 70,       7,        2,          300, EquipSlot::Head);
}

std::shared_ptr<Equipment> Equipment::DiamondHelmet() {
    // 钻石头盔: 防御+15, 耐久1200   name      value    defense_bonus  attack_bonus    durability
    return std::make_shared<Equipment>("钻石头盔", 250,       15,      0,           1200, EquipSlot::Head);
}

std::shared_ptr<Equipment> Equipment::IronLeggings() {
    // 铁护腿: 防御+12, 耐久250   name      value    defense_bonus  attack_bonus    durability
    return std::make_shared<Equipment>("铁护腿", 60,        12,      0,           250, EquipSlot::Legs);
}

std::shared_ptr<Equipment> Equipment::GoldenLeggings() {
    // 金护腿: 防御+8, 耐久300(金较软但不至于易碎)
    return std::make_shared<Equipment>("金护腿", 80,       8,        2,          300, EquipSlot::Legs);
}

std::shared_ptr<Equipment> Equipment::DiamondLeggings() {
    // 钻石护腿: 防御+18, 耐久1200   name      value    defense_bonus  attack_bonus    durability
    return std::make_shared<Equipment>("钻石护腿", 250,       18,      0,           1200, EquipSlot::Legs);
}

std::shared_ptr<Equipment> Equipment::IronBoots() {
    // 铁靴子: 防御+6, 耐久250   name      value    defense_bonus  attack_bonus    durability
    return std::make_shared<Equipment>("铁靴子", 40,        6,      0,           250, EquipSlot::Feet);
}

std::shared_ptr<Equipment> Equipment::GoldenBoots() {
    // 金靴子: 防御+4, 耐久300(金较软但不至于易碎)
    return std::make_shared<Equipment>("金靴子", 60,       4,        2,          300, EquipSlot::Feet);
}

std::shared_ptr<Equipment> Equipment::DiamondBoots() {
    // 钻石靴子: 防御+10, 耐久1200   name      value    defense_bonus  attack_bonus    durability
    return std::make_shared<Equipment>("钻石靴子", 250,       10,      0,           1200, EquipSlot::Feet);
}

// ========== 黑市专属 Equipment 工厂 ==========
std::shared_ptr<Equipment> Equipment::NightWalkerCloak() {
    // 夜行衣：防御+5，闪避+40%，耐久 800，身体槽
    //                     name      value  def  atk  dur          slot          dodge
    return std::make_shared<Equipment>("夜行衣", 500, 5, 0, 800, EquipSlot::Body, 40);
}
