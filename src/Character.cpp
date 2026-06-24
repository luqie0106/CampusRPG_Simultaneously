#include "../include/Character.h"
#include "../include/Exceptions.h"

Character::Character(std::string name, int health, int attack, int defense, int gold,
                     double dodge_rate, int level, int StaggerPoint, CharacterClass cls)
    : name(name), health(health), maxHealth(health), attack(attack), defense(defense),
      gold(gold), dodge_rate(dodge_rate), level(level), StaggerPoint(StaggerPoint),
      classType(cls) {}

Character::Character(std::string name) 
    : name(name), health(20), maxHealth(20), attack(10), defense(5),
      gold(100), dodge_rate(0.05), level(1), StaggerPoint(10),
      classType(CharacterClass::Student) {}

void Character::ConsumeFood(const Food& food, int rounds) {
    // 如果已有 buff 就覆盖（重新开始计时）
    foodBuffAtk        = food.GetAtkBuff();
    foodBuffRoundsLeft = rounds;
    // HP 立即回复部分由 UseItem 那一层处理，这里只负责设置 buff
}

// ── 职业查询 ──────────────────────────────────────────────────────
CharacterClass Character::GetClass() const { return classType; }

std::string Character::GetClassName() const {
    switch (classType) {
        case CharacterClass::Athlete: return "体育生";
        case CharacterClass::Nerd:    return "学霸";
        default:                      return "普通学生";
    }
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
    if (equippedHead)   totalAtk += equippedHead->GetAttackBonus();
    if (equippedBody)   totalAtk += equippedBody->GetAttackBonus();
    if (equippedLegs)   totalAtk += equippedLegs->GetAttackBonus();
    if (equippedFeet)   totalAtk += equippedFeet->GetAttackBonus();
    totalAtk -= GetStatusAtkPenalty(); // 扣除 Weakness 造成的攻击力惩罚
    return std::max(1, totalAtk); // 最少0，保证攻击力不负
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

    // 辅助 lambda：如果该槽有旧装备，先归还到背包再覆盖
    auto returnOldToBackpack = [&](std::shared_ptr<Equipment>& currentSlot) {
        if (currentSlot) {
            // shared_ptr<Equipment> → unique_ptr<Equipment>（拷贝构造克隆一份）
            backpack.AddItem(std::make_unique<Equipment>(*currentSlot));
        }
        currentSlot = equip;
    };

    switch (slot) {
        case EquipSlot::Head:   returnOldToBackpack(equippedHead);   break;
        case EquipSlot::Body:   returnOldToBackpack(equippedBody);   break;
        case EquipSlot::Legs:   returnOldToBackpack(equippedLegs);   break;
        case EquipSlot::Feet:   returnOldToBackpack(equippedFeet);   break;
        case EquipSlot::Weapon: returnOldToBackpack(equippedWeapon); break;
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

// ── 纯数据 Get（供 Qt MVC 层使用） ───────────────────────────────────
// Blind 状态下闪避率强制归零
double Character::GetDodgeRate() const {
    if (HasBlind()) return 0.0;
    return dodge_rate;
}
int    Character::GetStaggerPoint()      const { return StaggerPoint; }
int    Character::GetFoodBuffAtk()       const { return foodBuffAtk; }
int    Character::GetFoodBuffRoundsLeft() const { return foodBuffRoundsLeft; }

// ── 状态效果 ────────────────────────────────────────────────────────────

void Character::AddStatusEffect(StatusEffect effect) {
    // 如果已有相同类型的效果，用新的覆盖旧的（重设计时）
    for (auto& e : m_effects) {
        if (e.type == effect.type) {
            e = effect;
            return;
        }
    }
    m_effects.push_back(effect);
}

std::string Character::TickStatusEffects() {
    if (m_effects.empty()) return "";

    std::stringstream ss;
    std::vector<StatusEffect> remaining;

    for (auto& e : m_effects) {
        switch (e.type) {
            case StatusEffectType::HpRegen: {
                int before = health;
                HealHp(e.value);
                int actual = health - before;
                ss << "❤️ 《生命恢复》恢复了 " << actual << " 点HP（"
                   << health << "/" << maxHealth << "）\n";
                break;
            }
            case StatusEffectType::Wither:
                TakeDamage(e.value);
                ss << "🖤 《凋零》小啷损失了 " << e.value << " 点HP（"
                   << health << "/" << maxHealth << "）\n";
                break;
            case StatusEffectType::Poison:
                TakeDamage(e.value);
                ss << "💚 《中毒》小啷损失了 " << e.value << " 点HP（"
                   << health << "/" << maxHealth << "）\n";
                break;
            case StatusEffectType::Weakness:
                ss << "💪 《虚弱》攻击力降低 " << e.value << " 点（剩 "
                   << (e.roundsLeft - 1) << " 回合）\n";
                break;
            case StatusEffectType::Slow:
                ss << "🐢 《迟缓》本次攻击伤害将减半（剩 "
                   << (e.roundsLeft - 1) << " 回合）\n";
                break;
            case StatusEffectType::Blind:
                ss << "👁️ 《失明》闪避率归零（剩 "
                   << (e.roundsLeft - 1) << " 回合）\n";
                break;
            default: break;
        }

        --e.roundsLeft;
        if (e.roundsLeft > 0) {
            remaining.push_back(e);
        } else {
            ss << "✨ 《" << e.GetName() << "》效果已结束。\n";
        }
    }

    m_effects = std::move(remaining);
    return ss.str();
}

std::string Character::GetStatusEffectText() const {
    if (m_effects.empty()) return "无异常状态\n";
    std::stringstream ss;
    for (const auto& e : m_effects) {
        ss << e.GetDescription() << "\n";
    }
    return ss.str();
}

int Character::GetStatusAtkPenalty() const {
    int penalty = 0;
    for (const auto& e : m_effects) {
        if (e.type == StatusEffectType::Weakness) {
            penalty += e.value;
        }
    }
    return penalty;
}

bool Character::HasSlow() const {
    for (const auto& e : m_effects) {
        if (e.type == StatusEffectType::Slow) return true;
    }
    return false;
}

bool Character::HasBlind() const {
    for (const auto& e : m_effects) {
        if (e.type == StatusEffectType::Blind) return true;
    }
    return false;
}

void Character::ClearSlow() {
    m_effects.erase(
        std::remove_if(m_effects.begin(), m_effects.end(),
                       [](const StatusEffect& e){ return e.type == StatusEffectType::Slow; }),
        m_effects.end()
    );
}

void Character::ClearNegativeEffects() {
    // 保留 HpRegen 等正面效果；移除所有负面效果
    m_effects.erase(
        std::remove_if(m_effects.begin(), m_effects.end(),
            [](const StatusEffect& e) {
                return e.type == StatusEffectType::Poison   ||
                       e.type == StatusEffectType::Wither   ||
                       e.type == StatusEffectType::Weakness ||
                       e.type == StatusEffectType::Slow     ||
                       e.type == StatusEffectType::Blind;
            }),
        m_effects.end()
    );
}

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
    ss << "名字：" << name << "  [" << GetClassName() << "]" << std::endl;
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

// ─────────────────────────────────────────────────────────────────────────────
// 职业派生类构造函数
//
// 属性对比（区分小怪 / Boss 血量量级）：
//   Steve   : HP=20  ATK=10  DEF=5   Dodge=12%  Stagger=10  Gold=200
//   Athlete : HP=35  ATK=16  DEF=3   Dodge=5%   Stagger=5   Gold=150
//   Nerd    : HP=14  ATK=8   DEF=10  Dodge=25%  Stagger=12  Gold=250
// ─────────────────────────────────────────────────────────────────────────────

//               name   HP  ATK  DEF  Gold  Dodge   Lv  Stagger  Class
Steve::Steve(std::string name)
    : Character(name,   20,  10,   5, 200, 0.12,   1,  10, CharacterClass::Student) {}

Steve::Steve()
    : Character("Steve", 20, 10,   5, 200, 0.12,   1,  10, CharacterClass::Student) {}

Athlete::Athlete(std::string name)
    : Character(name,   35,  16,   3, 150, 0.05,   1,   5, CharacterClass::Athlete) {}

Nerd::Nerd(std::string name)
    : Character(name,   14,   8,  10, 250, 0.25,   1,  12, CharacterClass::Nerd) {}
