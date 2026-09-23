//
// Search limits and time allocation.
//

#include "timeman.h"

#include <algorithm>

void TimeManager::start(const SearchLimits &limits, Color us) {
    start_time = std::chrono::steady_clock::now();
    soft_limit = hard_limit = 0;
    fixed_time = false;

    if (limits.infinite) return;

    if (limits.movetime > 0) {
        soft_limit = hard_limit = std::max<int64_t>(1, limits.movetime - move_overhead);
        fixed_time = true;
        return;
    }

    if (limits.time[us] < 0) return;

    const int64_t available = std::max<int64_t>(1, limits.time[us] - move_overhead);
    const int moves_to_go = limits.movestogo > 0 ? std::min(limits.movestogo, 40) : 30;

    int64_t base = available / moves_to_go + limits.inc[us] * 3 / 4;
    soft_limit = std::max<int64_t>(1, std::min<int64_t>(base, available / 2));
    hard_limit = std::max<int64_t>(1, std::min<int64_t>(soft_limit * 4, available * 3 / 4));
}
