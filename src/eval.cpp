//
// Created by fabian on 9/19/25.
//

#include "eval.h"

int piece_value(int p) {
    switch (p) {
        case WHITE_PAWN:
        case BLACK_PAWN: return 100;
        case WHITE_KNIGHT:
        case BLACK_KNIGHT: return 320;
        case WHITE_BISHOP:
        case BLACK_BISHOP: return 330;
        case WHITE_ROOK:
        case BLACK_ROOK: return 500;
        case WHITE_QUEEN:
        case BLACK_QUEEN: return 900;
        case WHITE_KING:
        case BLACK_KING: return 20000;
        default: return 0;
    }
}

template<Color Us>
int evaluate(const Position &p) {
    int score = 0;
    for (int sq = 0; sq < 64; ++sq) {
        auto pc = p.at(Square(sq));
        if (pc == NO_PIECE) continue;
        int v = piece_value(pc);
        if (color_of(pc) == Us) score += v;
        else score -= v;
    }
    return score;
}

template int evaluate<WHITE>(const Position &p);
template int evaluate<BLACK>(const Position &p);