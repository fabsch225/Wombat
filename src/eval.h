//
// Created by fabian on 9/19/25.
//

#ifndef CHESS_EVAL_H
#define CHESS_EVAL_H

#pragma once

#include "surge_attacks.h"
#include "../lib/surge/src/position.h"

// Midgame piece values, used for move ordering, SEE and pruning margins
constexpr int PIECE_VALUE[6] = {100, 320, 330, 500, 950, 0};

// Static evaluation in centipawns from the side to move's point of view.
int evaluate(const Position &p);

#endif //CHESS_EVAL_H
