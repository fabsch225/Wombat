//
// Created by fabian on 9/19/25.
//
// Tapered (midgame / endgame) hand-crafted evaluation.
//

#include "eval.h"

#include <algorithm>
#include <cstdlib>

#include "board.h"
#include "eval_tables.h"

using namespace eval_tables;
using board::popcount;

namespace {

struct Masks {
    Bitboard passed[2][64];      // squares in front of a pawn (own + adjacent files) that must be pawn-free
    Bitboard adjacent_files[8];
    Bitboard king_zone[64];      // king square plus its neighbours

    Masks() {
        for (int f = 0; f < 8; ++f) {
            adjacent_files[f] = 0;
            if (f > 0) adjacent_files[f] |= MASK_FILE[f - 1];
            if (f < 7) adjacent_files[f] |= MASK_FILE[f + 1];
        }
        for (int sq = 0; sq < 64; ++sq) {
            int f = sq % 8, r = sq / 8;
            passed[WHITE][sq] = passed[BLACK][sq] = 0;
            for (int df = -1; df <= 1; ++df) {
                int ff = f + df;
                if (ff < 0 || ff > 7) continue;
                for (int rr = r + 1; rr < 8; ++rr) passed[WHITE][sq] |= 1ULL << (rr * 8 + ff);
                for (int rr = r - 1; rr >= 0; --rr) passed[BLACK][sq] |= 1ULL << (rr * 8 + ff);
            }
            king_zone[sq] = 0;
            for (int dr = -1; dr <= 1; ++dr)
                for (int df = -1; df <= 1; ++df) {
                    int rr = r + dr, ff = f + df;
                    if (rr >= 0 && rr < 8 && ff >= 0 && ff < 8) king_zone[sq] |= 1ULL << (rr * 8 + ff);
                }
        }
    }
};

const Masks masks;

// Weight of an attacker on the enemy king zone, and the resulting penalty by total attack units
constexpr int KING_ATTACK_WEIGHT[6] = {0, 2, 2, 3, 5, 0};

inline int king_danger(int units) {
    units = std::min(units, 40);
    return units * units * 3 / 2;
}

struct Score {
    int mg = 0, eg = 0;
    void add(int m, int e) { mg += m; eg += e; }
};

inline int relative_rank(Color c, int sq) { return c == WHITE ? sq / 8 : 7 - sq / 8; }

inline int chebyshev(int a, int b) {
    return std::max(std::abs(a % 8 - b % 8), std::abs(a / 8 - b / 8));
}

template<Color Us>
Score evaluate_side(const Position &p, int &phase) {
    constexpr Color Them = ~Us;
    Score s;

    const Bitboard occ = p.all_pieces<WHITE>() | p.all_pieces<BLACK>();
    const Bitboard own = p.all_pieces<Us>();
    const Bitboard our_pawns = p.bitboard_of(Us, PAWN);
    const Bitboard their_pawns = p.bitboard_of(Them, PAWN);
    const Bitboard their_pawn_attacks = pawn_attacks<Them>(their_pawns);
    const Bitboard mobility_area = ~own & ~their_pawn_attacks;

    const Square their_king = board::lsb(p.bitboard_of(Them, KING));
    const Bitboard their_king_zone = masks.king_zone[their_king];
    int attack_units = 0, attackers = 0;

    // Material and piece-square tables
    for (int pt = PAWN; pt <= KING; ++pt) {
        Bitboard b = p.bitboard_of(Us, PieceType(pt));
        while (b) {
            int sq = pop_lsb(&b);
            int idx = Us == WHITE ? (sq ^ 56) : sq;
            s.add(MG_VALUE[pt] + MG_PST[pt][idx], EG_VALUE[pt] + EG_PST[pt][idx]);
            phase += PHASE_INC[pt];
        }
    }

    // Bishop pair
    if (popcount(p.bitboard_of(Us, BISHOP)) >= 2) s.add(30, 50);

    // Pawn structure
    Bitboard b = our_pawns;
    while (b) {
        int sq = pop_lsb(&b);
        int f = sq % 8;
        if (!(masks.passed[Us][sq] & their_pawns)) {
            int rr = relative_rank(Us, sq);
            int mg = PASSED_MG[rr], eg = PASSED_EG[rr];
            // Blocked passers are worth less
            Square front = Square(Us == WHITE ? sq + 8 : sq - 8);
            if (occ & SQUARE_BB[front]) { mg /= 2; eg /= 2; }
            // Endgame: king proximity to the passer matters
            Square our_king = board::lsb(p.bitboard_of(Us, KING));
            eg += (chebyshev(their_king, front) - chebyshev(our_king, front)) * rr * 2;
            s.add(mg, eg);
        }
        if (!(masks.adjacent_files[f] & our_pawns)) s.add(-10, -15); // isolated
    }
    for (int f = 0; f < 8; ++f) {
        int n = popcount(our_pawns & MASK_FILE[f]);
        if (n > 1) s.add(-10 * (n - 1), -20 * (n - 1)); // doubled
    }

    // Pieces: mobility, rook files, king attack
    b = p.bitboard_of(Us, KNIGHT);
    while (b) {
        Square sq = pop_lsb(&b);
        Bitboard att = attacks<KNIGHT>(sq, occ);
        int mob = popcount(att & mobility_area);
        s.add((mob - 4) * 4, (mob - 4) * 4);
        if (att & their_king_zone) { attack_units += KING_ATTACK_WEIGHT[KNIGHT] * popcount(att & their_king_zone); ++attackers; }
    }
    b = p.bitboard_of(Us, BISHOP);
    while (b) {
        Square sq = pop_lsb(&b);
        Bitboard att = attacks<BISHOP>(sq, occ);
        int mob = popcount(att & mobility_area);
        s.add((mob - 6) * 5, (mob - 6) * 5);
        if (att & their_king_zone) { attack_units += KING_ATTACK_WEIGHT[BISHOP] * popcount(att & their_king_zone); ++attackers; }
    }
    b = p.bitboard_of(Us, ROOK);
    while (b) {
        Square sq = pop_lsb(&b);
        Bitboard att = attacks<ROOK>(sq, occ);
        int mob = popcount(att & mobility_area);
        s.add((mob - 7) * 2, (mob - 7) * 4);
        Bitboard file = MASK_FILE[file_of(sq)];
        if (!(file & our_pawns)) {
            if (!(file & their_pawns)) s.add(25, 10); // open file
            else s.add(12, 5);                        // semi-open file
        }
        if (att & their_king_zone) { attack_units += KING_ATTACK_WEIGHT[ROOK] * popcount(att & their_king_zone); ++attackers; }
    }
    b = p.bitboard_of(Us, QUEEN);
    while (b) {
        Square sq = pop_lsb(&b);
        Bitboard att = attacks<QUEEN>(sq, occ);
        int mob = popcount(att & mobility_area);
        s.add((mob - 14) * 1, (mob - 14) * 2);
        if (att & their_king_zone) { attack_units += KING_ATTACK_WEIGHT[QUEEN] * popcount(att & their_king_zone); ++attackers; }
    }

    // A lone attacker is rarely dangerous; require the queen for a real attack
    if (attackers >= 2 && p.bitboard_of(Us, QUEEN)) s.add(king_danger(attack_units), 0);

    // Pawn shield in front of our own king (only matters in the midgame)
    const Square our_king = board::lsb(p.bitboard_of(Us, KING));
    if (relative_rank(Us, our_king) <= 1) {
        Bitboard shield_zone = (MASK_FILE[file_of(our_king)] | masks.adjacent_files[file_of(our_king)]) &
                               masks.passed[Us][our_king];
        Bitboard near = shield_zone & (Us == WHITE ? (MASK_RANK[rank_of(our_king) + 1] | MASK_RANK[rank_of(our_king) + 2])
                                                   : (MASK_RANK[rank_of(our_king) - 1] | MASK_RANK[rank_of(our_king) - 2]));
        int shield = popcount(near & our_pawns);
        s.add(std::min(shield, 3) * 12 - 24, 0);
    }

    return s;
}

// Pushes the losing king towards the edge / corner in won endgames without pawns
int mop_up(const Position &p, Color strong) {
    Square wk = board::lsb(p.bitboard_of(strong, KING));
    Square lk = board::lsb(p.bitboard_of(~strong, KING));
    int f = lk % 8, r = lk / 8;
    int center_dist = std::max(3 - f, f - 4) + std::max(3 - r, r - 4);
    return center_dist * 10 + (14 - (std::abs(wk % 8 - f) + std::abs(wk / 8 - r))) * 4;
}

int material(const Position &p, Color c) {
    int m = 0;
    for (int pt = KNIGHT; pt <= QUEEN; ++pt) m += popcount(p.bitboard_of(c, PieceType(pt))) * PIECE_VALUE[pt];
    return m;
}

} // namespace

