#include "Common.h"

#include "../include/StatusEffect.h"

StatusEffect::StatusEffect(StatusEffectType t, int val, int rounds)
    : type(t), value(val), roundsLeft(rounds) {}

std::string StatusEffect::GetName() const {
    switch (type) {
        case StatusEffectType::HpRegen:  return "生命恢复";
        case StatusEffectType::Wither:   return "凋零";
        case StatusEffectType::Poison:   return "中毒";
        case StatusEffectType::Weakness: return "虚弱";
        case StatusEffectType::Slow:     return "迟缓";
        case StatusEffectType::Blind:    return "失明";
        default:                         return "无";
    }
}

std::string StatusEffect::GetDescription() const {
    std::string name = GetName();
    std::string desc;
    switch (type) {
        case StatusEffectType::HpRegen:
            desc = "每回合恢复 " + std::to_string(value) + " 点生命值";
            break;
        case StatusEffectType::Wither:
            desc = "每回合损失 " + std::to_string(value) + " 点生命值（凋零）";
            break;
        case StatusEffectType::Poison:
            desc = "每回合损失 " + std::to_string(value) + " 点生命值（中毒）";
            break;
        case StatusEffectType::Weakness:
            desc = "攻击力降低 " + std::to_string(value) + " 点";
            break;
        case StatusEffectType::Slow:
            desc = "下次攻击伤害减半";
            break;
        case StatusEffectType::Blind:
            desc = "闪避率归零";
            break;
        default:
            desc = "无效果";
            break;
    }
    return "【" + name + "】" + desc + "，剩余 " + std::to_string(roundsLeft) + " 回合";
}
