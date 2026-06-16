#include "../include/Character.h"
#include "../include/TimerManager.h"
#include "../include/Exceptions.h"
Character::Character(std::string name, int health, int attack, int defense, int gold, double dodge_rate, int level, int StaggerPoint) : 
name(name), health(health), maxHealth(health), attack(attack), defense(defense), gold(gold), dodge_rate(dodge_rate), level(level), StaggerPoint(StaggerPoint) {}

Character::Character(std::string name) : 
name(name), health(100), maxHealth(100), attack(10), defense(5), gold(100), dodge_rate(0.05), level(1), StaggerPoint(10) {}

void Character::ConsumeFood(const Food& food) {
    buffAtk.store(food.GetAtkBuff());
    foodTimer.store(static_cast<float>(food.GetDuration()));
    
    TimerManager::StartCountdown(static_cast<float>(food.GetDuration()),
        [this](float remaining) {
            this->foodTimer.store(remaining);
        },
        [this]() {
            this->buffAtk.store(0);
            this->foodTimer.store(0.0f);
        }
    );
}

int Character::GetGold() const {
    return gold;
}

// ── 属性访问 ─────────────────────────────────────────────────────

std::string Character::GetName() const { return name; }
int Character::GetHealth() const { return health; }
int Character::GetMaxHealth() const { return maxHealth; }
int Character::GetAttack() const { return attack; }
int Character::GetDefense() const { return defense; }

void Character::HealHp(int amount) {
    health = std::min(health + amount, maxHealth);
}

void Character::AddAttack(int amount) {
    attack += amount;
}

void Character::AddDefense(int amount) {
    defense += amount;
}

bool Character::SpendGold(int amount) {
    if (gold < amount) {
        throw NoEnoughGoldException("金币不足！");
    }
    gold -= amount;
    return true;
}

Backpack& Character::GetBackpack() {
    return backpack;
}

std::stringstream Character::DisplayStatus() const {
    std::stringstream ss;
    ss << "角色信息：" << std::endl;
    ss << "名字：" << name << std::endl;
    ss << "生命值：" << health << "/" << maxHealth << std::endl;
    
    int currentBuff = buffAtk.load();
    float currentTimer = foodTimer.load();
    
    if (currentBuff > 0 && currentTimer > 0.0f) {
        ss << "攻击力：" << attack << " (+" << currentBuff << " 料理加成) | 料理剩余时间: " 
           << std::fixed << std::setprecision(1) << currentTimer << "s\n";
    } else {
        ss << "攻击力：" << attack << std::endl;
    }
    ss << "防御力：" << defense << std::endl;
    ss << "金币：" << gold << std::endl;
    ss << "闪避率：" << dodge_rate << std::endl;
    ss << "等级：" << level << std::endl;
    ss << "破韧值：" << StaggerPoint << std::endl;
    return ss;
}

Steve::Steve() : Character("Steve", 100, 10, 5, 200, 0.12, 1, 10) {}
