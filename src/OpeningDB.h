//
// Created by fabian on 9/19/25.
//
// Opening book built by util/create_openings.cpp: for each position hash, the three most played moves.
//

#ifndef CHESS_OPENINGDB_H
#define CHESS_OPENINGDB_H

#pragma once

#include <cstdint>
#include <random>
#include <string>
#include <unordered_map>

#include "surge_attacks.h"
#include "../lib/surge/src/position.h"

struct PositionEntry {
    std::string moves[3];
    int counts[3] = {0, 0, 0};
    int total_occurrences = 0;
};

class OpeningDB {
public:
    OpeningDB() : rng(std::random_device{}()) {}

    bool load_from_csv(const std::string &filename);
    bool loaded() const { return !db.empty(); }
    size_t size() const { return db.size(); }

    // Picks a legal book move weighted by how often it was played. Positions seen fewer than
    // min_occurrences times are ignored, since those moves are not reliable.
    bool probe(Position &pos, Move &move, int min_occurrences = 10);

private:
    std::unordered_map<uint64_t, PositionEntry> db;
    std::mt19937 rng;
};

#endif //CHESS_OPENINGDB_H
