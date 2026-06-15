#pragma once

#include <vector>

template<typename T>
class Backpack {
private:
    std::vector<T> items;

public:
    Backpack() = default;
    void AddItem(const T& item) {
        items.push_back(item);
    }

    void RemoveItem(const T& item) {
        for (auto it = items.begin(); it != items.end(); it++) {
            if (*it == item) {
                items.erase(it);
                break;
            }
        }
    }

    void ShowItems() const {
        for (auto &item : items) {
            item.Show();
        }
    }

    void SortItemsByValue() {
        std::sort(items.begin(), items.end(), [](const T &a, const T &b) {
            return a.value > b.value;
        });
    }
};