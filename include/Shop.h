#pragma once
#include "Item.h"
#include "Character.h"
#include "Common.h"

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

private:
    std::vector<std::shared_ptr<Item>> m_shopItems;
};
