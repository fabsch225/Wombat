//
// Created by fabian on 9/19/25.
//

#include "OpeningDB.h"

#include <fstream>
#include <sstream>
#include <vector>

using namespace std;

bool OpeningDB::load_from_csv(const string &filename) {
    ifstream file(filename);
    if (!file.is_open()) return false;

    db.clear();
    string line;
    getline(file, line); // skip header

    while (getline(file, line)) {
        stringstream ss(line);
        string token;
        PositionEntry entry;

        if (!getline(ss, token, ',') || token.empty()) continue;
        uint64_t hash = stoull(token);

        if (!getline(ss, token, ',')) continue;
        entry.total_occurrences = stoi(token);

        for (int i = 0; i < 3; ++i) {
            if (getline(ss, token, ',')) entry.moves[i] = token;
            if (getline(ss, token, ',')) entry.counts[i] = token.empty() ? 0 : stoi(token);
        }
        db[hash] = entry;
    }
    return true;
}

// The book stores moves as surge from/to squares (castling is king-takes-rook), so match on those
template<Color Us>
static Move find_move(Position &pos, const string &uci) {
    if (uci.size() < 4) return Move();
    Square from = create_square(File(uci[0] - 'a'), Rank(uci[1] - '1'));
    Square to = create_square(File(uci[2] - 'a'), Rank(uci[3] - '1'));
    MoveList<Us> moves(pos);
    for (Move m: moves) {
        if (m.from() != from || m.to() != to) continue;
        MoveFlags f = m.flags();
        bool promotion = (f >= PR_KNIGHT && f <= PR_QUEEN) || (f >= PC_KNIGHT && f <= PC_QUEEN);
        if (promotion && (f & 3) != 3) continue; // book promotions are always to a queen
        return m;
    }
    return Move();
}

bool OpeningDB::probe(Position &pos, Move &move, int min_occurrences) {
    // The book was generated with surge's piece-placement hash, which ignores the side to move
    auto it = db.find(pos.get_hash());
    if (it == db.end() || it->second.total_occurrences < min_occurrences) return false;

    const PositionEntry &entry = it->second;
    vector<pair<Move, int>> candidates;
    for (int i = 0; i < 3; ++i) {
        if (entry.counts[i] <= 0) continue;
        Move m = pos.turn() == WHITE ? find_move<WHITE>(pos, entry.moves[i]) : find_move<BLACK>(pos, entry.moves[i]);
        if (m.to_from() != 0) candidates.emplace_back(m, entry.counts[i]);
    }
    if (candidates.empty()) return false;

    int total = 0;
    for (auto &c: candidates) total += c.second;
    int r = uniform_int_distribution<int>(1, total)(rng);
    for (auto &c: candidates) {
        r -= c.second;
        if (r <= 0) {
            move = c.first;
            return true;
        }
    }
    move = candidates.back().first;
    return true;
}
