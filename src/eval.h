//
// Created by fabian on 9/19/25.
//

#ifndef CHESS_EVAL_H
#define CHESS_EVAL_H

#pragma once

#include "../lib/surge/src/position.h"
#include "../lib/surge/src/types.h"

int piece_value(int piece);
int evaluate(const Position &p);

#endif //CHESS_EVAL_H