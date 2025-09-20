//
// Created by fabian on 9/19/25.
//

#ifndef CHESS_OPENINGDB_H
#define CHESS_OPENINGDB_H

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <random>
#include "../lib/surge/src/position.h"

bool parse_move(const Position &pos, const std::string &uci, Move &out_move);

class OpeningDB {
public:
    struct Entry {
        std::string eco;
        std::string name;
        std::string pgn;
        std::string uci;
        std::string fen; // original FEN for reference
    };

    OpeningDB();

    void load_all();
    bool get_random_move(const Position &pos, std::string &out_uci);

private:
    std::vector<Entry> openings;
    std::unordered_map<uint64_t, std::vector<std::string>> hash_to_moves;
    std::mt19937 rng;

    void load_csv(const std::string &filename);
    void build_index();
    void play_moves_on_position(Position &pos, const std::string &uci);
};

#endif //CHESS_OPENINGDB_H