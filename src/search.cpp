//
// Created by fabian on 9/19/25.
//

#include "search.h"
#include <algorithm>
#include <vector>
#include <random>
#include <ranges>

#include "EndgameDB.h"
#include "OpeningDB.h"
#include "TranspositionTable.h"

using namespace std;

extern OpeningDB opening_db;
TranspositionTable TT;
EndgameDB endgame_db;

template<Color Us>
int quiescence(Position &p, int alpha, int beta) {
    if (p.in_check<Us>()) {
        MoveList<Us> moves(p);
        if (moves.size() == 0) return -99999; // checkmate

        for (auto &m : moves) {
            Position copy = p;
            copy.play<Us>(m);
            int score = -quiescence<~Us>(copy, -beta, -alpha);
            if (score >= beta) return beta;
            if (score > alpha) alpha = score;
        }
        return alpha;
    }

    int stand = evaluate<Us>(p);
    if (stand >= beta) return beta;
    if (alpha < stand) alpha = stand;
    MoveList<Us> moves(p);
    // only consider captures and promotions
    vector<Move> essential_moves;

    for (auto &m: moves) {
        MoveFlags f = m.flags();
        bool isPromotion = (f >= MoveFlags::PR_KNIGHT && f <= MoveFlags::PR_QUEEN) ||
                           (f >= MoveFlags::PC_KNIGHT && f <= MoveFlags::PC_QUEEN);
        if (m.is_capture() || isPromotion) essential_moves.push_back(m);
    }

    // sort captures by simple MVV-LVA heuristic (here by captured piece value)
    sort(essential_moves.begin(), essential_moves.end(), [&](const Move &a, const Move &b) {
        int va = a.is_capture() ? piece_value(p.at(a.to())) : 0;
        int vb = b.is_capture() ? piece_value(p.at(b.to())) : 0;
        return va > vb;
    });
    for (auto &m: essential_moves) {
        Position copy = p;
        copy.play<Us>(m);
        int score = -quiescence<~Us>(copy, -beta, -alpha);
        if (score >= beta) return beta;
        if (score > alpha) alpha = score;
    }
    return alpha;
}

// Alpha-beta search
template<Color Us>
int alphabeta(Position &p, int depth, int alpha, int beta) {

    // TT Lookup
    uint64_t key = p.get_hash();
    TTEntry* entry = TT.probe(key);
    if (entry && entry->depth >= depth) {
        switch (entry->type) {
            case NodeType::EXACT: return entry->score;
            case NodeType::LOWER: if (entry->score > alpha) alpha = entry->score; break;
            case NodeType::UPPER: if (entry->score < beta)  beta  = entry->score; break;
        }
        if (alpha >= beta) return entry->score;
    }

    if (depth == 0) return quiescence<Us>(p, alpha, beta);
    MoveList<Us> moves(p);
    if (moves.size() == 0) {
        // checkmate or stalemate
        // if king is attacked -> checkmate
        if (p.in_check<Us>()) return -99999 + (10 - depth);
        return 0; // stalemate
    }
    // move ordering: simple: captures first
    vector<Move> moveVec(moves.begin(), moves.end());
    sort(moveVec.begin(), moveVec.end(), [&](const Move &a, const Move &b) {
        int va = a.is_capture() ? piece_value(p.at(a.to())) : 0;
        int vb = b.is_capture() ? piece_value(p.at(b.to())) : 0;
        return va > vb;
    });
    int bestScore = -1000000;
    Move bestMove;

    for (auto &m : moves) {
        Position copy = p;
        copy.play<Us>(m);
        int score = -alphabeta<~Us>(copy, depth - 1, -beta, -alpha);
        if (score > bestScore) {
            bestScore = score;
            bestMove = m;
        }
        if (bestScore > alpha) alpha = bestScore;
        if (alpha >= beta) break;
    }
    NodeType type;
    if (bestScore <= alpha) type = NodeType::UPPER;       // fail-low
    else if (bestScore >= beta) type = NodeType::LOWER;   // fail-high
    else type = NodeType::EXACT;                          // exact score

    TT.store(key, depth, bestScore, type, bestMove);

    return bestScore;
}

using Score = int64_t;
static constexpr Score INF = 1000000000; // large bound but << INT64_MAX

template<Color Us>
Move find_best_move(Position &p, int maxDepth) {
    // Opening book query
    std::string book_uci;
    MoveList<Us> rootMoves(p);
    if (maxDepth >= 3 && opening_db.get_random_move(p, book_uci)) {
        Move book_move;
        if (parse_move(p, book_uci, book_move)) {
            return book_move;
        }
    }

    // Endgame tablebase probe
    int dtz;
    Move result;
    if (endgame_db.probe_next_move(p, result, dtz)) {
        cout << "Tablebase move found with DTZ=" << dtz << ": " << result << "\n";
        return result;
    }

    Move bestMove;
    Score prevScore = 0;
    bool haveScore = false;

    // Iterative deepening loop
    for (int depth = 1; depth <= maxDepth; ++depth) {
        Score window = 50; // aspiration window in centipawns
        Score alpha = haveScore ? prevScore - window : -INF;
        Score beta  = haveScore ? prevScore + window :  INF;

        while (true) {
            Score low = alpha;
            Score high = beta;

            Score currentBestScore = -INF;
            Move currentBestMove;

            // Get move list fresh each iteration
            MoveList<Us> moves(p);
            std::vector<Move> moveVec(moves.begin(), moves.end());

            // Put previous best first for better move ordering
            if (haveScore) {
                auto it = std::find(moveVec.begin(), moveVec.end(), bestMove);
                if (it != moveVec.end()) std::iter_swap(moveVec.begin(), it);
            }

            // Root search loop
            for (auto &m : moveVec) {
                Position copy = p;
                copy.play<Us>(m);

                Score score = -alphabeta<~Us>(copy, depth - 1, -beta, -alpha);

                if (score > currentBestScore) {
                    currentBestScore = score;
                    currentBestMove = m;
                }
                if (score > alpha) alpha = score;
                if (alpha >= beta) break; // cutoff
            }

            // Aspiration window checks
            if (currentBestScore <= low) {
                // fail low → widen down
                if (window >= INF / 2) {
                    alpha = -INF; beta = INF;
                } else {
                    window = std::min(window * 2, INF / 2);
                    alpha = prevScore - window;
                    beta  = prevScore + window;
                }
                continue; // retry
            } else if (currentBestScore >= high) {
                // fail high → widen up
                if (window >= INF / 2) {
                    alpha = -INF; beta = INF;
                } else {
                    window = std::min(window * 2, INF / 2);
                    alpha = prevScore - window;
                    beta  = prevScore + window;
                }
                continue; // retry
            } else {
                // success
                prevScore = currentBestScore;
                bestMove = currentBestMove;
                haveScore = true;
                break; // done with this depth
            }
        }
    }

    return bestMove;
}

template int quiescence<WHITE>(Position&, int, int);
template int quiescence<BLACK>(Position&, int, int);
template int alphabeta<WHITE>(Position&, int, int, int);
template int alphabeta<BLACK>(Position&, int, int, int);
template Move find_best_move<WHITE>(Position&, int);
template Move find_best_move<BLACK>(Position&, int);