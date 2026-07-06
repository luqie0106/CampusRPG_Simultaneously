#pragma once

#include "Common.h"

class TimerManager {
public:
    static void StartCountdown(float durationSeconds, 
                               std::function<void(float)> onTick, 
                               std::function<void()> onComplete) {
        std::thread([durationSeconds, onTick, onComplete]() {
            auto startTime = std::chrono::steady_clock::now();
            float remaining = durationSeconds;

            while (remaining > 0.0f) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));

                auto currentTime = std::chrono::steady_clock::now();
                std::chrono::duration<float> elapsed = currentTime - startTime;
                remaining = durationSeconds - elapsed.count();

                if (remaining <= 0.0f) {
                    remaining = 0.0f;
                }

                if (onTick) {
                    onTick(remaining);
                }
            }

            if (onComplete) {
                onComplete();
            }
        }).detach();
    }
};
