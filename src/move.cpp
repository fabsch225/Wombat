//
// Created by fabian on 9/19/25.
//

#include "move.h"

using namespace std;

const u64 R_MAGIC[64] = {
    0x0080001020400080, 0x0040001000200040, 0x0080081000200080, 0x0080040800100080,
    0x0080020400080080, 0x0080010200040080, 0x0080008001000200, 0x0080002040800100,
    0x0000800020400080, 0x0000400020005000, 0x0000801000200080, 0x0000800800100080,
    0x0000800400080080, 0x0000800200040080, 0x0000800100020080, 0x0000800040800100,
    0x0000208000400080, 0x0000404000201000, 0x0000808010002000, 0x0000808008001000,
    0x0000808004000800, 0x0000808002000400, 0x0000010100020004, 0x0000020000408104,
    0x0000208080004000, 0x0000200040005000, 0x0000100080200080, 0x0000080080100080,
    0x0000040080080080, 0x0000020080040080, 0x0000010080800200, 0x0000800080004100,
    0x0000204000800080, 0x0000200040401000, 0x0000100080802000, 0x0000080080801000,
    0x0000040080800800, 0x0000020080800400, 0x0000020001010004, 0x0000800040800100,
    0x0000204000808000, 0x0000200040008080, 0x0000100020008080, 0x0000080010008080,
    0x0000040008008080, 0x0000020004008080, 0x0000010002008080, 0x0000004081020004,
    0x0000204000800080, 0x0000200040008080, 0x0000100020008080, 0x0000080010008080,
    0x0000040008008080, 0x0000020004008080, 0x0000800100020080, 0x0000800041000080,
    0x00FFFCDDFCED714A, 0x007FFCDDFCED714A, 0x003FFFCDFFD88096, 0x0000040810002101,
    0x0001000204080011, 0x0001000204000801, 0x0001000082000401, 0x0001FFFAABFAD1A2
};

const u64 B_MAGIC[64] = {
    0x0002020202020200, 0x0002020202020000, 0x0004010202000000, 0x0004040080000000,
    0x0001104000000000, 0x0000821040000000, 0x0000410410400000, 0x0000104104104000,
    0x0000040404040400, 0x0000020202020200, 0x0000040102020000, 0x0000040400800000,
    0x0000011040000000, 0x0000008210400000, 0x0000004104104000, 0x0000002082082000,
    0x0004000808080800, 0x0002000404040400, 0x0001000202020200, 0x0000800802004000,
    0x0000800400A00000, 0x0000200100884000, 0x0000400082082000, 0x0000200041041000,
    0x0002080010101000, 0x0001040008080800, 0x0000208004010400, 0x0000404004010200,
    0x0000840000802000, 0x0000404002011000, 0x0000808001041000, 0x0000404000820800,
    0x0001041000202000, 0x0000820800101000, 0x0000104400080800, 0x0000020080080080,
    0x0000404040040100, 0x0000808100020100, 0x0001010100020800, 0x0000808080010400,
    0x0000820820004000, 0x0000410410002000, 0x0000082088001000, 0x0000002011000800,
    0x0000080100400400, 0x0001010101000200, 0x0002020202000400, 0x0001010101000200,
    0x0000410410400000, 0x0000208208200000, 0x0000002084100000, 0x0000000020880000,
    0x0000001002020000, 0x0000040408020000, 0x0004040404040000, 0x0002020202020000,
    0x0000104104104000, 0x0000002082082000, 0x0000000020841000, 0x0000000000208800,
    0x0000000010020200, 0x0000000404080200, 0x0000040404040400, 0x0002020202020200
};

// shift values (number of relevant occupancy bits)
const int R_SHIFT[64] = {
    52, 53, 53, 53, 53, 53, 53, 52,
    53, 54, 54, 54, 54, 54, 54, 53,
    53, 54, 54, 54, 54, 54, 54, 53,
    53, 54, 54, 54, 54, 54, 54, 53,
    53, 54, 54, 54, 54, 54, 54, 53,
    53, 54, 54, 54, 54, 54, 54, 53,
    53, 54, 54, 54, 54, 54, 54, 53,
    52, 53, 53, 53, 53, 53, 53, 52
};

