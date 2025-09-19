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
#include <fstream>
#include <sstream>
#include <iostream>

#include "board.h"

class OpeningDB {
public:
    struct Entry {
        std::string eco;
        std::string name;
        std::string pgn;
        std::string uci;
        std::string epd;
    };

    OpeningDB();

    void load_all();
    bool get_random_move(const Board &bd, std::string &out_uci);

private:
    std::vector<Entry> openings;
    std::unordered_map<std::string, std::vector<std::string>> epd_to_moves;
    std::mt19937 rng;

    void load_csv(const std::string &filename);
    void build_index();
};

#endif //CHESS_OPENINGDB_H