//
// Created by fabian on 9/19/25.
//

#ifndef CHESS_SEARCH_H
#define CHESS_SEARCH_H

#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "score.h"
#include "timeman.h"
#include "tt.h"
#include "surge_attacks.h"
#include "../lib/surge/src/position.h"

// Everything a search needs to know about the game so far
struct GameState {
    std::string fen;                  // root position
    std::vector<uint64_t> key_history; // keys of positions before the root (for repetitions)
    int halfmove = 0;                 // fifty-move counter at the root
};

struct SearchResult {
    Move best;
    Move ponder;
    int score = 0;
    int depth = 0;
};

class Worker;

class Search {
public:
    Search();
    ~Search();

    void set_hash(size_t mb) { tt.resize(mb); }
    void set_threads(int n);
    void clear(); // ucinewgame

    // Starts searching in the background; bestmove is reported through on_done.
    void start(const GameState &game, const SearchLimits &limits, bool print_info = true);
    void stop() { stop_flag = true; }
    void wait();
    bool searching() const { return running; }

    // Blocking convenience wrapper (used by the interactive CLI and bench)
    SearchResult think(const GameState &game, const SearchLimits &limits, bool print_info);

    uint64_t total_nodes() const;

    TranspositionTable tt;
    std::atomic<bool> stop_flag{false};
    TimeManager time;
    bool print_info = true;
    bool use_nnue = true; // falls back to the hand-crafted eval if no network is loaded

private:
    friend class Worker;
    void run(GameState game, SearchLimits limits);

    std::vector<std::unique_ptr<Worker>> workers;
    std::thread main_thread;
    std::atomic<bool> running{false};
    SearchResult result;
};

#endif //CHESS_SEARCH_H
