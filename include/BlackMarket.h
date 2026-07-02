#pragma once
#include "Item.h"
#include "Character.h"
#include "Common.h"

// ─────────────────────────────────────────────────────────────────────────────
// BlackMarket — 夜晚专属黑市商人
//
// 结构与 Shop 对称，但：
//   1. 只出售黑市专属道具（MidnightBBQ / NightWalkerCloak）
//   2. 无库存刷新机制（每次夜晚生成时重新初始化）
//   3. 不记录刷新时间（每夜重新到来即为全新黑市）
// ─────────────────────────────────────────────────────────────────────────────
struct BlackMarketItem {
    std::shared_ptr<Item> item;
    int stock;
    int maxStock;
};

class BlackMarket {
public:
    BlackMarket();
    ~BlackMarket() = default;

    // 重置并初始化黑市商品（每次夜晚到来时调用）
    void InitItems();

    // 展示黑市界面文本
    std::stringstream DisplayBlackMarket() const;

    // 购买物品（0-based 索引），返回结果文本
    std::stringstream BuyItem(std::shared_ptr<Character> player, int itemIndex);

    // 返回全部商品的只读引用（供 Qt 渲染列表）
    const std::vector<BlackMarketItem>& GetItems() const;

private:
    std::vector<BlackMarketItem> m_items;
};
