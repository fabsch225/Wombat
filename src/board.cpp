//
// Thin helpers on top of surge's Position.
//

#include "board.h"

#include <sstream>

#include "../lib/surge/src/tables.h"

namespace board {

static uint64_t castle_keys[16];
static uint64_t ep_keys[8];

void init() {
    initialise_all_databases();
    zobrist::initialise_zobrist_keys();

    PRNG rng(0x5DEECE66DULL);
    for (auto &k: castle_keys) k = rng.rand<uint64_t>();
    for (auto &k: ep_keys) k = rng.rand<uint64_t>();
}

static void reset(Position &p) {
    for (auto &bb: p.piece_bb) bb = 0;
    for (auto &pc: p.board) pc = NO_PIECE;
    p.side_to_play = WHITE;
    p.game_ply = 0;
    p.hash = 0;
    p.history[0] = UndoInfo();
    p.checkers = 0;
    p.pinned = 0;
}

int set_fen(Position &p, const std::string &fen) {
    reset(p);
    Position::set(fen, p);

    std::istringstream ss(fen);
    std::string placement, side, castling, ep;
    int halfmove = 0;
    ss >> placement >> side >> castling >> ep >> halfmove;

    if (ep.size() == 2 && ep[0] >= 'a' && ep[0] <= 'h' && ep[1] >= '1' && ep[1] <= '8')
        p.history[0].epsq = create_square(File(ep[0] - 'a'), Rank(ep[1] - '1'));
    return halfmove;
}

std::string fen(const Position &p, int halfmove, int fullmove) {
    std::string s;
    for (int r = 7; r >= 0; --r) {
        int empty = 0;
        for (int f = 0; f < 8; ++f) {
            Piece pc = p.at(Square(r * 8 + f));
            if (pc == NO_PIECE) {
                ++empty;
                continue;
            }
            if (empty) s += char('0' + empty);
            empty = 0;
            s += PIECE_STR[pc];
        }
        if (empty) s += char('0' + empty);
        if (r > 0) s += '/';
    }
    s += p.turn() == WHITE ? " w " : " b ";

    int rights = castling_rights(p);
    if (!rights) s += '-';
    if (rights & 1) s += 'K';
    if (rights & 2) s += 'Q';
    if (rights & 4) s += 'k';
    if (rights & 8) s += 'q';

    Square ep = p.history[p.ply()].epsq;
    s += ' ';
    s += ep == NO_SQUARE ? "-" : SQSTR[ep];
    s += " " + std::to_string(halfmove) + " " + std::to_string(fullmove);
    return s;
}

int castling_rights(const Position &p) {
    const Bitboard entry = p.history[p.ply()].entry;
    int rights = 0;
    if (!(entry & WHITE_OO_MASK)) rights |= 1;
    if (!(entry & WHITE_OOO_MASK)) rights |= 2;
    if (!(entry & BLACK_OO_MASK)) rights |= 4;
    if (!(entry & BLACK_OOO_MASK)) rights |= 8;
    return rights;
}

uint64_t key(const Position &p) {
    uint64_t k = p.get_hash() ^ castle_keys[castling_rights(p)];
    if (p.turn() == BLACK) k ^= zobrist::side_key;

    // Only hash the ep square if a capture is actually possible, so repetitions are detected correctly
    Square ep = p.history[p.ply()].epsq;
    if (ep != NO_SQUARE) {
        Bitboard capturers = p.turn() == WHITE
                                 ? pawn_attacks<BLACK>(ep) & p.bitboard_of(WHITE, PAWN)
                                 : pawn_attacks<WHITE>(ep) & p.bitboard_of(BLACK, PAWN);
        if (capturers) k ^= ep_keys[file_of(ep)];
    }
    return k;
}

std::string move_to_uci(Move m) {
    if (is_null(m)) return "0000";
    Square to = m.to();
    if (m.flags() == OO) to = (m.from() == e1) ? g1 : g8;
    if (m.flags() == OOO) to = (m.from() == e1) ? c1 : c8;

    std::string s = std::string(SQSTR[m.from()]) + SQSTR[to];
    if (is_promotion(m)) s += "nbrq"[m.flags() & 3];
    return s;
}

template<Color Us>
static Move parse_uci_impl(Position &p, const std::string &uci) {
    MoveList<Us> moves(p);
    for (Move m: moves) {
        if (move_to_uci(m) == uci) return m;
    }
    return Move();
}

Move parse_uci(Position &p, const std::string &uci) {
    return p.turn() == WHITE ? parse_uci_impl<WHITE>(p, uci) : parse_uci_impl<BLACK>(p, uci);
}

bool insufficient_material(const Position &p) {
    if (p.bitboard_of(WHITE_PAWN) | p.bitboard_of(BLACK_PAWN) |
        p.bitboard_of(WHITE_ROOK) | p.bitboard_of(BLACK_ROOK) |
        p.bitboard_of(WHITE_QUEEN) | p.bitboard_of(BLACK_QUEEN))
        return false;
    int minors = popcount(p.bitboard_of(WHITE_KNIGHT) | p.bitboard_of(BLACK_KNIGHT) |
                          p.bitboard_of(WHITE_BISHOP) | p.bitboard_of(BLACK_BISHOP));
    return minors <= 1;
}

} // namespace board
