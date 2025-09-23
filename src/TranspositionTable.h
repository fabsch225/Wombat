//
// Created by fabian on 9/20/25.
//

#ifndef CHESS_TRANSPOSITIONTABLE_H
#define CHESS_TRANSPOSITIONTABLE_H

#include "../lib/surge/src/position.h"
#include <unordered_map>
#include <cstdint>
#include <shared_mutex>
#include <thread>
#include <atomic>
#include <chrono>
#include <mutex>

enum class NodeType : uint8_t {
    EXACT,     // exact evaluation
    LOWER,     // fail-high
    UPPER      // fail-low
};

struct TTEntry {
    int depth;
    int score;
    NodeType type;
    Move bestMove;
    int generation;   // move generation when stored
};

class TranspositionTable {
public:
    TranspositionTable() {
        running = true;
        cleanupThread = std::thread(&TranspositionTable::cleanupLoop, this);
    }

    ~TranspositionTable() {
        running = false;
        if (cleanupThread.joinable()) {
            cleanupThread.join();
        }
    }

    TTEntry* probe(uint64_t key) {
        std::shared_lock<std::shared_mutex> lock(mtx);
        auto it = table.find(key);
        if (it != table.end()) {
            if (currentGeneration - it->second.generation > maxAge) {
                return nullptr;
            }
            return &it->second;
        }
        return nullptr;
    }

    void store(uint64_t key, int depth, int score, NodeType type, Move bestMove) {
        std::unique_lock<std::shared_mutex> lock(mtx);
        TTEntry entry{depth, score, type, bestMove, currentGeneration};
        auto it = table.find(key);
        if (it == table.end() || depth >= it->second.depth) {
            table[key] = entry;
        }
    }

    void clear() {
        std::unique_lock<std::shared_mutex> lock(mtx);
        table.clear();
    }

    void newMove() {
        currentGeneration++;
    }

private:
    void cleanupLoop() {
        using namespace std::chrono_literals;
        while (running) {
            std::this_thread::sleep_for(100ms); // cleanup every 100ms

            std::unique_lock<std::shared_mutex> lock(mtx);
            for (auto it = table.begin(); it != table.end();) {
                if (currentGeneration - it->second.generation > maxAge) {
                    it = table.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }

    std::unordered_map<uint64_t, TTEntry> table;
    std::shared_mutex mtx;
    int currentGeneration = 0;

    static constexpr int maxAge = 8;

    std::atomic<bool> running;
    std::thread cleanupThread;
};

#endif //CHESS_TRANSPOSITIONTABLE_H
