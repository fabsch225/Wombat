//
// Self-play training data generation for the NNUE.
//

#ifndef CHESS_DATAGEN_H
#define CHESS_DATAGEN_H

#pragma once

#include <cstdint>
#include <string>

// Plays `games` self-play games at `nodes` nodes per move and appends quiet positions to `out` as
// "<fen> | <score, white pov, cp> | <result, white pov: 1.0 / 0.5 / 0.0>"
void run_datagen(const std::string &out, int games, int nodes, uint64_t seed);

#endif //CHESS_DATAGEN_H
