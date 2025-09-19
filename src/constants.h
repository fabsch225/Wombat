//
// Created by fabian on 9/19/25.
//

#ifndef CHESS_CONSTANTS_H
#define CHESS_CONSTANTS_H

#include <bits/stdc++.h>

#pragma once

using u64 = uint64_t;
using i64 = long long;

enum Piece : int {
    NO_PIECE = 0,
    WP = 1, WN = 2, WB = 3, WR = 4, WQ = 5, WK = 6,
    BP = 7, BN = 8, BB = 9, BR = 10, BQ = 11, BK = 12
};

enum Color { WHITE = 0, BLACK = 1 };

static inline int piece_color(int p) {
    if (p == NO_PIECE) return -1;
    return (p <= 6 ? WHITE : BLACK);
}

#endif //CHESS_CONSTANTS_H