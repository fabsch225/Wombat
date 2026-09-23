//
// Move ordering: scores all legal moves once, then hands them out best-first.
//

#ifndef CHESS_MOVEPICK_H
#define CHESS_MOVEPICK_H

#pragma once

#include <cstdint>
#include <cstdlib>
#include <utility>

#include "board.h"
#include "eval.h"
#include "see.h"

// Butterfly history of quiet moves, indexed [color][from][to]
using HistoryTable = int16_t[2][64][64];

constexpr int HISTORY_MAX = 16384;

inline void update_history(int16_t &entry, int bonus) {
    // Gravity: keeps values bounded and lets old information decay
    entry += bonus - entry * std::abs(bonus) / HISTORY_MAX;
}

class MovePicker {
public:
    enum Stage : int {
        SCORE_TT = 30'000'000,
        SCORE_GOOD_CAPTURE = 20'000'000,
        SCORE_PROMOTION = 19'000'000,
        SCORE_KILLER1 = 18'000'000,
        SCORE_KILLER2 = 17'000'000,
        SCORE_COUNTER = 16'000'000,
        SCORE_BAD_CAPTURE = -20'000'000,
    };

    // Main search: every legal move. Quiescence (captures_only): captures and queen promotions.
    MovePicker(Position &p, Move tt_move, const Move *killers, Move counter,
               const HistoryTable &history, bool captures_only) {
        Move *end = p.turn() == WHITE ? p.generate_legals<WHITE>(moves) : p.generate_legals<BLACK>(moves);
        total = int(end - moves);
        for (int i = 0; i < total; ++i) {
            Move m = moves[i];
            if (captures_only && !board::is_capture(m) &&
                !(board::is_promotion(m) && board::promotion_type(m) == QUEEN)) {
                continue;
            }
            moves[count] = m;
            scores[count++] = score(p, m, tt_move, killers, counter, history);
        }
    }

    // Number of legal moves in the position (independent of the captures_only filter)
    int legal_count() const { return total; }

    bool next(Move &m, int &move_score) {
        if (cur >= count) return false;
        int best = cur;
        for (int i = cur + 1; i < count; ++i)
            if (scores[i] > scores[best]) best = i;
        std::swap(moves[cur], moves[best]);
        std::swap(scores[cur], scores[best]);
        m = moves[cur];
        move_score = scores[cur++];
        return true;
    }

private:
    int score(const Position &p, Move m, Move tt_move, const Move *killers, Move counter,
              const HistoryTable &history) const {
        if (m == tt_move) return SCORE_TT;

        if (board::is_capture(m)) {
            int victim = PIECE_VALUE[board::captured_type(p, m)];
            int attacker = PIECE_VALUE[type_of(p.at(m.from()))];
            int mvv_lva = victim * 16 - attacker / 16;
            if (board::is_promotion(m)) mvv_lva += PIECE_VALUE[board::promotion_type(m)] * 16;
            return (see_ge(p, m, 0) ? SCORE_GOOD_CAPTURE : SCORE_BAD_CAPTURE) + mvv_lva;
        }
        if (board::is_promotion(m)) {
            return board::promotion_type(m) == QUEEN ? SCORE_PROMOTION : SCORE_BAD_CAPTURE - 1;
        }
        if (killers) {
            if (m == killers[0]) return SCORE_KILLER1;
            if (m == killers[1]) return SCORE_KILLER2;
        }
        if (m == counter) return SCORE_COUNTER;
        return history[color_of(p.at(m.from()))][m.from()][m.to()];
    }

    Move moves[218];
    int scores[218];
    int count = 0;
    int total = 0;
    int cur = 0;
};

#endif //CHESS_MOVEPICK_H
