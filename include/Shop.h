#pragma once
#include "Item.h"
#include "Character.h"
#include "Common.h"

class GameEngine;

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

    // 返回商店内全部商品的只读引用（供 Qt 不解析字符串直接渲染列表）
    const std::vector<ShopItem>& GetShopItems() const;

    // 购买物品
    std::stringstream BuyItem(std::shared_ptr<Character> player, int itemIndex);

    // 刷新商店库存
    void RefreshShop();

    void SetEngine(GameEngine* engine);

private:
    GameEngine* m_engine = nullptr;
    std::vector<ShopItem> m_shopItems;
    std::vector<ShopItem> m_allShopItems;
    int lastRefreshDays;
};