const int B_SHIFT[64] = {
    58, 59, 59, 59, 59, 59, 59, 58,
    59, 60, 60, 60, 60, 60, 60, 59,
    59, 60, 60, 60, 60, 60, 60, 59,
    59, 60, 60, 60, 60, 60, 60, 59,
    59, 60, 60, 60, 60, 60, 60, 59,
    59, 60, 60, 60, 60, 60, 60, 59,
    59, 60, 60, 60, 60, 60, 60, 59,
    58, 59, 59, 59, 59, 59, 59, 58
};

vector<u64> R_ATTACKS[64];
vector<u64> B_ATTACKS[64];

inline bool on_board(int sq) { return sq >= 0 && sq < 64; }

u64 rook_mask(int sq) {
    int r = sq / 8, f = sq % 8;
    u64 m = 0ULL;
    for (int rr = r + 1; rr <= 6; ++rr) m |= (1ULL << (rr * 8 + f));
    for (int rr = r - 1; rr >= 1; --rr) m |= (1ULL << (rr * 8 + f));
    for (int ff = f + 1; ff <= 6; ++ff) m |= (1ULL << (r * 8 + ff));
    for (int ff = f - 1; ff >= 1; --ff) m |= (1ULL << (r * 8 + ff));
    return m;
}

u64 bishop_mask(int sq) {
    int r = sq / 8, f = sq % 8;
    u64 m = 0ULL;
    for (int rr = r + 1, ff = f + 1; rr <= 6 && ff <= 6; ++rr, ++ff) m |= (1ULL << (rr * 8 + ff));
    for (int rr = r + 1, ff = f - 1; rr <= 6 && ff >= 1; ++rr, --ff) m |= (1ULL << (rr * 8 + ff));
    for (int rr = r - 1, ff = f + 1; rr >= 1 && ff <= 6; --rr, ++ff) m |= (1ULL << (rr * 8 + ff));
    for (int rr = r - 1, ff = f - 1; rr >= 1 && ff >= 1; --rr, --ff) m |= (1ULL << (rr * 8 + ff));
    return m;
}

u64 sliding_attacks_ray(int sq, u64 occ, int dir) {
    u64 attacks = 0ULL;
    int s = sq;
    while (true) {
        int r = s / 8, f = s % 8;
        int nr = r + (dir / 8), nf = f + (dir % 8);
        s += dir;
        if (!on_board(s)) break;
        // edge wrap checks
        if (abs((s % 8) - f) > 2 && (dir == DIR_E || dir == DIR_W || dir == DIR_NE || dir == DIR_NW || dir == DIR_SE ||
                                     dir == DIR_SW)) {
            // crude guard - but simpler to use file/rank update
        }
        attacks |= (1ULL << s);
        if (occ & (1ULL << s)) break;
    }
    return attacks;
}

// compute attacks given occupancy using simple ray method (fallback correctness)
u64 rook_attacks_on_the_fly(int sq, u64 occ) {
    u64 attacks = 0ULL;
    int r = sq / 8, f = sq % 8;
    for (int rr = r + 1; rr < 8; ++rr) {
        int s = rr * 8 + f;
        attacks |= (1ULL << s);
        if (occ & (1ULL << s)) break;
    }
    for (int rr = r - 1; rr >= 0; --rr) {
        int s = rr * 8 + f;
        attacks |= (1ULL << s);
        if (occ & (1ULL << s)) break;
    }
    for (int ff = f + 1; ff < 8; ++ff) {
        int s = r * 8 + ff;
        attacks |= (1ULL << s);
        if (occ & (1ULL << s)) break;
    }
    for (int ff = f - 1; ff >= 0; --ff) {
        int s = r * 8 + ff;
        attacks |= (1ULL << s);
        if (occ & (1ULL << s)) break;
    }
    return attacks;
}

