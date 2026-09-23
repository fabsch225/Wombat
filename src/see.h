//
// Static exchange evaluation.
//

#ifndef CHESS_SEE_H
#define CHESS_SEE_H

#pragma once

#include "surge_attacks.h"
#include "../lib/surge/src/position.h"

// True if the exchange sequence started by m on its target square wins at least `threshold` centipawns.
bool see_ge(const Position &p, Move m, int threshold);

#endif //CHESS_SEE_H
