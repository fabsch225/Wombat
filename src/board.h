//
// Thin helpers on top of surge's Position: full zobrist keys, FEN / UCI conversion,
// null moves and a few bitboard utilities.
//

#ifndef CHESS_BOARD_H
#define CHESS_BOARD_H

#pragma once

#include <cstdint>
#include <string>

#include "surge_attacks.h"
#include "../lib/surge/src/position.h"

namespace board {

// Must be called once at startup (initialises surge tables and our extra zobrist keys).
void init();

inline int popcount(Bitboard b) { return __builtin_popcountll(b); }
inline Square lsb(Bitboard b) { return Square(__builtin_ctzll(b)); }

// Resets p and loads the FEN, including en passant square. Returns the halfmove clock.
int set_fen(Position &p, const std::string &fen);

// Full FEN including en passant square and move counters (surge's Position::fen() is incomplete)
std::string fen(const Position &p, int halfmove = 0, int fullmove = 1);

// Castling rights as a 4-bit mask: 1 = K, 2 = Q, 4 = k, 8 = q
int castling_rights(const Position &p);

// Zobrist key including side to move, castling rights and en passant file.
// surge's own hash only covers piece placement.
uint64_t key(const Position &p);

// Converts to UCI notation (surge encodes castling as king-takes-rook, UCI wants e1g1).
std::string move_to_uci(Move m);

// Finds the legal move matching a UCI string, or returns Move() (null) if none does.
Move parse_uci(Position &p, const std::string &uci);

inline bool is_promotion(Move m) {
    MoveFlags f = m.flags();
    return (f >= PR_KNIGHT && f <= PR_QUEEN) || (f >= PC_KNIGHT && f <= PC_QUEEN);
}

inline PieceType promotion_type(Move m) {
    return PieceType(KNIGHT + (m.flags() & 3));
}

// surge's Move::is_capture() is true for every non-quiet flag (castles, double pushes...),
// so always use this instead.
inline bool is_capture(Move m) { return m.flags() & CAPTURE; }

inline bool is_tactical(Move m) { return is_capture(m) || is_promotion(m); }

inline bool is_null(Move m) { return m.to_from() == 0; }

// Piece that ends up being captured by m (handles en passant)
inline PieceType captured_type(const Position &p, Move m) {
    if (m.flags() == EN_PASSANT) return PAWN;
    Piece pc = p.at(m.to());
    return pc == NO_PIECE ? PieceType(-1) : type_of(pc);
}

// Null move: pass the turn. Surge has no native support, so we fake a ply in its history.
inline void make_null(Position &p) {
    p.side_to_play = ~p.side_to_play;
    ++p.game_ply;
    p.history[p.game_ply] = UndoInfo(p.history[p.game_ply - 1]);
}

inline void unmake_null(Position &p) {
    p.side_to_play = ~p.side_to_play;
    --p.game_ply;
}

inline bool in_check(const Position &p) {
    return p.turn() == WHITE ? p.in_check<WHITE>() : p.in_check<BLACK>();
}

// Non-pawn, non-king material present for color c
inline bool has_non_pawn_material(const Position &p, Color c) {
    return p.bitboard_of(c, KNIGHT) | p.bitboard_of(c, BISHOP) |
           p.bitboard_of(c, ROOK) | p.bitboard_of(c, QUEEN);
}

// True for positions no side can ever win (K vs K, K+minor vs K)
bool insufficient_material(const Position &p);

} // namespace board

#endif //CHESS_BOARD_H
