#include "Backpack.h"

void Backpack::AddItem(std::unique_ptr<Item> item) {
    if (item) {
        items.push_back(std::move(item));
    }
}

std::string Backpack::GetBackpackInfo() const {
    std::stringstream ss;
    ss << "====== 背包列表 ======\n";
    for (size_t i = 0; i < items.size(); ++i) {
        ss << "[" << i + 1 << "] " << items[i]->getName() << "\n"; // 多态调用
    }
    return ss.str();
}
