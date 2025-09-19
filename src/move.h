//
// Created by fabian on 9/19/25.
//

#ifndef CHESS_MOVE_H
#define CHESS_MOVE_H

#pragma once

#include <vector>
#include <string>
#include "board.h"
#include "constants.h"
#include "transposition.h"

using namespace std;

extern const u64 R_MAGIC[64];
extern const u64 B_MAGIC[64];
extern const int R_SHIFT[64];
extern const int B_SHIFT[64];

constexpr int DIR_N = 8;
constexpr int DIR_S = -8;
constexpr int DIR_E = 1;
constexpr int DIR_W = -1;
constexpr int DIR_NE = 9;
constexpr int DIR_NW = 7;
constexpr int DIR_SE = -7;
constexpr int DIR_SW = -9;

extern vector<u64> R_ATTACKS[64];
extern vector<u64> B_ATTACKS[64];

bool on_board(int sq);

u64 rook_mask(int sq);
u64 bishop_mask(int sq);

u64 sliding_attacks_ray(int sq, u64 occ, int dir);

u64 rook_attacks_on_the_fly(int sq, u64 occ);
u64 bishop_attacks_on_the_fly(int sq, u64 occ);

u64 rook_index(int sq, u64 occ);
u64 bishop_index(int sq, u64 occ);

void init_magic_tables();

u64 rook_attacks(int sq, u64 occ);
u64 bishop_attacks(int sq, u64 occ);
u64 queen_attacks(int sq, u64 occ);

struct Move {
    int from, to;
    int promo;
    bool capture;
    bool double_push;
    bool en_passant;
    bool castle;

    Move() : from(0), to(0), promo(0), capture(false), double_push(false), en_passant(false), castle(false) {
    }

    Move(int f, int t) : from(f), to(t), promo(0), capture(false), double_push(false), en_passant(false),
                         castle(false) {
    }
};

string move_to_str(const Move &m);

// make/unmake moves with state saving for legality tests
struct Undo {
    int from, to;
    int capturedPiece;
    int promoPiece;
    int prevEP;
    bool prevCWK, prevCWQ, prevCBK, prevCBQ;
    int halfmove;
    int fullmove;
    Color side;
};

void make_move(Board &bd, const Move &m, Undo &u);
void undo_move(Board &bd, const Move &m, const Undo &u);
void apply_move(Board &bd, const Move &m);

bool parse_move(const string &s, const vector<Move> &moves, Move &out);


#endif //CHESS_MOVE_H