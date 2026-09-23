//
// Search limits and time allocation.
//

#ifndef CHESS_TIMEMAN_H
#define CHESS_TIMEMAN_H

#pragma once

#include <chrono>
#include <cstdint>

#include "score.h"
#include "../lib/surge/src/types.h"

struct SearchLimits {
    int depth = MAX_PLY - 1;
    uint64_t nodes = 0;   // 0 = unlimited
    int64_t movetime = 0; // ms, 0 = not set
    int64_t time[2] = {-1, -1};
    int64_t inc[2] = {0, 0};
    int movestogo = 0;
    bool infinite = false;
};

class TimeManager {
public:
    int64_t move_overhead = 30;

    void start(const SearchLimits &limits, Color us);

    int64_t elapsed() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_time).count();
    }

    bool limited() const { return soft_limit > 0; }
    // Don't start another iteration after this; `scale` grows when the best move is unstable
    bool soft_exceeded(double scale = 1.0) const {
        return limited() && elapsed() >= (fixed_time ? hard_limit : int64_t(soft_limit * scale));
    }
    // Abort the running iteration
    bool hard_exceeded() const { return limited() && elapsed() >= hard_limit; }

private:
    std::chrono::steady_clock::time_point start_time;
    int64_t soft_limit = 0; // 0 = no time limit
    int64_t hard_limit = 0;
    bool fixed_time = false; // "go movetime": use all of it
};

#endif //CHESS_TIMEMAN_H
