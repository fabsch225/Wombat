//
// Created by fabian on 9/19/25.
//

#include "openingdb.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

OpeningDB::OpeningDB() : rng(std::random_device{}()) {}

void OpeningDB::load_csv(const std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return;

    std::string line;
    // Skip header
    std::getline(file, line);

    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        Entry e;
        std::getline(ss, e.eco, '\t');
        std::getline(ss, e.name, '\t');
        std::getline(ss, e.pgn, '\t');
        std::getline(ss, e.uci, '\t');
        std::getline(ss, e.epd, '\t');
        openings.push_back(e);
    }
}

void OpeningDB::load_all() {
    load_csv("/home/fabian/misc/Chess/data/openings.txt");
    build_index();
}

void OpeningDB::build_index() {
    // First, map "full UCI prefix" -> entries
    std::unordered_map<std::string, std::vector<Entry*>> prefix_map;

    for (auto &op : openings) {
        std::stringstream ss(op.uci);
        std::vector<std::string> moves;
        std::string m;
        while (ss >> m) moves.push_back(m);

        std::string prefix;
        for (size_t i = 0; i < moves.size(); ++i) {
            if (!prefix.empty()) prefix += " ";
            prefix += moves[i];
        }
        prefix_map[prefix].push_back(&op);
    }

    // Now connect rows: prefix (all but last) -> epd of that prefix row
    for (auto &op : openings) {
        std::stringstream ss(op.uci);
        std::vector<std::string> moves;
        std::string m;
        while (ss >> m) moves.push_back(m);

        if (moves.size() < 2) continue;

        std::string prefix;
        for (size_t i = 0; i < moves.size() - 1; ++i) {
            if (!prefix.empty()) prefix += " ";
            prefix += moves[i];
        }

        auto it = prefix_map.find(prefix);
        if (it != prefix_map.end()) {
            for (Entry *before : it->second) {
                epd_to_moves[before->epd].push_back(moves.back());
            }
        }
    }
}

bool OpeningDB::get_random_move(const Board &bd, std::string &out_uci) {
    std::string current_epd = bd.to_epd(true);
    auto it = epd_to_moves.find(current_epd);
    if (it == epd_to_moves.end()) return false;

    auto &moves = it->second;
    std::uniform_int_distribution<int> dist(0, moves.size() - 1);
    out_uci = moves[dist(rng)];
    return true;
}