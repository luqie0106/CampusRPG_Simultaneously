#include "../include/Shop.h"

Shop::Shop() {
    InitShopItems();
}

Shop::~Shop() {}

void Shop::InitShopItems() {
    // TODO: 完善物品类的构造逻辑，这里假设存在相应的派生类或构造方式
    // m_shopItems.push_back(std::make_shared<Consumable>("苹果", "恢复少量生命值", 10, 20));
    // m_shopItems.push_back(std::make_shared<Equipment>("木剑", "增加少量攻击力", 50, 5));
}

std::stringstream Shop::DisplayShop() {
    std::stringstream ss;
    ss << "\n=== 校园小卖部 ===" << std::endl;
    if (m_shopItems.empty()) {
        ss << "商店里暂时没有物品。" << std::endl;
        return ss;
    }
    
    for (size_t i = 0; i < m_shopItems.size(); ++i) {
        // ss << i + 1 << ". " << m_shopItems[i]->GetName() 
        //           << " - 价格: " << m_shopItems[i]->GetPrice() << " 金币" << std::endl;
    }
    ss << "0. 离开商店" << std::endl;
    return ss;
}

std::stringstream Shop::BuyItem(std::shared_ptr<Character> player, int itemIndex) {
    std::stringstream ss;
    if (itemIndex < 1 || itemIndex > static_cast<int>(m_shopItems.size())) {
        ss << "无效的物品选择。" << std::endl;
        return ss;
    }

    auto item = m_shopItems[itemIndex - 1];
    /* 
    if (player->GetGold() >= item->GetPrice()) {
        player->SpendGold(item->GetPrice());
        player->GetBackpack().AddItem(item);
        ss << "购买 " << item->GetName() << " 成功！" << std::endl;
        return ss;
    } else {
        ss << "金币不足，购买失败。" << std::endl;
        return ss;
    }
    */
    return ss;
}
