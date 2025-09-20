//
// Created by fabian on 9/20/25.
//

#ifndef CHESS_ENDGAMEDB_H
#define CHESS_ENDGAMEDB_H

#pragma once

#include "../lib/surge/src/position.h"
#include <string>

class EndgameDB {
public:
    EndgameDB();

    void load(const std::string& path);
    bool probe_next_move(const Position &p, Move &move, int &dtz);
    bool probe_dtz(const Position& pos, int& result);
    bool available() const { return initialized; }

private:
    bool initialized;
};

#endif //CHESS_ENDGAMEDB_H