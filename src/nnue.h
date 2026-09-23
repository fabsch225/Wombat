//
// NNUE evaluation: a (768 -> 256)x2 -> 1 network with SCReLU activation.
//
// Inputs are 768 piece-square features (own/enemy x 6 piece types x 64 squares) seen from each
// side's perspective. The hidden layer ("accumulator") is updated incrementally as moves are
// made, so a full evaluation only costs the small output layer.
//

#ifndef CHESS_NNUE_H
#define CHESS_NNUE_H

#pragma once

#include <cstdint>
#include <string>

#include "surge_attacks.h"
#include "../lib/surge/src/position.h"

namespace nnue {

constexpr int INPUTS = 768;
constexpr int HIDDEN = 256;
constexpr int QA = 255;    // quantisation of the feature transformer
constexpr int QB = 64;     // quantisation of the output layer
constexpr int SCALE = 400; // network output (in win-probability logits) -> centipawns

struct Network {
    alignas(64) int16_t feature_weights[INPUTS * HIDDEN];
    alignas(64) int16_t feature_bias[HIDDEN];
    alignas(64) int16_t output_weights[2 * HIDDEN]; // side to move first, then the other side
    int32_t output_bias;
};

struct Accumulator {
    alignas(64) int16_t values[2][HIDDEN]; // indexed by perspective (WHITE / BLACK)
};

// Loads the network embedded in the binary. Returns false if it is missing or malformed.
bool load_default();
// Loads a network file produced by nnue/train.py
bool load(const std::string &path);
bool loaded();

// Builds the accumulator from scratch
void refresh(Accumulator &acc, const Position &p);

// child = parent updated for move m; `before` is the position before m is played
void update(const Accumulator &parent, Accumulator &child, const Position &before, Move m);

// Evaluation in centipawns from the side to move's point of view
int evaluate(const Accumulator &acc, Color stm);

// Convenience: refresh + evaluate (slow, for the UCI "eval" command and tests)
int evaluate(const Position &p);

} // namespace nnue

#endif //CHESS_NNUE_H
