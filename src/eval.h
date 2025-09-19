//
// Created by fabian on 9/19/25.
//

#ifndef CHESS_EVAL_H
#define CHESS_EVAL_H

#pragma once

#include "constants.h"
#include "board.h"

int piece_value(int piece);
int evaluate(const Board &bd);

#endif //CHESS_EVAL_H