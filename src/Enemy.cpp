#include "../include/Enemy.h"
#include "../include/Character.h"
#include "../include/RNG.h"
#include <cmath>

// maxStaggerPoints 传 0 即为无韧度小怪；staggerDuration 为 Boss 瘫痪回合数
Enemy::Enemy(std::string name, int health, int attack, int defense, int exp, int gold,
    int maxStaggerPoints, int staggerDuration,
    StatusEffectType debuffType, int debuffChance, int debuffValue, int debuffDuration) :
name(name), health(health), attack(attack), defense(defense), exp(exp), gold(gold),
maxStaggerPoints(maxStaggerPoints), currentStaggerPoints(maxStaggerPoints),
staggerDuration(staggerDuration), isStaggered(false), staggerRoundsLeft(0),
m_debuffType(debuffType), m_debuffChance(debuffChance),
m_debuffValue(debuffValue), m_debuffDuration(debuffDuration) {}

// ── 状态显示 ──────────────────────────────────────────────────────
std::string Enemy::DisplayStatus() const {
    std::stringstream ss;
    ss << "敌人信息：\n";
    ss << "名字：" << name << "\n";
    ss << "生命值：" << health << "\n";
    ss << "攻击力：" << attack << "\n";
    ss << "防御力：" << defense << "\n";
    ss << "经验值：" << exp << "\n";
    ss << "金币：" << gold << "\n";

    // 只有 Boss（maxStaggerPoints > 0）才显示韧度条
    if (maxStaggerPoints > 0) {
        ss << "韧性值：" << currentStaggerPoints << "/" << maxStaggerPoints << "\n";
    }

    if (isStaggered) {
        ss << "状态：💥 瘫痪中（剩余 " << staggerRoundsLeft << " 回合）\n";
    } else {
        ss << "状态：正常\n";
    }
    return ss.str();
}

// ── 基础属性 ──────────────────────────────────────────────────────
int  Enemy::GetHealth()  const { return health; }
std::string Enemy::GetName() const { return name; }
int  Enemy::GetAttack()  const { return attack; }
int  Enemy::GetDefense() const { return defense; }
int  Enemy::GetExp()     const { return exp; }
int  Enemy::GetGold()    const { return gold; }

bool Enemy::IsStaggered() const { return isStaggered; }

void Enemy::TakeDamage(int damage) {
    health -= damage;
}

std::string Enemy::Attack(Character& target) {
    if (isStaggered) {
        return name + " 处于瘫痪状态，无法攻击！\n";
    }

    // ── 闪避判定（优先于部位抽取）────────────────────────────────
    // RandInt(0, 99) / 100.0 ∈ [0.00, 0.99]，与 GetDodgeRate() ∈ [0.0, 0.95] 比较
    double dodgeRoll = RNG::RandInt(0, 99) / 100.0;
    if (dodgeRoll < target.GetDodgeRate()) {
        return target.GetName() + " 闪避了攻击！\n";
    }

    // 部位抽取：用全局 RNG 替换原来的局部 mt19937
    int roll = RNG::RandInt(1, 100);

    EquipSlot targetSlot;
    std::string partName;

    // 10% 头部，50% 身体，30% 腿部，10% 足部
    if (roll <= 10) {
        targetSlot = EquipSlot::Head;
        partName = "头部";
    } else if (roll <= 60) {
        targetSlot = EquipSlot::Body;
        partName = "身体";
    } else if (roll <= 90) {
        targetSlot = EquipSlot::Legs;
        partName = "腿部";
    } else {
        targetSlot = EquipSlot::Feet;
        partName = "足部";
    }

    std::shared_ptr<Equipment> partArmor = target.GetEquipmentAt(targetSlot);
    int partDef = 0;
    if (partArmor) {
        partDef = partArmor->GetDefenseBonus();
    }

    int overallDefense = target.GetBaseDefense() + partDef;
    int actualDamage = attack - overallDefense;
    if (actualDamage < 1) actualDamage = 1;

    target.TakeDamage(actualDamage);

    std::stringstream ss;
    ss << name << " 攻击了 " << target.GetName() << " 的 " << partName 
       << "，造成了 " << actualDamage << " 点伤害";

    if (partArmor) {
        int durLoss = 0;
        if (attack > 0) {
            durLoss = static_cast<int>(std::floor(std::log2(attack)));
        }
        if (durLoss < 0) durLoss = 0;

        partArmor->ReduceDurability(durLoss);
        ss << " (" << partArmor->getName() << " 耐久度 -" << durLoss << ")";

        if (partArmor->GetDurability() <= 0) {
            ss << "\n【" << partArmor->getName() << "】 已损坏！";
            target.UnequipItem(targetSlot);
        }
    }
    ss << "\n";

    // ══ Boss 专属：概率触发负面状态效果 ══
    if (IsBoss() && m_debuffType != StatusEffectType::None && m_debuffChance > 0) {
        int debuffRoll = RNG::RandInt(1, 100);
        if (debuffRoll <= m_debuffChance) {
            StatusEffect debuff(m_debuffType, m_debuffValue, m_debuffDuration);
            target.AddStatusEffect(debuff);
            ss << "⚡ " << name << " 将《" << debuff.GetName()
               << "》施加给了 " << target.GetName()
               << "！（" << debuff.GetDescription() << "）\n";
        }
    }

    return ss.str();
}

