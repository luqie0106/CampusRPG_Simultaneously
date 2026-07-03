#include "../include/BlackMarket.h"
#include "../include/Exceptions.h"

BlackMarket::BlackMarket() {
    InitItems();
}

void BlackMarket::InitItems() {
    m_items.clear();
    // ── 黑市专属食物 ──────────────────────────────────────────────
    m_items.push_back({ Food::MidnightBBQ(), 3, 3 });
    // ── 黑市专属装备 ──────────────────────────────────────────────
    m_items.push_back({ Equipment::NightWalkerCloak(), 1, 1 });
}

const std::vector<BlackMarketItem>& BlackMarket::GetItems() const {
    return m_items;
}

std::stringstream BlackMarket::DisplayBlackMarket() const {
    std::stringstream ss;
    ss << "\n🌙 === 神秘黑市 === 🌙\n";
    ss << "（仅限夜间营业，天亮即消失）\n";
    if (m_items.empty()) {
        ss << "黑市今晚已售罄。\n";
        return ss;
    }
    for (size_t i = 0; i < m_items.size(); ++i) {
        ss << i + 1 << ". " << m_items[i].item->getName()
           << " - 价格: " << m_items[i].item->getValue() << " 金币 "
           << "(库存: " << m_items[i].stock << "/" << m_items[i].maxStock << ")\n"
           << "   详情: " << m_items[i].item->Show().str() << "\n";
    }
    ss << "0. 离开黑市\n";
    return ss;
}

std::stringstream BlackMarket::BuyItem(std::shared_ptr<Character> player, int itemIndex) {
    std::stringstream ss;

    if (itemIndex < 1 || itemIndex > static_cast<int>(m_items.size())) {
        throw GameException("无效的黑市商品选择。");
    }

    auto& bmi = m_items[itemIndex - 1];

    if (bmi.stock <= 0) {
        throw GameException(bmi.item->getName() + " 已售罄！");
    }
    if (player->GetGold() < bmi.item->getValue()) {
        throw NoEnoughGoldException("金币不足，黑市交易失败。");
    }

    player->SpendGold(bmi.item->getValue());

    // 按实际子类型克隆物品到背包
    std::unique_ptr<Item> newItem;
    if (auto food = std::dynamic_pointer_cast<Food>(bmi.item)) {
        newItem = std::make_unique<Food>(food->getName(), food->getValue(),
                                         food->GetHpRecovery(), food->GetAtkBuff(),
                                         food->GetDefBuff(), food->GetDuration(),
                                         food->GetEffectType(), food->GetEffectValue());
    } else if (auto equip = std::dynamic_pointer_cast<Equipment>(bmi.item)) {
        newItem = std::make_unique<Equipment>(equip->getName(), equip->getValue(),
                                              equip->GetDefenseBonus(), equip->GetAttackBonus(),
                                              equip->GetDurability(), equip->GetSlot(),
                                              equip->GetDodgeBonus());
    } else {
        newItem = std::make_unique<Item>(bmi.item->getName(), bmi.item->getValue());
    }

    player->GetBackpack().AddItem(std::move(newItem));
    bmi.stock--;

    ss << "🌙 购买【" << bmi.item->getName() << "】成功！"
       << "剩余金币：" << player->GetGold() << "\n";
    return ss;
}
