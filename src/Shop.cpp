#include "../include/Shop.h"
#include "../include/Exceptions.h"

Shop::Shop() {
    InitShopItems();
}

Shop::~Shop() {}

void Shop::InitShopItems() {
    m_shopItems.push_back({std::make_shared<Food>("烤肉串", 20, 50), 10, 10});
    m_shopItems.push_back({std::make_shared<Medicine>("提神饮料", 15, 30), 10, 10});
    m_shopItems.push_back({std::make_shared<Equipment>("木制球棒", 80, 0, 10), 1, 1});
    m_shopItems.push_back({std::make_shared<Equipment>("运动校服", 100, 15, 0), 1, 1});
    lastRefreshTime = std::chrono::system_clock::now();
}

void Shop::RefreshShop() {
    auto now = std::chrono::system_clock::now();
    auto hours = std::chrono::duration_cast<std::chrono::hours>(now - lastRefreshTime).count();
    if (hours >= 24) {
        for (auto& shopItem : m_shopItems) {
            shopItem.stock = shopItem.maxStock;
        }
        lastRefreshTime = now;
    }
}

std::stringstream Shop::DisplayShop() {
    std::stringstream ss;
    ss << "\n=== 校园小卖部 ===" << '\n';
    if (m_shopItems.empty()) {
        ss << "商店里暂时没有物品。" << '\n';
        return ss;
    }
    
    RefreshShop();
    for (size_t i = 0; i < m_shopItems.size(); ++i) {
        ss << i + 1 << ". " << m_shopItems[i].item->getName() 
           << " - 价格: " << m_shopItems[i].item->getValue() << " 金币 "
           << "(库存: " << m_shopItems[i].stock << "/" << m_shopItems[i].maxStock << ")\n"
           << "   详情: " << m_shopItems[i].item->Show().str() << "\n";
    }
    ss << "0. 离开商店" << '\n';
    return ss;
}

std::stringstream Shop::BuyItem(std::shared_ptr<Character> player, int itemIndex) {
    std::stringstream ss;
    RefreshShop();
    if (itemIndex < 1 || itemIndex > static_cast<int>(m_shopItems.size())) {
        throw GameException("无效的物品选择。");
    }

    auto& shopItem = m_shopItems[itemIndex - 1];
    
    if (shopItem.stock <= 0) {
        throw GameException(shopItem.item->getName() + " 已经售罄！");
    }

    if (player->GetGold() < shopItem.item->getValue()) {
        throw NoEnoughGoldException("金币不足，购买失败。");
    }

    player->SpendGold(shopItem.item->getValue());
        
        std::unique_ptr<Item> newItem;
        if (auto food = std::dynamic_pointer_cast<Food>(shopItem.item)) {
            newItem = std::make_unique<Food>(food->getName(), food->getValue(), food->GetHpRecovery());
        } else if (auto med = std::dynamic_pointer_cast<Medicine>(shopItem.item)) {
            newItem = std::make_unique<Medicine>(med->getName(), med->getValue(), med->GetMpRecovery());
        } else if (auto equip = std::dynamic_pointer_cast<Equipment>(shopItem.item)) {
            newItem = std::make_unique<Equipment>(equip->getName(), equip->getValue(), equip->GetDefenseBonus(), equip->GetAttackBonus());
        } else {
            newItem = std::make_unique<Item>(shopItem.item->getName(), shopItem.item->getValue());
        }
        
        player->GetBackpack().AddItem(std::move(newItem));
        shopItem.stock--;
        ss << "购买 " << shopItem.item->getName() << " 成功！" << '\n';
        return ss;
}
