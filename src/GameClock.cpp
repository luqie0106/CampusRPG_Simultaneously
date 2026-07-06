#include "Common.h"

#include "../include/GameClock.h"
// ─────────────────────────────────────────────────────────────────────────────
// 构造 / 析构
// ─────────────────────────────────────────────────────────────────────────────

GameClock::GameClock()
    : m_running(false), m_wasNight(false) {
    // 游戏从第 1 天 08:00 开始（白天）
    m_time = GameTime{ 1, 8, 0 };
}

GameClock::~GameClock() {
    Stop();   // 确保析构时线程已退出
}

// ─────────────────────────────────────────────────────────────────────────────
// 启动 / 停止
// ─────────────────────────────────────────────────────────────────────────────

void GameClock::Start() {
    if (m_running.exchange(true)) return;   // 已在运行，幂等
    m_wasNight = m_time.IsNight();
    m_thread   = std::thread(&GameClock::_Run, this);
}

void GameClock::Stop() {
    if (!m_running.exchange(false)) return; // 已停止，幂等
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// 线程安全读取接口
// ─────────────────────────────────────────────────────────────────────────────

GameTime GameClock::GetTime() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_time;
}

bool GameClock::IsNight() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_time.IsNight();
}

// ─────────────────────────────────────────────────────────────────────────────
// 回调注册
// ─────────────────────────────────────────────────────────────────────────────

void GameClock::SetOnDayToNight(std::function<void()> cb) {
    m_onDayToNight = std::move(cb);
}

void GameClock::SetOnNightToDay(std::function<void()> cb) {
    m_onNightToDay = std::move(cb);
}

// ─────────────────────────────────────────────────────────────────────────────
// 后台线程主循环
//
// 每隔 TICK_MS 毫秒睡一次，推进游戏时间 MINUTES_PER_TICK 分钟。
// 检测昼夜切换并触发对应回调（回调在本线程中同步执行）。
// ─────────────────────────────────────────────────────────────────────────────

void GameClock::_Run() {
    while (m_running.load()) {
        // 精确睡眠：用 steady_clock 避免系统时间跳变导致的漂移
        std::this_thread::sleep_for(
            std::chrono::milliseconds(TICK_MS));

        if (!m_running.load()) break;   // 醒来后再检查一次，防止 Stop() 竞争

        bool nowNight;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_time.Advance(MINUTES_PER_TICK);
            nowNight = m_time.IsNight();
        }

        // ── 昼夜切换检测（在锁外执行回调，避免死锁）──────────────────
        if (!m_wasNight && nowNight) {
            // 白天 → 夜晚
            if (m_onDayToNight) m_onDayToNight();
        } else if (m_wasNight && !nowNight) {
            // 夜晚 → 白天
            if (m_onNightToDay) m_onNightToDay();
        }
        m_wasNight = nowNight;
    }
}
