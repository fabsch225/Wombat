//
// Created by fabian on 9/20/25.
//

#ifndef CHESS_TRANSPOSITIONTABLE_H
#define CHESS_TRANSPOSITIONTABLE_H

#include "../lib/surge/src/position.h"
#include <unordered_map>
#include <cstdint>
#include <mutex>

enum class NodeType : uint8_t {
    EXACT,     // exact evaluation
    LOWER,     // fail-high
    UPPER      // fail-low
};

struct TTEntry {
    int depth;        // search depth at which this score was stored
    int score;        // evaluation score
    NodeType type;    // bound type
    Move bestMove;    // best move found (optional but helps ordering)
};

class TranspositionTable {
public:
    TTEntry* probe(uint64_t key) {
        auto it = table.find(key);
        if (it != table.end()) return &it->second;
        return nullptr;
    }

    void store(uint64_t key, int depth, int score, NodeType type, Move bestMove) {
        TTEntry entry{depth, score, type, bestMove};
        auto it = table.find(key);
        if (it == table.end() || depth >= it->second.depth) {
            table[key] = entry; // replace if deeper
        }
    }

    void clear() {
        table.clear();
    }

private:
    std::unordered_map<uint64_t, TTEntry> table;
};



#endif //CHESS_TRANSPOSITIONTABLE_H