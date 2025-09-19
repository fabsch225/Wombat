//
// Created by fabian on 9/19/25.
//

#include "eval.h"

int piece_value(int p) {
    switch (p) {
        case WP:
        case BP: return 100;
        case WN:
        case BN: return 320;
        case WB:
        case BB: return 330;
        case WR:
        case BR: return 500;
        case WQ:
        case BQ: return 900;
        case WK:
        case BK: return 20000;
        default: return 0;
    }
}

int evaluate(const Board &bd) {
    int score = 0;
    for (int sq = 0; sq < 64; ++sq) {
        int p = bd.sq_piece[sq];
        if (p == NO_PIECE) continue;
        int v = piece_value(p);
        if (piece_color(p) == WHITE) score += v;
        else score -= v;
    }
    // small piece-square/king safety can be added but keep basic
    return (bd.side_to_move == WHITE ? score : -score); // perspective: positive means side to move advantage
}