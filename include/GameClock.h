#pragma once

#include "Common.h"

#include "GameTime.h"
// ─────────────────────────────────────────────────────────────────────────────
// GameClock — 游戏时间后台线程
//
// 每隔 1 真实秒，游戏时间推进 2 分钟（1 天 = 720 真实秒 ≈ 12 分钟）。
//
// 数据安全：
//   - m_time 由 m_mutex 保护；主线程读取时通过 GetTime() / IsNight() 加锁
//   - 回调（OnDayToNight / OnNightToDay）在后台线程中执行；
//     调用者负责保证回调内的操作是线程安全的
//
// 典型用法：
//   GameClock clock;
//   clock.SetOnDayToNight([this]{ _OnDayToNight(); });
//   clock.SetOnNightToDay([this]{ _OnNightToDay(); });
//   clock.Start();
//   ...
//   clock.Stop(); // 析构前必须调用
// ─────────────────────────────────────────────────────────────────────────────
class GameClock {
public:
    // 每次 tick 推进的游戏分钟数（1 真实秒 = 2 游戏分钟）
    static constexpr int MINUTES_PER_TICK = 2;
    // tick 间隔（毫秒）
    static constexpr int TICK_MS = 1000;

    GameClock();
    ~GameClock();

    // 启动后台时间线程；重复调用无效
    void Start();

    // 停止后台时间线程；阻塞直到线程退出
    void Stop();

    // 线程安全地读取当前游戏时间快照
    GameTime GetTime() const;

    // 线程安全地设置当前游戏时间（读档时使用）
    void SetTime(const GameTime& t);

    // 线程安全地查询当前是否为夜晚
    bool IsNight() const;

    // 注册昼→夜转换回调（每次进入夜晚时触发一次）
    void SetOnDayToNight(std::function<void()> cb);

    // 注册夜→昼转换回调（每次离开夜晚时触发一次）
    void SetOnNightToDay(std::function<void()> cb);

private:
    mutable std::mutex      m_mutex;      // 保护 m_time
    GameTime                m_time;       // 当前游戏时间（后台线程写，主线程读）
    std::atomic<bool>       m_running;    // 控制线程是否继续
    std::thread             m_thread;     // 后台时间推进线程

    std::function<void()>   m_onDayToNight;  // 昼→夜回调
    std::function<void()>   m_onNightToDay;  // 夜→昼回调
    bool                    m_wasNight;      // 上一 tick 是否为夜晚

    // 后台线程主循环
    void _Run();
};
