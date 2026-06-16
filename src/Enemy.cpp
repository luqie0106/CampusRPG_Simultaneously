#include "../include/Enemy.h"
#include "../include/TimerManager.h"

Enemy::Enemy(std::string name, int health, int attack, int defense, int exp, int gold, 
    int maxStaggerPoints, int currentStaggerPoints, bool isStaggered, float staggerTimer) : 
name(name), health(health), attack(attack), defense(defense), exp(exp), gold(gold), 
maxStaggerPoints(maxStaggerPoints), currentStaggerPoints(currentStaggerPoints), isStaggered(isStaggered), staggerTimer(staggerTimer) {}

std::string Enemy::DisplayStatus() const {
    std::stringstream ss;
    ss << "敌人信息：\n";
    ss << "名字：" << name << "\n";
    ss << "生命值：" << health << "\n";
    ss << "攻击力：" << attack << "\n";
    ss << "防御力：" << defense << "\n";
    ss << "经验值：" << exp << "\n";
    ss << "金币：" << gold << "\n";
    ss << "韧性值：" << currentStaggerPoints << "/" << maxStaggerPoints << "\n";
    if (isStaggered.load()) {
        ss << "状态：💥 瘫痪中（剩余 " << std::fixed << std::setprecision(1) << staggerTimer.load() << "s）\n";
    } else {
        ss << "状态：正常\n";
    }
    return ss.str();
}

int Enemy::GetHealth() const {
    return health;
}

void Enemy::TakeDamage(int damage) {
    health -= damage;
}

void Enemy::TakeToughnessDamage(int toughnessDamage) {
    if (isStaggered.load()) return;

    currentStaggerPoints -= toughnessDamage;
    if (currentStaggerPoints <= 0) {
        currentStaggerPoints = 0;
        
        isStaggered.store(true);
        staggerTimer.store(5.0f);

        TimerManager::StartCountdown(5.0f, 
            [this](float remaining) {
                this->staggerTimer.store(remaining);
            }, 
            [this]() {
                this->isStaggered.store(false);
                this->staggerTimer.store(0.0f);
                this->currentStaggerPoints = this->maxStaggerPoints;
            }
        );
    }
}

int Enemy::GetAttack() const {
    return attack;
}

int Enemy::GetDefense() const {
    return defense;
}

int Enemy::GetExp() const {
    return exp;
}

int Enemy::GetGold() const {
    return gold;
}
