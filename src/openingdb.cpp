//
// Created by fabian on 9/19/25.
//

#include "openingdb.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

bool parse_move(const Position &pos, const std::string &uci, Move &out_move) {
    if (uci.length() < 4) return false;
    Square from = create_square(File(uci[0] - 'a'), Rank(uci[1] - '1'));
    Square to = create_square(File(uci[2] - 'a'), Rank(uci[3] - '1'));
    MoveFlags flags = QUIET;

    if (uci.length() == 5) {
        char promo = uci[4];
        switch (promo) {
            case 'n': flags = PR_KNIGHT; break;
            case 'b': flags = PR_BISHOP; break;
            case 'r': flags = PR_ROOK; break;
            case 'q': flags = PR_QUEEN; break;
            default: return false;
        }
    }

    out_move = Move(from, to, flags);
    return true;
}

OpeningDB::OpeningDB() : rng(std::random_device{}()) {}

void OpeningDB::load_csv(const std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return;

    std::string line;
    std::getline(file, line); // skip header

    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        Entry e;
        std::getline(ss, e.eco, '\t');
        std::getline(ss, e.name, '\t');
        std::getline(ss, e.pgn, '\t');
        std::getline(ss, e.uci, '\t');
        std::getline(ss, e.fen, '\t');
        openings.push_back(e);
    }
}

void OpeningDB::load_all() {
    load_csv("a.csv");
    load_csv("b.csv");
    load_csv("c.csv");
    load_csv("d.csv");
    load_csv("e.csv");
    build_index();
}

void OpeningDB::play_moves_on_position(Position &pos, const std::string &uci) {
    std::stringstream ss(uci);
    std::string move;
    while (ss >> move) {
        Move m;
        if (parse_move(pos, move, m)) {
            if (pos.turn() == WHITE) pos.play<WHITE>(m);
            else pos.play<BLACK>(m);
        }
    }
}

void OpeningDB::build_index() {
    for (auto &op : openings) {
        Position pos;
        Position::set(op.fen, pos); // initialise board from FEN
        std::stringstream ss(op.uci);
        std::vector<std::string> moves;
        std::string move;
        while (ss >> move) moves.push_back(move);

        // Apply all moves except last to the position
        for (size_t i = 0; i + 1 < moves.size(); ++i) {
            play_moves_on_position(pos, moves[i]);
        }

        if (!moves.empty()) {
            // Last move is candidate
            hash_to_moves[pos.get_hash()].push_back(moves.back());
        }
    }
}

bool OpeningDB::get_random_move(const Position &pos, std::string &out_uci) {
    auto it = hash_to_moves.find(pos.get_hash());
    if (it == hash_to_moves.end()) return false;

    auto &moves = it->second;
    std::uniform_int_distribution<int> dist(0, moves.size() - 1);
    out_uci = moves[dist(rng)];
    return true;
}