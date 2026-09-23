//
// UCI protocol front end.
//

#ifndef CHESS_UCI_H
#define CHESS_UCI_H

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "OpeningDB.h"
#include "search.h"

class Engine {
public:
    Engine();

    // Reads UCI commands from stdin until "quit"
    void loop();
    // Handles one command line; returns false on "quit"
    bool command(const std::string &line);

    // Fixed-depth search over a set of positions; prints total nodes and nps (for regression checks)
    void bench(int depth);

    void set_position(const std::string &fen, const std::vector<std::string> &moves);
    // Book / tablebase / search; blocking. Used by the interactive mode.
    Move best_move(const SearchLimits &limits, bool print_info);

    // Plain search without book / tablebases, no UCI output (used by datagen)
    SearchResult think(const SearchLimits &limits) { return search.think(state, limits, false); }
    void new_game() { search.clear(); }

    Position &position() { return *pos; }
    const GameState &game() const { return state; }
    void play(Move m);

private:
    void go(std::istringstream &is);
    void setoption(std::istringstream &is);
    void uci_position(std::istringstream &is);
    bool book_or_tablebase_move(Move &m);

    Search search;
    OpeningDB book;
    std::unique_ptr<Position> pos;
    GameState state;
    int fullmove = 1;

    bool own_book = false;
    std::string book_file = "data/my_openings_l.csv";
    std::string syzygy_path = "<empty>";
};

uint64_t perft(Position &p, int depth);

#endif //CHESS_UCI_H
