#pragma once

#include "Common.h"

// ─────────────────────────────────────────────────────────────────────────────
// GameTime — 游戏内时间结构体
//
// 规则：
//   真实世界 1 秒 = 游戏 2 分钟
//   1 游戏天 = 720 真实秒 = 12 分钟
//
// 夜晚定义：22:00（含）— 次日 06:00（不含）
// ─────────────────────────────────────────────────────────────────────────────
struct GameTime {
    int Day    = 1;   // 游戏天数（从第 1 天开始）
    int Hour   = 8;   // 小时（0–23）
    int Minute = 0;   // 分钟（0–59）

    // 返回是否处于夜晚时段（22:00 ≤ time 或 time < 06:00）
    bool IsNight() const {
        return Hour >= 22 || Hour < 6;
    }

    // 返回格式化时间字符串，例如 "第3天 22:05"
    std::string ToString() const {
        char buf[64];
        std::snprintf(buf, sizeof(buf),
                      "第%d天 %02d:%02d", Day, Hour, Minute);
        return std::string(buf);
    }

    // 推进指定分钟数（内部处理进位与天数递增）
    void Advance(int minutes) {
        Minute += minutes;
        while (Minute >= 60) {
            Minute -= 60;
            ++Hour;
        }
        while (Hour >= 24) {
            Hour -= 24;
            ++Day;
        }
    }
};
