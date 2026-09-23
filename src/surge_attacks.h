//
// surge declares get_rook_attacks / get_bishop_attacks as constexpr in tables.h but only defines
// them in tables.cpp. Constexpr functions are implicitly inline, so every translation unit that uses
// attacks<>() (including surge's own move generator) needs the definition. GCC happens to link without
// it, clang does not. The definitions below are token-identical to tables.cpp (ODR-safe) and also let
// the compiler inline the magic lookups in the hot paths.
//
// Include this (via board.h) instead of the surge headers directly.
//

#ifndef CHESS_SURGE_ATTACKS_H
#define CHESS_SURGE_ATTACKS_H

#pragma once

#include "../lib/surge/src/tables.h"

//Returns the attacks bitboard for a rook at a given square, using the magic lookup table
constexpr Bitboard get_rook_attacks(Square square, Bitboard occ) {
	return ROOK_ATTACKS[square][((occ & ROOK_ATTACK_MASKS[square]) * ROOK_MAGICS[square])
		>> ROOK_ATTACK_SHIFTS[square]];
}

//Returns the attacks bitboard for a bishop at a given square, using the magic lookup table
constexpr Bitboard get_bishop_attacks(Square square, Bitboard occ) {
	return BISHOP_ATTACKS[square][((occ & BISHOP_ATTACK_MASKS[square]) * BISHOP_MAGICS[square])
		>> BISHOP_ATTACK_SHIFTS[square]];
}

#endif //CHESS_SURGE_ATTACKS_H
