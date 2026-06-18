#include "../include/Character.h"
#include "../include/Exceptions.h"
Character::Character(std::string name, int health, int attack, int defense, int gold, double dodge_rate, int level, int StaggerPoint) : 
name(name), health(health), maxHealth(health), attack(attack), defense(defense), gold(gold), dodge_rate(dodge_rate), level(level), StaggerPoint(StaggerPoint) {}

Character::Character(std::string name) : 
name(name), health(20), maxHealth(20), attack(10), defense(5), gold(100), dodge_rate(0.05), level(1), StaggerPoint(10) {}

void Character::ConsumeFood(const Food& food, int rounds) {
    // 如果已有 buff 就覆盖（重新开始计时）
    foodBuffAtk        = food.GetAtkBuff();
    foodBuffRoundsLeft = rounds;
    // HP 立即回复部分由 UseItem 那一层处理，这里只负责设置 buff
}

int Character::GetGold() const {
    return gold;
}

// ── 属性访问 ─────────────────────────────────────────────────────

std::string Character::GetName() const { return name; }
int Character::GetHealth() const { return health; }
int Character::GetMaxHealth() const { return maxHealth; }
int Character::GetAttack() const { 
    int totalAtk = attack + foodBuffAtk;  // 加上当前食物 buff
    if (equippedWeapon) totalAtk += equippedWeapon->GetAttackBonus();
    if (equippedHead) totalAtk += equippedHead->GetAttackBonus();
    if (equippedBody) totalAtk += equippedBody->GetAttackBonus();
    if (equippedLegs) totalAtk += equippedLegs->GetAttackBonus();
    if (equippedFeet) totalAtk += equippedFeet->GetAttackBonus();
    return totalAtk; 
}
int Character::GetDefense() const { 
    int totalDef = defense;
    if (equippedWeapon) totalDef += equippedWeapon->GetDefenseBonus();
    if (equippedHead) totalDef += equippedHead->GetDefenseBonus();
    if (equippedBody) totalDef += equippedBody->GetDefenseBonus();
    if (equippedLegs) totalDef += equippedLegs->GetDefenseBonus();
    if (equippedFeet) totalDef += equippedFeet->GetDefenseBonus();
    return totalDef; 
}
int Character::GetBaseAttack() const { return attack; }
int Character::GetBaseDefense() const { return defense; }

void Character::TakeDamage(int damage) {
    health -= damage;
    if (health < 0) health = 0;
}

void Character::EquipItem(std::shared_ptr<Equipment> equip) {
    if (!equip) return;
    EquipSlot slot = equip->GetSlot();
    switch (slot) {
        case EquipSlot::Head: equippedHead = equip; break;
        case EquipSlot::Body: equippedBody = equip; break;
        case EquipSlot::Legs: equippedLegs = equip; break;
        case EquipSlot::Feet: equippedFeet = equip; break;
        case EquipSlot::Weapon: equippedWeapon = equip; break;
        default: break;
    }
}

void Character::UnequipItem(EquipSlot slot) {
    switch (slot) {
        case EquipSlot::Head: equippedHead = nullptr; break;
        case EquipSlot::Body: equippedBody = nullptr; break;
        case EquipSlot::Legs: equippedLegs = nullptr; break;
        case EquipSlot::Feet: equippedFeet = nullptr; break;
        case EquipSlot::Weapon: equippedWeapon = nullptr; break;
        default: break;
    }
}

std::shared_ptr<Equipment> Character::GetEquipmentAt(EquipSlot slot) {
    switch (slot) {
        case EquipSlot::Head: return equippedHead;
        case EquipSlot::Body: return equippedBody;
        case EquipSlot::Legs: return equippedLegs;
        case EquipSlot::Feet: return equippedFeet;
        case EquipSlot::Weapon: return equippedWeapon;
        default: return nullptr;
    }
}

void Character::HealHp(int amount) {
    health = std::min(health + amount, maxHealth);
}

