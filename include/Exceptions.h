#pragma once

#include "Common.h"

class GameException : public std::exception {
protected:
    std::string message;
public:
    explicit GameException(std::string msg) : message(std::move(msg)) {}
    const char* what() const noexcept override { return message.c_str(); }
};

class NoEnoughHpException : public GameException {
public:
    explicit NoEnoughHpException(std::string msg) : GameException(std::move(msg)) {}
};

class NoEnoughGoldException : public GameException {
public:
    explicit NoEnoughGoldException(std::string msg) : GameException(std::move(msg)) {}
};
