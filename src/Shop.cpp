#include "Common.h"

#include "../include/Shop.h"
#include "../include/Exceptions.h"
#include "../include/RNG.h"
#include "../include/GameEngine.h"

Shop::Shop() {
    lastRefreshDays = -1;
    InitShopItems();
}

Shop::~Shop() {}

void Shop::InitShopItems() {
    m_allShopItems.clear();
    // 食物
    m_allShopItems.push_back({Food::Pork(),                  5, 5});
    m_allShopItems.push_back({Food::Steak(),                 5, 5});
    m_allShopItems.push_back({Food::GoldenApple(),           3, 3});
    m_allShopItems.push_back({Food::EnchantedGoldenApple(),  1, 1});
    // 药品
    m_allShopItems.push_back({Medicine::HealingPotion(),       5, 5});
    m_allShopItems.push_back({Medicine::StrongHealingPotion(), 3, 3});
    m_allShopItems.push_back({Medicine::RegenPotion(),         3, 3});
    // 装备
    m_allShopItems.push_back({Equipment::IronSword(),    2, 2});
    m_allShopItems.push_back({Equipment::IronHelmet(),   2, 2});
    m_allShopItems.push_back({Equipment::IronArmor(),    2, 2});
    m_allShopItems.push_back({Equipment::IronLeggings(), 2, 2});
    m_allShopItems.push_back({Equipment::IronBoots(),    2, 2});
    
    m_allShopItems.push_back({Equipment::GoldenSword(),  1, 1});
    m_allShopItems.push_back({Equipment::GoldenHelmet(), 1, 1});
    m_allShopItems.push_back({Equipment::GoldenArmor(),  1, 1});
    m_allShopItems.push_back({Equipment::GoldenLeggings(),1, 1});
    m_allShopItems.push_back({Equipment::GoldenBoots(),  1, 1});
    
    m_allShopItems.push_back({Equipment::DiamondSword(), 1, 1});
    m_allShopItems.push_back({Equipment::DiamondHelmet(),1, 1});
    m_allShopItems.push_back({Equipment::DiamondArmor(), 1, 1});
    m_allShopItems.push_back({Equipment::DiamondLeggings(),1, 1});
    m_allShopItems.push_back({Equipment::DiamondBoots(), 1, 1});

    RefreshShop();
}

void Shop::SetEngine(GameEngine* engine) {
    m_engine = engine;
}

void Shop::RefreshShop() {
    int currentDays = 0;
    int currentHours = 0;
    if (m_engine) {
        currentDays = m_engine->GetGameTime().Day;
        currentHours = m_engine->GetGameTime().Hour;
    }

    int currentCycle = currentDays;
    if (currentHours < 6) {
        currentCycle--;
    }

    if (currentCycle > lastRefreshDays) {
        std::vector<ShopItem> pool = m_allShopItems;
        for (int i = (int)pool.size() - 1; i > 0; --i) {
            int j = RNG::RandInt(0, i);
            std::swap(pool[i], pool[j]);
        }
        
        m_shopItems.clear();
        for (int i = 0; i < 15 && i < (int)pool.size(); ++i) {
            pool[i].stock = pool[i].maxStock;
            m_shopItems.push_back(pool[i]);
        }
        
        lastRefreshDays = currentCycle;
    }
}

const std::vector<ShopItem>& Shop::GetShopItems() {
    RefreshShop();
    return m_shopItems;
}

std::stringstream Shop::DisplayShop(std::shared_ptr<Character> player) {
    std::stringstream ss;
    ss << "\n=== 校园小卖部 ===" << '\n';
    if (m_shopItems.empty()) {
        ss << "商店里暂时没有物品。" << '\n';
        return ss;
    }

    bool hasDiscount = false;
    if (player) {
        for (const auto& item : player->GetBackpack().GetItems()) {
            if (item->getName() == "商店折扣券") {
                hasDiscount = true;
                break;
            }
        }
    }
    
    RefreshShop();
    for (size_t i = 0; i < m_shopItems.size(); ++i) {
        int price = m_shopItems[i].item->getValue();
        if (hasDiscount) {
            price = std::max(1, static_cast<int>(price * 0.8));
        }
        ss << i + 1 << ". " << m_shopItems[i].item->getName() 
           << " - 价格: " << price << " 金币";
        if (hasDiscount) ss << " (已享受8折优惠) ";
        ss << " (库存: " << m_shopItems[i].stock << "/" << m_shopItems[i].maxStock << ")\n"
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

    int price = shopItem.item->getValue();
    
    // 检查背包是否有折扣券
    bool hasDiscount = false;
    for (const auto& item : player->GetBackpack().GetItems()) {
        if (item->getName() == "商店折扣券") {
            hasDiscount = true;
            break;
        }
    }
    
    if (hasDiscount) {
        price = std::max(1, static_cast<int>(price * 0.8)); // 8折
    }

    if (player->GetGold() < price) {
        throw NoEnoughGoldException("金币不足，购买失败。");
    }

    player->SpendGold(price);
        
    std::unique_ptr<Item> newItem = shopItem.item->Clone();
    
    player->GetBackpack().AddItem(std::move(newItem));
    bool isHealItem = (dynamic_cast<Medicine*>(shopItem.item.get()) != nullptr) || (dynamic_cast<Food*>(shopItem.item.get()) != nullptr);
    if (m_engine) m_engine->GetTaskManager().OnItemBought(shopItem.item->getName(), isHealItem);
    shopItem.stock--;
    ss << "购买 " << shopItem.item->getName() << " 成功！" << '\n';
    if (hasDiscount) {
        ss << "（已应用折扣券，原价 " << shopItem.item->getValue() << "，折后价 " << price << "）\n";
    }
    return ss;
}
