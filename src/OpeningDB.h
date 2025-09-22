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

#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <string>
#include <vector>
#include <random>

using namespace std;

struct PositionEntry {
    string move1;
    int move1_count = 0;
    string move2;
    int move2_count = 0;
    string move3;
    int move3_count = 0;
    int total_occurrences = 0;
};

class OpeningDB {
private:
    unordered_map<uint64_t, PositionEntry> db;
    mt19937 rng;

public:
    OpeningDB() : rng(random_device{}()) {}

    bool load_from_csv(const string &filename) {
        ifstream file(filename);
        if (!file.is_open()) return false;

        string line;
        getline(file, line); // skip header

        while (getline(file, line)) {
            stringstream ss(line);
            string token;
            PositionEntry entry;

            // Hash
            getline(ss, token, ',');
            uint64_t hash = stoull(token);

            // Total occurrences
            getline(ss, token, ',');
            entry.total_occurrences = stoi(token);

            // Move1
            if (getline(ss, token, ',')) entry.move1 = token;
            if (getline(ss, token, ',')) entry.move1_count = token.empty() ? 0 : stoi(token);

            // Move2
            if (getline(ss, token, ',')) entry.move2 = token;
            if (getline(ss, token, ',')) entry.move2_count = token.empty() ? 0 : stoi(token);

            // Move3
            if (getline(ss, token, ',')) entry.move3 = token;
            if (getline(ss, token, ',')) entry.move3_count = token.empty() ? 0 : stoi(token);

            db[hash] = entry;
        }

        return true;
    }

    // Probe method: returns a random move weighted by count
    bool probe(const Position pos, Move &move) {
        auto pos_hash = pos.get_hash();
        auto it = db.find(pos_hash);
        if (it == db.end()) {
            return false;
        }

        const PositionEntry &entry = it->second;

        vector<pair<string, int>> moves;
        if (entry.move1_count > 0) moves.push_back({entry.move1, entry.move1_count});
        if (entry.move2_count > 0) moves.push_back({entry.move2, entry.move2_count});
        if (entry.move3_count > 0) moves.push_back({entry.move3, entry.move3_count});

        if (moves.empty()) return "";

        // Weighted random selection
        int total = 0;
        for (auto &m : moves) total += m.second;
        uniform_int_distribution<int> dist(1, total);
        int r = dist(rng);

        for (auto &m : moves) {
            r -= m.second;
            if (r <= 0) {
                move = parse_move(m.first);
                return true;
            }
        }

        move = parse_move(moves.back().first);
        return true;
    }
private:
    Move parse_move(string uci) {
        Square from = create_square(File(uci[0] - 'a'), Rank(uci[1] - '1'));
        Square to = create_square(File(uci[2] - 'a'), Rank(uci[3] - '1'));
        return Move(from, to);
    }
};

#endif //CHESS_OPENINGDB_H