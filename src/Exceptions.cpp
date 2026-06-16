#include "../include/Exceptions.h"

NoEnoughHpException::NoEnoughHpException(std::string msg) : msg(msg) {}

const char* NoEnoughHpException::what() const noexcept {
    return msg.c_str();
}

NoEnoughGoldException::NoEnoughGoldException(std::string msg) : msg(msg) {}

const char* NoEnoughGoldException::what() const noexcept {
    return msg.c_str();
}
