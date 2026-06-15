#pragma once

#include <exception>
#include <string>


class NoEnoughHpException : public std::exception {
    private:
    std::string msg;
    public:
    NoEnoughHpException(std::string msg) : msg(msg) {}
    const char* what() const noexcept override {
        return msg.c_str();
    }
};