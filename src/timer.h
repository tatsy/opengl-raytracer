#pragma once

#include <chrono>

using time_type = std::chrono::time_point<std::chrono::system_clock>;

class Timer {
public:
    Timer() {
    }

    void start() {
        start_ = tick();
    }

    double count() {
        stop_ = tick();
        return to_duration(start_, stop_);
    }

    void reset() {
        start_ = tick();
    }

private:
    time_type tick() const {
        return std::chrono::system_clock::now();
    }

    double to_duration(time_type start_point, time_type end_point) const {
        return std::chrono::duration_cast<std::chrono::microseconds>(end_point - start_point).count() / 1.0e6;
    }

    time_type start_;
    time_type stop_;
};
