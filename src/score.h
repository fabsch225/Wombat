//
// Score constants shared by search, TT and UCI output.
//

#ifndef CHESS_SCORE_H
#define CHESS_SCORE_H

#pragma once

constexpr int MAX_PLY = 128;

constexpr int INF = 32000;
constexpr int MATE = 31000;
constexpr int MATE_IN_MAX = MATE - MAX_PLY;
constexpr int NO_SCORE = 32001;

inline bool is_mate_score(int s) { return s >= MATE_IN_MAX || s <= -MATE_IN_MAX; }

#endif //CHESS_SCORE_H
