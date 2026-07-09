#include "Common.h"

#include "Backpack.h"
#include "../include/Character.h"   // 完整定义，UseItem 需要调用 Character 的方法
#include "../include/Exceptions.h"

// ─────────────────────────────────────────────────────────────────────────────
// AddItem
// ─────────────────────────────────────────────────────────────────────────────

void Backpack::AddItem(std::unique_ptr<Item> item) {
    if (items.size() >= 1000) {
        throw GameException("背包已满，无法放入更多物品！");
    }
    if (item) {
        items.push_back(std::move(item));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// UseItem（1-based index）
// 根据物品实际类型（多态 dynamic_cast）产生不同效果，使用后移除。
// ─────────────────────────────────────────────────────────────────────────────

std::string Backpack::UseItem(int index, Character& player) {
    if (index < 1 || index > static_cast<int>(items.size())) {
        throw GameException("无效的物品序号：" + std::to_string(index));
    }

    // 转为 0-based
    int idx = index - 1;
    Item* raw = items[idx].get();

    std::stringstream ss;

    // ── Food：回复 HP + 触发限时 ATK/DEF Buff ────────────────────────────
    if (Food* food = dynamic_cast<Food*>(raw)) {
        int heal = food->GetHpRecovery();
        int before = player.GetHealth();
        player.HealHp(heal);
        int after = player.GetHealth();
        int actual = after - before;

        ss << "使用了【" << food->getName() << "】\n";
        if (actual > 0) {
            ss << "  生命值恢复 " << actual << " 点（当前 " << after
               << "/" << player.GetMaxHealth() << "）\n";
        } else {
            ss << "  生命值已满，无法恢复更多。\n";
        }

        // 调用 ConsumeFood 激活限时 Buff
        player.ConsumeFood(*food, food->GetDuration());
        if (food->GetAtkBuff() > 0 || food->GetDefBuff() > 0) {
            ss << "  获得料理增益：攻击力+" << food->GetAtkBuff()
               << " 防御力+" << food->GetDefBuff()
               << "，持续 " << food->GetDuration() << " 回合\n";
        }

        // 附加状态效果（如金苹果的生命恢复）
        if (food->GetEffectType() != StatusEffectType::None) {
            StatusEffect eff(food->GetEffectType(), food->GetEffectValue(), food->GetDuration());
            player.AddStatusEffect(eff);
            ss << "  附加效果：" << eff.GetDescription() << "\n";
        }

    // ── Medicine：永久恢复 HP ─────────────────────────────────────────────
    } else if (Medicine* med = dynamic_cast<Medicine*>(raw)) {
        int heal = med->GetHpRecovery();
        int before = player.GetHealth();
        player.HealHp(heal);
        int after = player.GetHealth();
        int actual = after - before;

        ss << "使用了【" << med->getName() << "】\n";
        if (actual > 0) {
            ss << "  生命值恢复 " << actual << " 点（当前 " << after
               << "/" << player.GetMaxHealth() << "）\n";
        } else {
            ss << "  生命值已满，药品没有额外效果。\n";
            return ss.str();
        }

    // ── Equipment：穿戴装备（装备后物品保留在背包列表，按钮变为"脱下"）──
    } else if (Equipment* equip = dynamic_cast<Equipment*>(raw)) {
        // 复制一份给装备槽，原 items[idx] 保留在背包中以便界面显示
        std::shared_ptr<Equipment> equipPtr(dynamic_cast<Equipment*>(
            std::make_unique<Equipment>(*equip).release()));

        player.EquipItem(equipPtr);

        ss << "装备了【" << equip->getName() << "】\n";
        int atkBonus = equip->GetAttackBonus();
        int defBonus = equip->GetDefenseBonus();
        if (atkBonus > 0) ss << "  攻击力 +" << atkBonus << " (来自装备)\n";
        if (defBonus > 0) ss << "  防御力 +" << defBonus << " (来自装备)\n";
        ss << "  当前属性 → 攻击力: " << player.GetAttack()
           << "  防御力: " << player.GetDefense() << "\n";

        // 注意：不 erase，装备物品留在背包列表中，由 BackpackWindow 显示为"已装备"状态
        return ss.str();

    } else if (StatBoostItem* boost = dynamic_cast<StatBoostItem*>(raw)) {
        player.AddAttack(boost->GetAtkBoost());
        player.AddDefense(boost->GetDefBoost());
        
        ss << "使用了【" << boost->getName() << "】\n";
        if (boost->GetAtkBoost() > 0) ss << "  永久攻击力 +" << boost->GetAtkBoost() << "\n";
        if (boost->GetDefBoost() > 0) ss << "  永久防御力 +" << boost->GetDefBoost() << "\n";
        ss << "  当前属性 → 攻击力: " << player.GetAttack()
           << "  防御力: " << player.GetDefense() << "\n";

    } else if (CouponItem* coupon = dynamic_cast<CouponItem*>(raw)) {
        ss << "【" << coupon->getName() << "】是一个被动特权凭证，只需要放在背包中即可生效，无需使用。\n";
        return ss.str(); // Don't remove it from backpack

    } else if (AchievementItem* ach = dynamic_cast<AchievementItem*>(raw)) {
        ss << "【" << ach->getName() << "】是特殊物品，放在背包里作为你实力的证明，无需使用。\n";
        return ss.str(); // Don't remove it from backpack

    } else {
        ss << "使用了【" << raw->getName() << "】，但没有产生特殊效果。\n";
    }

    // 使用后从背包移除
    items.erase(items.begin() + idx);
    return ss.str();
}

// ─────────────────────────────────────────────────────────────────────────────
// RemoveItem（1-based index）—— 丢弃物品，不产生效果
// ─────────────────────────────────────────────────────────────────────────────

std::string Backpack::RemoveItem(int index) {
    if (index < 1 || index > static_cast<int>(items.size())) {
        throw GameException("无效的物品序号：" + std::to_string(index));
    }
    int idx = index - 1;
    std::string name = items[idx]->getName();
    items.erase(items.begin() + idx);
    return "已丢弃【" + name + "】。\n";
}

// ─────────────────────────────────────────────────────────────────────────────
// Getters
// ─────────────────────────────────────────────────────────────────────────────

int Backpack::GetSize() const {
    return static_cast<int>(items.size());
}

const std::vector<std::unique_ptr<Item>>& Backpack::GetItems() const {
    return items;
}

std::string Backpack::GetItemInfo(int index) const {
    if (index < 1 || index > static_cast<int>(items.size())) {
        return "（无效序号）";
    }
    return items[index - 1]->Show().str();
}

int Backpack::GetItemValue(int index) const {
    if (index < 1 || index > static_cast<int>(items.size())) {
        return -1;
    }
    return items[index - 1]->getValue();
}

std::string Backpack::GetItemName(int index) const {
    if (index < 1 || index > static_cast<int>(items.size())) {
        return "";
    }
    return items[index - 1]->getName();
}

std::string Backpack::GetBackpackInfo() const {
    std::stringstream ss;
    ss << "====== 背包列表 ======\n";
    if (items.empty()) {
        ss << "（背包空空如也）\n";
    } else {
        for (size_t i = 0; i < items.size(); ++i) {
            ss << "[" << i + 1 << "] " << items[i]->Show().str() << "\n";
        }
    }
    ss << "共 " << items.size() << " 件物品\n";
    return ss.str();
}
