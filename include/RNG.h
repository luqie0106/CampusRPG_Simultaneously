#pragma once

#include "Common.h"

// ─────────────────────────────────────────────────────────────────────────────
// RNG — 全局随机数引擎（mt19937）
//
// 使用方式：
//   1. 程序启动时调用一次 RNG::Init()（在 GameEngine::Init() 中调用）
//   2. 之后所有随机数均通过 RNG::RandInt / RNG::RandDouble 生成
//      内部使用 std::uniform_int_distribution / std::uniform_real_distribution
// ─────────────────────────────────────────────────────────────────────────────
namespace RNG {
    // 用 std::random_device 生成种子并初始化全局 mt19937 引擎
    // 必须在使用任何 Rand* 函数之前调用一次
    void Init();

    // 返回 [lo, hi] 范围内的均匀随机整数（含两端）
    int RandInt(int lo, int hi);

    // 返回 [lo, hi) 范围内的均匀随机浮点数
    double RandDouble(double lo, double hi);
}