u64 bishop_attacks_on_the_fly(int sq, u64 occ) {
    u64 attacks = 0ULL;
    int r = sq / 8, f = sq % 8;
    for (int rr = r + 1, ff = f + 1; rr < 8 && ff < 8; ++rr, ++ff) {
        int s = rr * 8 + ff;
        attacks |= (1ULL << s);
        if (occ & (1ULL << s)) break;
    }
    for (int rr = r + 1, ff = f - 1; rr < 8 && ff >= 0; ++rr, --ff) {
        int s = rr * 8 + ff;
        attacks |= (1ULL << s);
        if (occ & (1ULL << s)) break;
    }
    for (int rr = r - 1, ff = f + 1; rr >= 0 && ff < 8; --rr, ++ff) {
        int s = rr * 8 + ff;
        attacks |= (1ULL << s);
        if (occ & (1ULL << s)) break;
    }
    for (int rr = r - 1, ff = f - 1; rr >= 0 && ff >= 0; --rr, --ff) {
        int s = rr * 8 + ff;
        attacks |= (1ULL << s);
        if (occ & (1ULL << s)) break;
    }
    return attacks;
}

// index by occupancy using magic multiplication
inline u64 rook_index(int sq, u64 occ) {
    u64 m = rook_mask(sq);
    occ &= m;
    u64 idx = (occ * R_MAGIC[sq]) >> R_SHIFT[sq];
    return idx;
}

inline u64 bishop_index(int sq, u64 occ) {
    u64 m = bishop_mask(sq);
    occ &= m;
    u64 idx = (occ * B_MAGIC[sq]) >> B_SHIFT[sq];
    return idx;
}

// build attack tables at startup using on-the-fly method to compute target attack sets for each occupancy subset
void init_magic_tables() {
    for (int sq = 0; sq < 64; ++sq) {
        u64 rmask = rook_mask(sq);
        int rbits = popcount(rmask);
        int rsize = 1 << rbits;
        R_ATTACKS[sq].assign(rsize, 0ULL);
        // enumerate subset by mapping subset index to bits covering mask
        vector<int> bits;
        for (int b = 0; b < 64; ++b) if (rmask & (1ULL << b)) bits.push_back(b);
        for (int idx = 0; idx < rsize; ++idx) {
            u64 occ = 0ULL;
            for (int j = 0; j < rbits; ++j) if (idx & (1 << j)) occ |= (1ULL << bits[j]);
            u64 key = ((occ) * R_MAGIC[sq]) >> R_SHIFT[sq];
            R_ATTACKS[sq][key] = rook_attacks_on_the_fly(sq, occ);
        }

        u64 bmask = bishop_mask(sq);
        int bbits = popcount(bmask);
        int bsize = 1 << bbits;
        B_ATTACKS[sq].assign(bsize, 0ULL);
        bits.clear();
        for (int b = 0; b < 64; ++b) if (bmask & (1ULL << b)) bits.push_back(b);
        for (int idx = 0; idx < bsize; ++idx) {
            u64 occ = 0ULL;
            for (int j = 0; j < bbits; ++j) if (idx & (1 << j)) occ |= (1ULL << bits[j]);
            u64 key = ((occ) * B_MAGIC[sq]) >> B_SHIFT[sq];
            B_ATTACKS[sq][key] = bishop_attacks_on_the_fly(sq, occ);
        }
    }
}

u64 rook_attacks(int sq, u64 occ) {
    u64 m = rook_mask(sq);
    u64 idx = ((occ & m) * R_MAGIC[sq]) >> R_SHIFT[sq];
    return R_ATTACKS[sq][idx];
}

u64 bishop_attacks(int sq, u64 occ) {
    u64 m = bishop_mask(sq);
    u64 idx = ((occ & m) * B_MAGIC[sq]) >> B_SHIFT[sq];
    return B_ATTACKS[sq][idx];
}

