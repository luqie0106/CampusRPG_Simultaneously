#pragma once
#include "Common.h" // 里面包含了 <vector> 和 <memory>
#include "Item.h"   // 引入物品基类

class Backpack {
private:
    // 整个大作业的核心：用基类智能指针的 vector 容纳所有派生类物品
    std::vector<std::unique_ptr<Item>> items; 

public:
    Backpack() = default;

    // 放入物品（使用 std::move 转移智能指针所有权）
    void AddItem(std::unique_ptr<Item> item);

    // 这里用到了 std::stringstream，Common.h 警告立马消失！
    std::string GetBackpackInfo() const;
};