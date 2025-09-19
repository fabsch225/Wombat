//
// Created by fabian on 9/19/25.
//

#ifndef CHESS_MOVEGEN_H
#define CHESS_MOVEGEN_H

#pragma once

#include <vector>
#include "board.h"
#include "move.h"
#include "constants.h"

extern u64 KNIGHT_ATTACKS[64];
extern u64 KING_ATTACKS[64];

void init_kminor();
bool is_square_attacked(const Board &bd, int sq, Color byColor);
void generate_moves(const Board &bd, std::vector<Move> &out);

bool is_legal_move(const Board &bd, const Move &m);
vector<Move> legal_moves(const Board &bd);

#endif //CHESS_MOVEGEN_H