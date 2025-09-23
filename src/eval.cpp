//
// Created by fabian on 9/19/25.
//

#include "eval.h"

#include <array>

int piece_value(int p) {
    switch (p) {
        case WHITE_PAWN:
        case BLACK_PAWN: return 1000;
        case WHITE_KNIGHT:
        case BLACK_KNIGHT: return 3200;
        case WHITE_BISHOP:
        case BLACK_BISHOP: return 3300;
        case WHITE_ROOK:
        case BLACK_ROOK: return 5000;
        case WHITE_QUEEN:
        case BLACK_QUEEN: return 9000;
        case WHITE_KING:
        case BLACK_KING: return 200000;
        default: return 0;
    }
}

static const int pawn_table[64] = {
    0,  0,  0,  0,  0,  0,  0,  0,
   50, 50, 50, 50, 50, 50, 50, 50,
   10, 10, 20, 30, 30, 20, 10, 10,
    5,  5, 10, 25, 25, 10,  5,  5,
    0,  5,  5, 20, 20,  5,  5,  5,
    5,  5,  0,  0,  0,  0,  5,  5,
    5,  5, 10,-20,-20, 10,  5,  5,
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
    -20,  0,  0,  0,  0,  0,  0,-20,
      0, 10,  0,  5,  5,  0, 10,  0,
      5,  0,  0,  5,  5,  0,  0,  5,
      0,  0,  5,  0,  0,  5,  0,  0,
      0,  5,  0,  0,  0,  0,  5,  0,
      0,  0,  0,  0,  0,  0,  0,  5,
      0, 10,  0,  5,  5,  0,  10, 0,
    -20,  0,  0,  0,  0, -20,  0,-20
};

static const int rook_table[64] = {
      5,  5,  5,  5,  5,  5,  5,  5,
      0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,
      5,  5,  5,  5,  5,  5,  5,  5,
};

static const int queen_table[64] = {
    0,  5,  5,  0,  0,  5,  5,  0,
    5,  5,  5,  0,  0,  5,  5,  5,
    5,  5,  5,  0,  0,  5,  5,  5,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    5,  5,  5,  0,  0,  5,  5,  5,
    5,  5,  5,  0,  0,  5,  5,  5,
    0,  5,  5,  0,  0,  5,  5,  0,
};

static const int king_table[64] = {
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    5, 40, 40,  0,  0,  5, 80,  5,
};