// ── 回合计时器 ────────────────────────────────────────────────────
// 每回合开始时调用；返回本回合是否仍处于瘫痪
bool Enemy::TickStagger() {
    if (!isStaggered) return false;
    --staggerRoundsLeft;
    if (staggerRoundsLeft <= 0) {
        isStaggered          = false;
        staggerRoundsLeft    = 0;
        currentStaggerPoints = maxStaggerPoints; // 韧度条恢复满
    }
    return isStaggered;
}

// ── 破韧伤害 ──────────────────────────────────────────────────────
void Enemy::TakeToughnessDamage(int toughnessDamage) {
    if (maxStaggerPoints == 0) return; // 小怪无韧度机制
    if (isStaggered)           return; // 瘫痪期间不叠加

    currentStaggerPoints -= toughnessDamage;
    if (currentStaggerPoints <= 0) {
        currentStaggerPoints = 0;
        isStaggered          = true;
        staggerRoundsLeft    = staggerDuration; // 使用各自配置的回合数
    }
}

bool Enemy::IsBoss() const { return maxStaggerPoints > 0; }

// ═══════════════════════════════════════════════════════════════════
// 小怪工厂（maxStaggerPoints = 0，无韧度条）
// ═══════════════════════════════════════════════════════════════════

// 校园混混：街头小霸王，喜欢在走廊游荡找人碰瓷
Enemy Enemy::Bully() {
    //               名字          HP  ATK DEF  EXP GOLD  韧度  瘫痪回合
    return Enemy("校园混混",        30,   8,  1,   6,  10,    0,  0);
}

// 逃课大神：行动飘忽，输出不高但极难预判
Enemy Enemy::Skipper() {
    return Enemy("逃课大神",        120,   4,  0,   4,   6,    0,  0);
}

// 考试黄牛：靠作弊为生，金币多但战斗力弱
Enemy Enemy::Cheater() {
    return Enemy("考试黄牛",        100,   2,  2,   3,  20,    0,  0);
}

// 小弟弟：跟在混混身后充数，群殴才有威胁
Enemy Enemy::GangMember() {
    return Enemy("小弟弟",           80,   6,  0,   5,   8,    0,  0);
}

// ═══════════════════════════════════════════════════════════════════
// Boss 工厂（各自有韧度条 + 不同瘫痪回合）
// ═══════════════════════════════════════════════════════════════════

// 教导主任：雷厉风行，但只要破韧就能让他停顿一拍（1 回合）
Enemy Enemy::DeanOfStudents() {
    //               名字          HP  ATK DEF  EXP GOLD  韧度  瘫痪  debuffType                  概率  每回合量  回合
    return Enemy("教导主任",         500,  25,  4,  30,  50,    5,   1,
                 StatusEffectType::Wither,   40,   5,          2);
}

// 体育委员长：力量型 Boss，体力充沛；破韧后喘息 2 回合
Enemy Enemy::PECommittee() {
    return Enemy("体育委员长",       800,  35,  6,  55,  80,    8,   2,
                 StatusEffectType::Weakness, 60,   8,          2);
}

// 校长：终极 Boss，防御极高；破韧后陷入长达 3 回合的混乱
Enemy Enemy::Principal() {
    return Enemy("校长",            1200,  45, 10, 100, 200,   12,   3,
                 StatusEffectType::Poison,   75,   8,          3);
}

// ═══════════════════════════════════════════════════════════════════
// 夜晚专属怪物工厂
// ═══════════════════════════════════════════════════════════════════

// 宿管阿姨：夜晚最强 Boss，高血高防，破韧后瘫痪 2 回合
// 特色：攻击附带 Wither（凋零）60% 概率，8 点/回合，持续 2 回合
// 击杀奖励：EXP=80 GOLD=120（约为白天 Boss 的 2~3 倍）
Enemy Enemy::DormGuard() {
    //               名字           HP  ATK DEF  EXP  GOLD  韧度  瘫痪  debuffType                概率  每回合量  回合
    return Enemy("宿管阿姨",         600,  30,  15,  80,  120,   10,   2,
                 StatusEffectType::Wither,   60,   8,          2);
}

// 午夜卷王幽灵：夜晚强力小怪，血量极少但攻击极高，无韧度
// 特色：单体小怪，击杀后提供丰厚奖励
// EXP=150 GOLD=200（击杀收益远超白天）
Enemy Enemy::MidnightNerd() {
    //               名字             HP  ATK DEF  EXP  GOLD  韧度  瘫痪（小怪无意义）
    return Enemy("午夜卷王幽灵",       120,  60,   2, 150,  200,    0,   0);
}