void Character::AddAttack(int amount) {
    attack += amount;
}

void Character::AddDefense(int amount) {
    defense += amount;
}

// ── 食物 Buff 回合推进 ────────────────────────────────────────────
void Character::TickFoodBuff() {
    if (foodBuffRoundsLeft <= 0) return;
    --foodBuffRoundsLeft;
    if (foodBuffRoundsLeft <= 0) {
        foodBuffAtk        = 0;
        foodBuffRoundsLeft = 0;
    }
}

// ── 纯数据 Get（供 Qt MVC 层使用） ─────────────────────────────────
double Character::GetDodgeRate()    const { return dodge_rate; }
int    Character::GetStaggerPoint() const { return StaggerPoint; }
int    Character::GetFoodBuffAtk()  const { return foodBuffAtk; }
int    Character::GetFoodBuffRoundsLeft() const { return foodBuffRoundsLeft; }

// ── 经验 & 升级 ───────────────────────────────────────────────────

int Character::GetLevel() const { return level; }
int Character::GetExp() const { return exp; }

int Character::ExpToNextLevel() const {
    // 每级需要经验 = 当前等级 * 10（1级→2级需10，2级→3级需20，以此类推）
    return level * 10;
}

bool Character::LevelUp() {
    if (level >= MAX_LEVEL) return false;   // 已满级
    ++level;
    // 每级 +1 颗心 (HP_PER_LEVEL = 2)
    maxHealth += HP_PER_LEVEL;
    health     = std::min(health + HP_PER_LEVEL, maxHealth); // 升级顺便回满这部分
    // 每级攻击 +2
    attack += ATK_PER_LEVEL;
    exp = 0;    // 重置本级经验
    return true;
}

bool Character::AddExp(int amount) {
    if (level >= MAX_LEVEL) return false;   // 满级不再累积
    exp += amount;
    bool leveled = false;
    // 支持连续升多级
    while (exp >= ExpToNextLevel() && level < MAX_LEVEL) {
        exp -= ExpToNextLevel();
        LevelUp();
        leveled = true;
    }
    return leveled;
}

bool Character::SpendGold(int amount) {
    if (gold < amount) {
        throw NoEnoughGoldException("金币不足！");
    }
    gold -= amount;
    return true;
}

void Character::AddGold(int amount) {
    if (amount > 0) gold += amount;
}

Backpack& Character::GetBackpack() {
    return backpack;
}

const Backpack& Character::GetBackpack() const {
    return backpack;
}

std::stringstream Character::DisplayStatus() const {
    std::stringstream ss;
    ss << "角色信息：" << std::endl;
    ss << "名字：" << name << std::endl;
    ss << "生命值：" << health << "/" << maxHealth << std::endl;
    
    if (foodBuffAtk > 0 && foodBuffRoundsLeft > 0) {
        ss << "攻击力：" << attack << " (+" << foodBuffAtk
           << " 料理加成) | 料理剩余回合: " << foodBuffRoundsLeft << "\n";
    } else {
        ss << "攻击力：" << attack << std::endl;
    }
    ss << "防御力：" << defense << std::endl;
    ss << "金币：" << gold << std::endl;
    ss << "闪避率：" << dodge_rate << std::endl;
    ss << "等级：" << level;
    if (level < MAX_LEVEL) {
        ss << "  (" << exp << "/" << ExpToNextLevel() << " exp)";
    } else {
        ss << "  (MAX)";
    }
    ss << std::endl;
    ss << "破韧值：" << StaggerPoint << std::endl;
    return ss;
} //进阶优化建议： 如果后续发现界面卡顿，可以考虑抛弃 stringstream，
// 直接在 Qt 侧通过 QString::recurring 或 QString("%1").arg(...) 来拼接，或者只有当数据真正发生变化时才更新界面。

Steve::Steve() : Character("Steve", 20, 10, 5, 200, 0.12, 1, 10) {}
