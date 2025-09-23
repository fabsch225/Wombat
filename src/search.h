//
// Created by fabian on 9/19/25.
//

#ifndef CHESS_SEARCH_H
#define CHESS_SEARCH_H

#pragma once

#include "eval.h"
#include "../lib/surge/src/position.h"

template<Color Us>
int quiescence(Position &p, int alpha, int beta);

template<Color Us>
int alphabeta_pvs(Position &p, int depth, int alpha, int beta);

template<Color Us>
Move find_best_move(Position &p, int depth, int timeLimitMs = 1000);

#endif //CHESS_SEARCH_H