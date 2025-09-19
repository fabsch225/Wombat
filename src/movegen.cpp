//
// Created by fabian on 9/19/25.
//

#include "movegen.h"

using namespace std;

// knight / king attack tables
u64 KNIGHT_ATTACKS[64];
u64 KING_ATTACKS[64];

void init_kminor() {
    for (int sq = 0; sq < 64; ++sq) {
        u64 att = 0ULL;
        int r = sq / 8, f = sq % 8;
        const int d[8][2] = {{2, 1}, {1, 2}, {-1, 2}, {-2, 1}, {-2, -1}, {-1, -2}, {1, -2}, {2, -1}};
        for (auto &di: d) {
            int nr = r + di[0], nf = f + di[1];
            if (nr >= 0 && nr < 8 && nf >= 0 && nf < 8) att |= (1ULL << (nr * 8 + nf));
        }
        KNIGHT_ATTACKS[sq] = att;
        u64 ka = 0ULL;
        for (int dr = -1; dr <= 1; ++dr)
            for (int df = -1; df <= 1; ++df)
                if (dr || df) {
                    int nr = r + dr, nf = f + df;
                    if (nr >= 0 && nr < 8 && nf >= 0 && nf < 8) ka |= (1ULL << (nr * 8 + nf));
                }
        KING_ATTACKS[sq] = ka;
    }
}

bool is_square_attacked(const Board &bd, int sq, Color byColor) {
    // pawns
    if (byColor == WHITE) {
        u64 pawns = bd.pieces[WP];
        u64 attacks = ((pawns & ~0x0101010101010101ULL) << 7) | ((pawns & ~0x8080808080808080ULL) << 9);
        if (attacks & (1ULL << sq)) return true;
    } else {
        u64 pawns = bd.pieces[BP];
        u64 attacks = ((pawns & ~0x0101010101010101ULL) >> 9) | ((pawns & ~0x8080808080808080ULL) >> 7);
        if (attacks & (1ULL << sq)) return true;
    }
    // knights
    if (KNIGHT_ATTACKS[sq] & (byColor == WHITE ? bd.pieces[WN] : bd.pieces[BN])) return true;
    // bishops/queens
    if (bishop_attacks(sq, bd.occupancy) & (byColor == WHITE
                                                ? (bd.pieces[WB] | bd.pieces[WQ])
                                                : (bd.pieces[BB] | bd.pieces[BQ])))
        return true;
    // rooks/queens
    if (rook_attacks(sq, bd.occupancy) & (byColor == WHITE
                                              ? (bd.pieces[WR] | bd.pieces[WQ])
                                              : (bd.pieces[BR] | bd.pieces[BQ])))
        return true;
    // king
    if (KING_ATTACKS[sq] & (byColor == WHITE ? bd.pieces[WK] : bd.pieces[BK])) return true;
    return false;
}