int evaluate(const Position &p) {
    int phase = 0;
    Score w = evaluate_side<WHITE>(p, phase);
    Score b = evaluate_side<BLACK>(p, phase);

    int mg = w.mg - b.mg;
    int eg = w.eg - b.eg;

    // Endgame knowledge
    const int w_pawns = popcount(p.bitboard_of(WHITE_PAWN));
    const int b_pawns = popcount(p.bitboard_of(BLACK_PAWN));
    const int w_mat = material(p, WHITE), b_mat = material(p, BLACK);
    if (w_pawns == 0 && b_pawns == 0 && std::abs(w_mat - b_mat) >= PIECE_VALUE[ROOK])
        eg += (w_mat > b_mat ? 1 : -1) * mop_up(p, w_mat > b_mat ? WHITE : BLACK);

    phase = std::min(phase, 24);
    int score = (mg * phase + eg * (24 - phase)) / 24;

    // The stronger side without pawns cannot win with less than a rook more material
    Color strong = score > 0 ? WHITE : BLACK;
    int strong_pawns = strong == WHITE ? w_pawns : b_pawns;
    int diff = std::abs(w_mat - b_mat);
    if (strong_pawns == 0 && diff < PIECE_VALUE[ROOK]) score /= (diff <= PIECE_VALUE[BISHOP] ? 8 : 2);

    if (p.turn() == BLACK) score = -score;
    return score + 10; // tempo
}
