#pragma once

#include "Common.h"


class NoEnoughHpException : public std::exception {
    private:
    std::string msg;
    public:
    NoEnoughHpException(std::string msg);
    const char* what() const noexcept override;
};

class NoEnoughGoldException : public std::exception {
    private:
    std::string msg;
    public:
    NoEnoughGoldException(std::string msg);
    const char* what() const noexcept override;
};