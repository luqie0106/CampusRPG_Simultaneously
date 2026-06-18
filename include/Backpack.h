#pragma once
#include "Common.h" // 里面包含了 <vector> 和 <memory>
#include "Item.h"   // 引入物品基类

// 前向声明：避免 Backpack.h → Character.h → Backpack.h 的循环包含
class Character;

class Backpack {
private:
    // 整个大作业的核心：用基类智能指针的 vector 容纳所有派生类物品
    std::vector<std::unique_ptr<Item>> items; 

public:
    Backpack() = default;

    // 放入物品（使用 std::move 转移智能指针所有权）
    void AddItem(std::unique_ptr<Item> item);

    // 使用物品（index 为 1-based）：根据类型对 player 产生效果，使用后从背包移除
    std::string UseItem(int index, Character& player);

    // 丢弃物品（index 为 1-based）
    std::string RemoveItem(int index);

    // 返回背包内物品数量
    int GetSize() const;

    // 返回背包内全部物品的只读引用（供 Qt 逐格绘制）
    // 使用 Item* 多态可判断子类型（dynamic_cast）
    const std::vector<std::unique_ptr<Item>>& GetItems() const;

    // 返回序号为 i（1-based）的物品信息字符串
    std::string GetItemInfo(int index) const;

    // 返回序号为 i（1-based）的物品原始价值（供出售计算用）
    // 索引无效时返回 -1
    int GetItemValue(int index) const;

    // 返回序号为 i（1-based）的物品名称
    // 索引无效时返回空字符串
    std::string GetItemName(int index) const;

    // 返回背包展示内容（带编号列表）
    std::string GetBackpackInfo() const;
};