void generate_moves(const Board &bd, vector<Move> &out) {
    out.clear();
    Color us = bd.side_to_move;
    Color them = (us == WHITE ? BLACK : WHITE);
    u64 own = bd.by_color[us];
    u64 opp = bd.by_color[them];
    u64 occ = bd.occupancy;
    // pawns
    if (us == WHITE) {
        u64 pawns = bd.pieces[WP];
        while (pawns) {
            int from = lsb_index(pawns);
            pawns &= pawns - 1;
            int r = from / 8, f = from % 8;
            int to = from + 8;

            // single push
            if (to < 64 && !(occ & (1ULL << to))) {
                if (to / 8 == 7) {
                    // promotion
                    for (int promoPiece: {WQ, WR, WB, WN}) {
                        Move m(from, to);
                        m.promo = promoPiece;
                        out.push_back(m);
                    }
                } else {
                    out.push_back(Move(from, to));

                    // double push
                    int to2 = from + 16;
                    if (r == 1 && !(occ & (1ULL << to2))) {
                        Move m2(from, to2);
                        m2.double_push = true;
                        out.push_back(m2);
                    }
                }
            }

            // captures
            if (f > 0) {
                int t = from + 7;
                if (t < 64 && (opp & (1ULL << t))) {
                    if (t / 8 == 7) {
                        for (int promoPiece: {WQ, WR, WB, WN}) {
                            Move m(from, t);
                            m.promo = promoPiece;
                            m.capture = true;
                            out.push_back(m);
                        }
                    } else {
                        Move m(from, t);
                        m.capture = true;
                        out.push_back(m);
                    }
                }
            }
            if (f < 7) {
                int t = from + 9;
                if (t < 64 && (opp & (1ULL << t))) {
                    if (t / 8 == 7) {
                        for (int promoPiece: {WQ, WR, WB, WN}) {
                            Move m(from, t);
                            m.promo = promoPiece;
                            m.capture = true;
                            out.push_back(m);
                        }
                    } else {
                        Move m(from, t);
                        m.capture = true;
                        out.push_back(m);
                    }
                }
            }

            // en passant
            if (bd.en_passant != -1 && r == 4) {
                if (abs((bd.en_passant % 8) - f) == 1) {
                    if (bd.en_passant == from + 7 || bd.en_passant == from + 9) {
                        Move m(from, bd.en_passant);
                        m.en_passant = true;
                        out.push_back(m);
                    }
                }
            }
        }
    } else {
        // black pawns
        u64 pawns = bd.pieces[BP];
        while (pawns) {
            int from = lsb_index(pawns);
            pawns &= pawns - 1;
            int r = from / 8, f = from % 8;
            int to = from - 8;

            // single push
            if (to >= 0 && !(occ & (1ULL << to))) {
                if (to / 8 == 0) {
                    // promotion
                    for (int promoPiece: {BQ, BR, BB, BN}) {
                        Move m(from, to);
                        m.promo = promoPiece;
                        out.push_back(m);
                    }
                } else {
                    out.push_back(Move(from, to));

                    // double push
                    int to2 = from - 16;
                    if (r == 6 && !(occ & (1ULL << to2))) {
                        Move m2(from, to2);
                        m2.double_push = true;
                        out.push_back(m2);
                    }
                }
            }

            // captures
            if (f > 0) {
                int t = from - 9;
                if (t >= 0 && (opp & (1ULL << t))) {
                    if (t / 8 == 0) {
                        for (int promoPiece: {BQ, BR, BB, BN}) {
                            Move m(from, t);
                            m.promo = promoPiece;
                            m.capture = true;
                            out.push_back(m);
                        }
                    } else {
                        Move m(from, t);
                        m.capture = true;
                        out.push_back(m);
                    }
                }
            }
            if (f < 7) {
                int t = from - 7;
                if (t >= 0 && (opp & (1ULL << t))) {
                    if (t / 8 == 0) {
                        for (int promoPiece: {BQ, BR, BB, BN}) {
                            Move m(from, t);
                            m.promo = promoPiece;
                            m.capture = true;
                            out.push_back(m);
                        }
                    } else {
                        Move m(from, t);
                        m.capture = true;
                        out.push_back(m);
                    }
                }
            }

            // en passant
            if (bd.en_passant != -1 && r == 3) {
                if (abs((bd.en_passant % 8) - f) == 1) {
                    if (bd.en_passant == from - 7 || bd.en_passant == from - 9) {
                        Move m(from, bd.en_passant);
                        m.en_passant = true;
                        out.push_back(m);
                    }
                }
            }
        }
    }

    // knights
    u64 knights = (us == WHITE ? bd.pieces[WN] : bd.pieces[BN]);
    while (knights) {
        int from = lsb_index(knights);
        knights &= knights - 1;
        u64 att = KNIGHT_ATTACKS[from] & ~own;
        while (att) {
            int to = lsb_index(att);
            att &= att - 1;
            Move m(from, to);
            m.capture = ((1ULL << to) & opp);
            out.push_back(m);
        }
    }
    // bishops
    u64 bishops = (us == WHITE ? bd.pieces[WB] : bd.pieces[BB]);
    while (bishops) {
        int from = lsb_index(bishops);
        bishops &= bishops - 1;
        u64 att = bishop_attacks(from, occ) & ~own;
        while (att) {
            int to = lsb_index(att);
            att &= att - 1;
            Move m(from, to);
            m.capture = ((1ULL << to) & opp);
            out.push_back(m);
        }
    }
    // rooks
    u64 rooks = (us == WHITE ? bd.pieces[WR] : bd.pieces[BR]);
    while (rooks) {
        int from = lsb_index(rooks);
        rooks &= rooks - 1;
        u64 att = rook_attacks(from, occ) & ~own;
        while (att) {
            int to = lsb_index(att);
            att &= att - 1;
            Move m(from, to);
            m.capture = ((1ULL << to) & opp);
            out.push_back(m);
        }
    }
    // queens
    u64 queens = (us == WHITE ? bd.pieces[WQ] : bd.pieces[BQ]);
    while (queens) {
        int from = lsb_index(queens);
        queens &= queens - 1;
        u64 att = queen_attacks(from, occ) & ~own;
        while (att) {
            int to = lsb_index(att);
            att &= att - 1;
            Move m(from, to);
            m.capture = ((1ULL << to) & opp);
            out.push_back(m);
        }
    }
    // king (including castling)
    u64 king = (us == WHITE ? bd.pieces[WK] : bd.pieces[BK]);
    int ksq = lsb_index(king);
    u64 kall = KING_ATTACKS[ksq] & ~own;
    u64 tmp = kall;
    while (tmp) {
        int to = lsb_index(tmp);
        tmp &= tmp - 1;
        Move m(ksq, to);
        m.capture = ((1ULL << to) & opp);
        out.push_back(m);
    }
    // castling: check rights and that squares between are empty and not attacked
    if (us == WHITE) {
        if (bd.can_castle_white_kingside) {
            if (!(bd.occupancy & ((1ULL << 5) | (1ULL << 6)))) {
                // squares e1,f1,g1 -> indices 4,5,6. king currently e1
                // ensure not in check and f1,g1 not attacked
                bool ok = true;
                Board tmpbd = bd;
                // ensure king not in check
                if (is_square_attacked(tmpbd, 4, BLACK)) ok = false;
                if (is_square_attacked(tmpbd, 5, BLACK)) ok = false;
                if (is_square_attacked(tmpbd, 6, BLACK)) ok = false;
                if (ok) {
                    Move m(4, 6);
                    m.castle = true;
                    out.push_back(m);
                }
            }
        }
        if (bd.can_castle_white_queenside) {
            if (!(bd.occupancy & ((1ULL << 1) | (1ULL << 2) | (1ULL << 3)))) {
                bool ok = true;
                Board tmpbd = bd;
                if (is_square_attacked(tmpbd, 4, BLACK)) ok = false;
                if (is_square_attacked(tmpbd, 3, BLACK)) ok = false;
                if (is_square_attacked(tmpbd, 2, BLACK)) ok = false;
                if (ok) {
                    Move m(4, 2);
                    m.castle = true;
                    out.push_back(m);
                }
            }
        }
    } else {
        if (bd.can_castle_black_kingside) {
            if (!(bd.occupancy & ((1ULL << 61) | (1ULL << 62)))) {
                bool ok = true;
                Board tmpbd = bd;
                if (is_square_attacked(tmpbd, 60, WHITE)) ok = false;
                if (is_square_attacked(tmpbd, 61, WHITE)) ok = false;
                if (is_square_attacked(tmpbd, 62, WHITE)) ok = false;
                if (ok) {
                    Move m(60, 62);
                    m.castle = true;
                    out.push_back(m);
                }
            }
        }
        if (bd.can_castle_black_queenside) {
            if (!(bd.occupancy & ((1ULL << 57) | (1ULL << 58) | (1ULL << 59)))) {
                bool ok = true;
                Board tmpbd = bd;
                if (is_square_attacked(tmpbd, 60, WHITE)) ok = false;
                if (is_square_attacked(tmpbd, 59, WHITE)) ok = false;
                if (is_square_attacked(tmpbd, 58, WHITE)) ok = false;
                if (ok) {
                    Move m(60, 58);
                    m.castle = true;
                    out.push_back(m);
                }
            }
        }
    }
}

// Check if move is legal: make it on a copy and verify king not in check
bool is_legal_move(const Board &bd, const Move &m) {
    Board copy = bd;
    Undo u;
    make_move(copy, m, u);
    // find king square of side that moved
    Color us = bd.side_to_move;
    int ksq = -1;
    u64 kings = copy.by_color[us] & (us == WHITE ? copy.pieces[WK] : copy.pieces[BK]);
    if (kings) ksq = lsb_index(kings);
    else return false;
    // if king is attacked by opponent then illegal
    if (is_square_attacked(copy, ksq, (us == WHITE ? BLACK : WHITE))) return false;
    return true;
}

vector<Move> legal_moves(const Board &bd) {
    vector<Move> all;
    generate_moves(bd, all);
    vector<Move> res;
    res.reserve(all.size());
    for (auto &m: all) { if (is_legal_move(bd, m)) res.push_back(m); }
    return res;
}
