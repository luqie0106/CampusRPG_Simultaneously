#pragma once
#include "Common.h"

// ─────────────────────────────────────────────────────────────────────────────
// 状态效果类型枚举
// ─────────────────────────────────────────────────────────────────────────────
enum class StatusEffectType {
    None,       // 无效果（占位用）
    HpRegen,    // 生命恢复：每回合开始时回复 value 点 HP
    Wither,     // 凋零    ：每回合开始时损失 value 点 HP（Boss 专属）
    Poison,     // 中毒    ：每回合开始时损失 value 点 HP（稍弱于凋零）
    Weakness,   // 虚弱    ：攻击力降低 value 点（Boss 专属）
    Slow,       // 迟缓    ：下一次攻击伤害减半（Boss 专属）
    Blind,      // 失明    ：闪避率归零（Boss 专属）
};

// ─────────────────────────────────────────────────────────────────────────────
// StatusEffect — 单个状态效果实例
// ─────────────────────────────────────────────────────────────────────────────
struct StatusEffect {
    StatusEffectType type;   // 效果类型
    int              value;  // 每回合效果量（HP 增减或属性减少值）
    int              roundsLeft; // 剩余持续回合数

    StatusEffect(StatusEffectType t, int val, int rounds);

    // 返回效果中文名称（如 "凋零"）
    std::string GetName() const;

    // 返回一行效果描述（如 "凋零：每回合损失 5 HP，剩余 2 回合"）
    std::string GetDescription() const;
};
