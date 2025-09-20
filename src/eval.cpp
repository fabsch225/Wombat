//
// Created by fabian on 9/19/25.
//

#include "eval.h"

#include <array>

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

static const int pawn_table[64] = {
    0,  0,  0,  0,  0,  0,  0,  0,
   50, 50, 50, 50, 50, 50, 50, 50,
   10, 10, 20, 30, 30, 20, 10, 10,
    5,  5, 10, 25, 25, 10,  5,  5,
    0,  0,  0, 20, 20,  0,  0,  0,
    5, -5,-10,  0,  0,-10, -5,  5,
    5, 10, 10,-20,-20, 10, 10,  5,
    0,  0,  0,  0,  0,  0,  0,  0
};

static const int knight_table[64] = {
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50
};

static const int bishop_table[64] = {
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -20,-10,-10,-10,-10,-10,-10,-20
};

template<Color Us>
int evaluate(const Position &p) {
    int score = 0;

    // Track pawns by file for connected pawn detection
    std::array<std::vector<int>, 8> pawn_files_white;
    std::array<std::vector<int>, 8> pawn_files_black;

    for (int sq = 0; sq < 64; ++sq) {
        auto pc = p.at(Square(sq));
        if (pc == NO_PIECE) continue;

        int v = piece_value(pc);
        int piece_score = v;

        int idx = (color_of(pc) == WHITE ? sq : (63 - sq));

        // Add piece-square table bonuses
        switch (type_of(pc)) {
            case PAWN:   piece_score += 0.05 * pawn_table[idx];   break;
            case KNIGHT: piece_score += 0.05 * knight_table[idx]; break;
            case BISHOP: piece_score += 0.05 * bishop_table[idx]; break;
            default: break;
        }

        // Collect pawn info for structure bonuses
        if (type_of(pc) == PAWN) {
            int file = sq % 8;
            int rank = sq / 8;
            if (color_of(pc) == WHITE) pawn_files_white[file].push_back(rank);
            else pawn_files_black[file].push_back(rank);
        }

        if (color_of(pc) == Us) score += piece_score;
        else score -= piece_score;
    }

    // Connected pawn bonus
    auto connected_pawns = [&](const std::array<std::vector<int>, 8>& pawns, Color c) {
        int bonus = 0;
        for (int f = 0; f < 8; ++f) {
            if (pawns[f].empty()) continue;
            for (int r : pawns[f]) {
                // Check left file
                if (f > 0) {
                    for (int r2 : pawns[f-1]) {
                        if (std::abs(r - r2) <= 1) {
                            bonus += 15 + r * 2; // reward more if advanced
                        }
                    }
                }
                // Check right file
                if (f < 7) {
                    for (int r2 : pawns[f+1]) {
                        if (std::abs(r - r2) <= 1) {
                            bonus += 15 + r * 2;
                        }
                    }
                }
            }
        }
        return bonus;
    };

    int conn_white = 0.05 * connected_pawns(pawn_files_white, WHITE);
    int conn_black = 0.05 * connected_pawns(pawn_files_black, BLACK);

    if (Us == WHITE) score += conn_white - conn_black;
    else score += conn_black - conn_white;

    return score;
}
template int evaluate<WHITE>(const Position &p);
template int evaluate<BLACK>(const Position &p);