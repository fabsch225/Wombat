//
// Created by fabian on 9/20/25.
//
// Syzygy tablebase probing through Fathom.
//

#ifndef CHESS_ENDGAMEDB_H
#define CHESS_ENDGAMEDB_H

#pragma once

#include <string>

#include "surge_attacks.h"
#include "../lib/surge/src/position.h"

class EndgameDB {
public:
    // Loads tablebases from a directory (or "<empty>" to disable). Returns false if none were found.
    bool load(const std::string &path);
    bool available() const;
    int max_pieces() const;

    enum WDL { LOSS = -1, DRAW = 0, WIN = 1, FAILED = 2 };

    // Win/draw/loss for the side to move. Only valid when the fifty-move counter is zero.
    WDL probe_wdl(const Position &p) const;

    // Best move at the root according to DTZ tables, respecting the fifty-move counter.
    bool probe_root(Position &p, int halfmove, Move &move, WDL &wdl) const;
};

extern EndgameDB endgame_db;

#endif //CHESS_ENDGAMEDB_H