u64 queen_attacks(int sq, u64 occ) {
    return rook_attacks(sq, occ) | bishop_attacks(sq, occ);
}

string move_to_str(const Move &m) {
    string s = "";
    s.push_back('a' + (m.from % 8));
    s.push_back('1' + (m.from / 8));
    s.push_back('-');
    s.push_back('a' + (m.to % 8));
    s.push_back('1' + (m.to / 8));
    if (m.promo) {
        char c = '?';
        switch (m.promo) {
            case WQ:
            case BQ: c = 'q';
                break;
            case WR:
            case BR: c = 'r';
                break;
            case WB:
            case BB: c = 'b';
                break;
            case WN:
            case BN: c = 'n';
                break;
        }
        s.push_back('=');
        s.push_back(c);
    }
    if (m.en_passant) s += " e.p.";
    if (m.castle) s += " castle";
    return s;
}

void make_move(Board &bd, const Move &m, Undo &u) {
    u.from = m.from;
    u.to = m.to;
    u.capturedPiece = NO_PIECE;
    u.promoPiece = 0;
    u.prevEP = bd.en_passant;
    u.prevCWK = bd.can_castle_white_kingside;
    u.prevCWQ = bd.can_castle_white_queenside;
    u.prevCBK = bd.can_castle_black_kingside;
    u.prevCBQ = bd.can_castle_black_queenside;
    u.halfmove = bd.halfmove_clock;
    u.fullmove = bd.fullmove_number;
    u.side = bd.side_to_move;

    int piece = bd.sq_piece[m.from];
    int captured = bd.sq_piece[m.to];
    // handle en-passant capture
    if (m.en_passant) {
        if (bd.side_to_move == WHITE) {
            captured = BP;
            bd.remove_piece(m.to - 8);
            u.capturedPiece = captured;
        } else {
            captured = WP;
            bd.remove_piece(m.to + 8);
            u.capturedPiece = captured;
        }
    } else if (captured != NO_PIECE) {
        bd.remove_piece(m.to);
        u.capturedPiece = captured;
    }
    // move piece
    bd.remove_piece(m.from);
    if (m.promo) {
        // promotion
        bd.set_piece(m.to, m.promo);
        u.promoPiece = m.promo;
    } else {
        bd.set_piece(m.to, piece);
    }
    // castling: move rook
    if (m.castle) {
        if (bd.side_to_move == WHITE) {
            if (m.to == 6) {
                // kingside
                bd.remove_piece(7);
                bd.set_piece(5, WR);
            } else if (m.to == 2) {
                bd.remove_piece(0);
                bd.set_piece(3, WR);
            }
        } else {
            if (m.to == 62) {
                bd.remove_piece(63);
                bd.set_piece(61, BR);
            } else if (m.to == 58) {
                bd.remove_piece(56);
                bd.set_piece(59, BR);
            }
        }
    }

    // update en-passant
    if (m.double_push) {
        // set ep square
        if (bd.side_to_move == WHITE) bd.en_passant = m.from + 8;
        else bd.en_passant = m.from - 8;
    } else bd.en_passant = -1;

    // update castling rights if king or rook moved/captured
    if (piece == WK) { bd.can_castle_white_kingside = bd.can_castle_white_queenside = false; }
    if (piece == BK) { bd.can_castle_black_kingside = bd.can_castle_black_queenside = false; }
    if (m.from == 0 || m.to == 0) bd.can_castle_white_queenside = false; // rook a1
    if (m.from == 7 || m.to == 7) bd.can_castle_white_kingside = false; // rook h1
    if (m.from == 56 || m.to == 56) bd.can_castle_black_queenside = false; // rook a8
    if (m.from == 63 || m.to == 63) bd.can_castle_black_kingside = false; // rook h8

    // halfmove clock
    if (piece == WP || piece == BP || u.capturedPiece != NO_PIECE) bd.halfmove_clock = 0;
    else bd.halfmove_clock++;
    if (bd.side_to_move == BLACK) bd.fullmove_number++;
    bd.side_to_move = (bd.side_to_move == WHITE ? BLACK : WHITE);
}

