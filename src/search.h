//
// Created by fabian on 9/19/25.
//

#ifndef CHESS_SEARCH_H
#define CHESS_SEARCH_H

#pragma once

#include "board.h"
#include "move.h"
#include "eval.h"
#include "movegen.h"
#include "openingdb.h"

extern OpeningDB opening_db;

int quiescence(Board &bd, int alpha, int beta);
int alphabeta(Board &bd, int depth, int alpha, int beta);
Move find_best_move(Board &bd, int depth);

#endif //CHESS_SEARCH_H