template<Color Us>
int evaluate(Position &p) {
    int score = 0;

    // Track pawns by file for connected pawn detection
    std::array<std::vector<int>, 8> pawn_files_white;
    std::array<std::vector<int>, 8> pawn_files_black;
    for (int sq = 0; sq < 64; ++sq) {
        auto pc = p.at(Square(sq));
        if (pc == NO_PIECE) continue;
        int v = piece_value(pc);
        int piece_score = v;

        int idx = (color_of(pc) == BLACK ? sq : (63 - sq));

        // Add piece-square table bonuses
        switch (type_of(pc)) {
            case PAWN:   piece_score += 0.2 * pawn_table[idx];   break;
            case KNIGHT: piece_score += 0.2 * knight_table[idx]; break;
            case BISHOP: piece_score += 0.2 * bishop_table[idx]; break;
            case ROOK:   piece_score += 0.2 * rook_table[idx];   break;
            case QUEEN:  piece_score += 0.2 * queen_table[idx];  break;
            case KING:   piece_score += 0.4 * king_table[idx];   break;
            default: break;
        }

        if (color_of(pc) == Us) score += piece_score;
        else score -= piece_score;
    }

    // If we're up a rook, soft strategic bonuses hardly matter
    if (score > 5000) {
        return score;
    }

    for (int sq = 0; sq < 64; ++sq) {
        auto pc = p.at(Square(sq));
        int piece_score = 0;
        // Collect pawn info for structure bonuses
        if (type_of(pc) == PAWN) {
            int file = sq % 8;
            int rank = sq / 8;
            if (color_of(pc) == WHITE) pawn_files_white[file].push_back(rank);
            else pawn_files_black[file].push_back(rank);
        }

        bool defended = false;
        Bitboard defenders;
        if (color_of(pc) == WHITE)
            defenders = p.attackers_from<WHITE>(Square(sq), p.all_pieces<WHITE>() | p.all_pieces<BLACK>());
        else
            defenders = p.attackers_from<BLACK>(Square(sq), p.all_pieces<WHITE>() | p.all_pieces<BLACK>());
        // Remove self from defenders
        defenders &= ~(Bitboard(1ULL) << sq);
        if (defenders) defended = true;

        if (defended) {
            int defend_bonus = 0;
            switch (type_of(pc)) {
                case ROOK: defend_bonus = 20; break;
                case QUEEN: defend_bonus = 10; break;
                case BISHOP: defend_bonus = 20; break;
                case KNIGHT: defend_bonus = 20; break;
                case PAWN: defend_bonus = 10; break;
                default: break;
            }
            piece_score += 0.2 * defend_bonus;
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

    int conn_white = 0.1 * connected_pawns(pawn_files_white, WHITE);
    int conn_black = 0.1 * connected_pawns(pawn_files_black, BLACK);

    if (Us == WHITE) score += conn_white - conn_black;
    else score += conn_black - conn_white;

    auto count_bits = [&](Bitboard b) {
        int c = 0;
        while (b) { pop_lsb(&b); ++c; }
        return c;
    };

    const Bitboard all = p.all_pieces<WHITE>() | p.all_pieces<BLACK>();
    const Bitboard w_all = p.all_pieces<WHITE>();
    const Bitboard b_all = p.all_pieces<BLACK>();

    const Square wksq = bsf(p.bitboard_of(WHITE, KING));
    const Square bksq = bsf(p.bitboard_of(BLACK, KING));

    // Pins: pieces pinned to their own kings
    Bitboard pinned_white = p.get_pinned<WHITE>(wksq, w_all, all);
    Bitboard pinned_black = p.get_pinned<BLACK>(bksq, b_all, all);

    auto pin_score_for = [&](Color c, Bitboard pinned_bb) {
        int s = 0;
        // Heavier penalty/bonus for minors, then rooks, then pawns, queens lowest
        if (pinned_bb) {
            s += 20 * count_bits(pinned_bb & p.bitboard_of(c, KNIGHT));
            s += 20 * count_bits(pinned_bb & p.bitboard_of(c, BISHOP));
            s += 15 * count_bits(pinned_bb & p.bitboard_of(c, ROOK));
            s += 10 * count_bits(pinned_bb & p.bitboard_of(c, PAWN));
            s += 8  * count_bits(pinned_bb & p.bitboard_of(c, QUEEN));
        }
        return s;
    };

    int pins_w = int(0.5 * pin_score_for(WHITE, pinned_white));
    int pins_b = int(0.5 * pin_score_for(BLACK, pinned_black));

    if (Us == WHITE) score += pins_b - pins_w;
    else score += pins_w - pins_b;

    // Pseudo-legal mobility using attack bitboards
    auto mobility_for = [&](Color c) {
        const Bitboard own = (c == WHITE ? w_all : b_all);
        const Bitboard opp = (c == WHITE ? b_all : w_all);
        int m = 0;

        Bitboard b;

        // Knights
        b = p.bitboard_of(c, KNIGHT);
        while (b) {
            Square s = pop_lsb(&b);
            Bitboard t = attacks<KNIGHT>(s, all) & ~own;
            m += count_bits(t) * 4;
        }

        // Bishops
        b = p.bitboard_of(c, BISHOP);
        while (b) {
            Square s = pop_lsb(&b);
            Bitboard t = attacks<BISHOP>(s, all) & ~own;
            m += count_bits(t) * 4;
        }

        // Rooks
        b = p.bitboard_of(c, ROOK);
        while (b) {
            Square s = pop_lsb(&b);
            Bitboard t = attacks<ROOK>(s, all) & ~own;
            m += count_bits(t) * 2;
        }

        // Queens (rook + bishop attacks)
        b = p.bitboard_of(c, QUEEN);
        while (b) {
            Square s = pop_lsb(&b);
            Bitboard t = (attacks<BISHOP>(s, all) | attacks<ROOK>(s, all)) & ~own;
            m += count_bits(t) * 1;
        }

        // Optionally include small pawn attack mobility (very light)
        Bitboard pawns = p.bitboard_of(c, PAWN);
        Bitboard pawn_att = (c == WHITE ? pawn_attacks<WHITE>(pawns) : pawn_attacks<BLACK>(pawns)) & ~own;
        m += count_bits(pawn_att) * 0.5;

        // Bitboard k = p.bitboard_of(c, KING);
        // if (k) m += count_bits(attacks<KING>(bsf(k), all) & ~own) * 1;

        return m;
    };

    int mob_w = int(0.2 * mobility_for(WHITE));
    int mob_b = int(0.2 * mobility_for(BLACK));

    if (Us == WHITE) score += mob_w - mob_b;
    else score += mob_b - mob_w;

    auto king_threat_for = [&](Color c) {
        Color oc = (c == WHITE ? BLACK : WHITE);
        Square ksq = bsf(p.bitboard_of(c, KING));
        if (ksq == Square(-1)) return 0; // no king (shouldn't happen)

        int threat = 0;
        const Bitboard occ = all;
        const Bitboard opp_rooks  = p.bitboard_of(oc, ROOK);
        const Bitboard opp_bishops= p.bitboard_of(oc, BISHOP);
        const Bitboard opp_queens = p.bitboard_of(oc, QUEEN);
        const Bitboard opp_sliders = opp_rooks | opp_bishops | opp_queens;

        // Pawn shield: three squares in front of the king (center file and adjacent files)
        auto file_of = [](Square s){ return int(s) % 8; };
        auto rank_of = [](Square s){ return int(s) / 8; };
        int kf = file_of(ksq), kr = rank_of(ksq);
        int shield_count = 0;
        if (c == WHITE) {
            int shield_rank = kr + 1;
            if (shield_rank <= 7) {
                for (int df = -1; df <= 1; ++df) {
                    int f = kf + df;
                    if (f < 0 || f > 7) continue;
                    Square sq = Square(shield_rank * 8 + f);
                    auto pc = p.at(sq);
                    if (pc != NO_PIECE && color_of(pc) == WHITE && type_of(pc) == PAWN) ++shield_count;
                }
            }
        } else { // BLACK
            int shield_rank = kr - 1;
            if (shield_rank >= 0) {
                for (int df = -1; df <= 1; ++df) {
                    int f = kf + df;
                    if (f < 0 || f > 7) continue;
                    Square sq = Square(shield_rank * 8 + f);
                    auto pc = p.at(sq);
                    if (pc != NO_PIECE && color_of(pc) == BLACK && type_of(pc) == PAWN) ++shield_count;
                }
            }
        }
        // Penalize missing shield pawns
        threat += (3 - shield_count) * 30;

        // Get all attackers to the king and restrict to sliding attackers
        Bitboard attackers = (oc == WHITE)
            ? p.attackers_from<WHITE>(ksq, occ)
            : p.attackers_from<BLACK>(ksq, occ);
        Bitboard slider_attackers = attackers & opp_sliders;

        // Helper to step from a to b along a straight line and count blockers and pawn-blockers
        auto between_info = [&](Square a, Square b) {
            int blockers = 0;
            int pawn_blockers = 0;
            int af = int(a) % 8, ar = int(a) / 8;
            int bf = int(b) % 8, br = int(b) / 8;
            int df = bf - af;
            int dr = br - ar;
            int stepf = 0, stepr = 0;
            if (df == 0) stepf = 0; else stepf = (df > 0 ? 1 : -1);
            if (dr == 0) stepr = 0; else stepr = (dr > 0 ? 1 : -1);
            // must be aligned (rook or bishop line)
            if (!(df == 0 || dr == 0 || std::abs(df) == std::abs(dr))) return std::pair<int,int>(-1,-1);
            int step = stepr * 8 + stepf;
            int sq = int(a) + step;
            while (sq != int(b)) {
                auto pc = p.at(Square(sq));
                if (pc != NO_PIECE) {
                    ++blockers;
                    if (type_of(pc) == PAWN) ++pawn_blockers;
                }
                sq += step;
            }
            return std::pair<int,int>(blockers, pawn_blockers);
        };

        // Evaluate each slider attacker
        Bitboard b = slider_attackers;
        while (b) {
            Square s = pop_lsb(&b);
            // Only consider true sliding directions (ensure aligned)
            auto [blockers, pawn_blockers] = between_info(s, ksq);
            if (blockers == -1) continue;
            int df = std::max(std::abs(int(s) % 8 - kf), std::abs(int(s) / 8 - kr)); // chebyshev distance
            int base = 0;
            auto pc = p.at(s);
            int piece_weight = (type_of(pc) == QUEEN) ? 180 : 120; // queen more dangerous
            if (pawn_blockers == 0) {
                // unobstructed: strong threat, closer attackers weigh more
                base = piece_weight + (3 - std::min(3, df)) * 40;
            } else {
                // blocked by pawns: weaker threat
                base = (piece_weight / 3) + (3 - std::min(3, df)) * 10;
            }
            // If there are additional non-pawn blockers, reduce threat further
            base = base - std::min(blockers, 3) * 10;
            threat += std::max(0, base);
        }

        return threat;
    };

    int danger_w = 0.05 * king_threat_for(WHITE);
    int danger_b = 0.05 * king_threat_for(BLACK);

    if (Us == WHITE) score += danger_b - danger_w;
    else score += danger_w - danger_b;

    return score;
}
template int evaluate<WHITE>(Position &p);
template int evaluate<BLACK>(Position &p);