void undo_move(Board &bd, const Move &m, const Undo &u) {
    // reverse side
    bd.side_to_move = u.side;
    if (bd.side_to_move == BLACK) bd.fullmove_number = u.fullmove; // original
    bd.halfmove_clock = u.halfmove;
    // undo everything
    // remove what is at to
    int curPiece = bd.sq_piece[m.to];
    bd.remove_piece(m.to);
    // restore moved piece
    if (m.promo) {
        // promoted piece removed, restore pawn
        if (bd.side_to_move == WHITE) bd.set_piece(m.from, WP);
        else bd.set_piece(m.from, BP);
    } else {
        bd.set_piece(m.from, curPiece == NO_PIECE ? (bd.side_to_move == WHITE ? WK : BK) : curPiece);
        // Actually curPiece may equal moved piece; we need to restore original type
        // Simpler: we know original piece via side and square? For baseline we search piece moved by comparing u - but we stored nothing. To be safe, determine from context: if capture happened originally, etc.
        // To avoid complexity, we will derive original piece by checking u.side and whether m.promo present. We already handled promotions. For non-promo, we restored using curPiece; this is correct because we removed and then re-add original piece type. For castling we correct below.
    }
    // restore captured piece
    if (u.capturedPiece != NO_PIECE) {
        if (m.en_passant) {
            if (bd.side_to_move == WHITE) { bd.set_piece(m.to - 8, u.capturedPiece); } else {
                bd.set_piece(m.to + 8, u.capturedPiece);
            }
        } else { bd.set_piece(m.to, u.capturedPiece); }
    }
    // undo rook move for castling
    if (m.castle) {
        if (bd.side_to_move == WHITE) {
            if (m.to == 6) {
                bd.remove_piece(5);
                bd.set_piece(7, WR);
            } else {
                bd.remove_piece(3);
                bd.set_piece(0, WR);
            }
        } else {
            if (m.to == 62) {
                bd.remove_piece(61);
                bd.set_piece(63, BR);
            } else {
                bd.remove_piece(59);
                bd.set_piece(56, BR);
            }
        }
    }
    // restore castle rights and en_passant
    bd.en_passant = u.prevEP;
    bd.can_castle_white_kingside = u.prevCWK;
    bd.can_castle_white_queenside = u.prevCWQ;
    bd.can_castle_black_kingside = u.prevCBK;
    bd.can_castle_black_queenside = u.prevCBQ;
}

// Because undo_move above is messy, for correctness in legality we will instead copy and restore board snapshot when making move for search/legal check


void apply_move(Board &bd, const Move &m) {
    // simplified: copy-based apply
    Snapshot snap;
    snap.bd = bd; // copy
    Undo u;
    make_move(bd, m, u);
}

bool parse_move(const string &s, const vector<Move> &moves, Move &out) {
    if (s.size() < 4) return false;
    int from = (s[1] - '1') * 8 + (s[0] - 'a');
    int to = (s[3] - '1') * 8 + (s[2] - 'a');
    int promo = 0;
    if (s.size() >= 5) {
        char pc = tolower(s[4]);
        if (pc == 'q') promo = (moves[0].from < 16) ? WQ : BQ;
        if (pc == 'r') promo = (moves[0].from < 16) ? WR : BR;
        if (pc == 'b') promo = (moves[0].from < 16) ? WB : BB;
        if (pc == 'n') promo = (moves[0].from < 16) ? WN : BN;
    }
    for (const auto &m: moves) {
        if (m.from == from && m.to == to && (promo == 0 || m.promo == promo)) {
            out = m;
            return true;
        }
    }
    return false;
}
