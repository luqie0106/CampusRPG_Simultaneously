#include "Common.h"

#include "../include/RNG.h"

namespace RNG {

// 全局唯一的 mt19937 引擎（仅此文件内可见）
static std::mt19937 g_rng;

void Init() {
    // 用硬件随机设备生成种子，程序运行时调用一次
    std::random_device rd;
    g_rng.seed(rd());
}

int RandInt(int lo, int hi) {
    std::uniform_int_distribution<int> dist(lo, hi);
    return dist(g_rng);
}

double RandDouble(double lo, double hi) {
    std::uniform_real_distribution<double> dist(lo, hi);
    return dist(g_rng);
}

} // namespace RNG
