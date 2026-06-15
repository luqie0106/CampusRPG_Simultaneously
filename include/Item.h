#pragma once
#include <string>
#include <iostream>


class Item {
private:
    std::string name;
    int value;

public:
    Item(std::string name, int value) : name(name), value(value) {}
    void Show() const {
        std::cout << name << " " << value << std::endl;
    }
    bool operator==(const Item& other) const {
        return name == other.name && value == other.value;
    }

};