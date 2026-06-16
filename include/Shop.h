#pragma once
#include "Item.h"
#include "Character.h"
#include "Common.h"

struct ShopItem {
    std::shared_ptr<Item> item;
    int stock;
    int maxStock;
};

class Shop {
public:
    Shop();
    ~Shop();

    // 初始化商店物品
    void InitShopItems();

    // 展示商店界面
    std::stringstream DisplayShop();

    // 购买物品
    std::stringstream BuyItem(std::shared_ptr<Character> player, int itemIndex);

    // 刷新商店库存
    void RefreshShop();

private:
    std::vector<ShopItem> m_shopItems;
    std::chrono::system_clock::time_point lastRefreshTime;
};
