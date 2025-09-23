//
// Created by fabian on 9/19/25.
//

#include "search.h"
#include <algorithm>
#include <future>
#include <vector>
#include <random>
#include <ranges>

#include "EndgameDB.h"
#include "OpeningDB.h"
#include "SearchThreadpool.h"
#include "TranspositionTable.h"

using namespace std;

using Score = int64_t;
static constexpr Score INF = 1000000000; // large bound but << INT64_MAX
static constexpr int MATE_SCORE = 10000000;

extern OpeningDB opening_db;
TranspositionTable TT;
EndgameDB endgame_db;

template<Color Us>
int quiescence(Position &p, int alpha, int beta) {
    if (p.in_check<Us>() || p.in_check<~Us>()) {
        MoveList<Us> moves(p);
        if (moves.size() == 0) return -MATE_SCORE; // checkmate

        for (auto &m : moves) {
            p.play<Us>(m);
            int score = -quiescence<~Us>(p, -beta, -alpha);
            p.undo<Us>(m);
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

    // sort
    sort(essential_moves.begin(), essential_moves.end(), [&](const Move &a, const Move &b) {
        int va = a.is_capture() ? piece_value(p.at(a.to())) : 0;
        int vb = b.is_capture() ? piece_value(p.at(b.to())) : 0;
        return va > vb;
    });
    for (auto &m: essential_moves) {
        p.play<Us>(m);
        int score = -quiescence<~Us>(p, -beta, -alpha);
        p.undo<Us>(m);
        if (score >= beta) return beta;
        if (score > alpha) alpha = score;
    }
    return alpha;
}


SearchThreadPool pool(std::thread::hardware_concurrency());

// Alpha-beta search
template<Color Us>
int parralel_alphabeta_pvs(Position &p, int depth, int alpha, int beta, bool shouldParallelize) {
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

    int wdl;
    if (depth > 1 && endgame_db.probe_wdl(p, wdl)) {
        
    }

    if (depth == 0) return quiescence<Us>(p, alpha, beta);
    MoveList<Us> moves(p);
    if (moves.size() == 0) {
        // checkmate or stalemate
        // if king is attacked -> checkmate
        if (p.in_check<Us>()) return -MATE_SCORE + (10 - depth);
        return 0; // stalemate
    }

    // Null move pruning

    if (depth >= 3 && !p.in_check<Us>()) {
        Position copy = p;
        copy.side_to_play = ~copy.side_to_play;
        copy.hash ^= zobrist::side_key;

        int score = -parralel_alphabeta_pvs<~Us>(copy, depth - 3, -beta, -beta + 1, shouldParallelize);
        if (score >= beta) return beta;
    }

    // move ordering: simple: captures first
    vector<Move> moveVec(moves.begin(), moves.end());
    sort(moveVec.begin(), moveVec.end(), [&](const Move &a, const Move &b) {
        int va = a.is_capture() ? piece_value(p.at(a.to())) : 0;
        int vb = b.is_capture() ? piece_value(p.at(b.to())) : 0;
        return va > vb;
    });
    int bestScore = -1000000;
    int score;
    Move bestMove;
    int origAlpha = alpha;
    int moveCount = 0;
    std::vector<future<SearchResult>> futures;
    bool pvDone = false;
    for (auto &m : moveVec) {
        moveCount++;

        // Futility Pruning
        if (depth == 1 && !p.in_check<Us>() && !m.is_capture()) {
            int stand = evaluate<Us>(p);
            if (stand + 800 <= alpha) continue;
        }

        // Late Move Pruning
        if (depth <= 3 && moveCount > 12 && !p.in_check<Us>() && !m.is_capture()) {
            continue;
        }

        if (!pvDone) { // pv
            p.play<Us>(m);
            int bestScore = -parralel_alphabeta_pvs<~Us>(p, depth - 1, -beta, -alpha, false);
            p.undo<Us>(m);
            if( bestScore > alpha ) {
                if( bestScore >= beta )
                    return bestScore;
                alpha = bestScore;
            }
            pvDone = true;
        } else if (shouldParallelize) {
            //cout << "Spawning parallel task for move " << m << " at depth " << depth << "\n";
            std::promise<SearchResult> prom;
            futures.push_back(prom.get_future());

            pool.enqueue(packaged_task<void()>([p, depth, alpha, beta, m, prom = std::move(prom)]() mutable {
                Position child = p;
                child.play<Us>(m);
                int score = -parralel_alphabeta_pvs<~Us>(child, depth - 1, -alpha-1, -alpha, false);
                if( score > alpha && score < beta ) {
                    // research with window [alfa;beta]
                    score = -parralel_alphabeta_pvs<~Us>(child, depth-1, -beta, -alpha, false);
                    if(score > alpha)
                        alpha = score;
                }
                prom.set_value(SearchResult{score, m});
            }));
        } else {
            p.play<Us>(m);
            score = -parralel_alphabeta_pvs<~Us>(p, depth - 1, -alpha-1, -alpha, false);
            if( score > alpha && score < beta ) {
                // research with window [alfa;beta]
                score = -parralel_alphabeta_pvs<~Us>(p, depth-1, -beta, -alpha, false);
                if(score > alpha)
                    alpha = score;
            }
            p.undo<Us>(m);
            if (score > bestScore) {
                bestScore = score;
                bestMove = m;
            }
            if (bestScore > alpha) alpha = bestScore;
            if (alpha >= beta) break;
        }
    }

    if (shouldParallelize) {
        for (auto &fut : futures) {
            auto res = fut.get();
            int score = res.score;
            if (score > bestScore) {
                bestScore = score;
                bestMove = res.move;
            }
            if (score > alpha) alpha = score;
            if (alpha >= beta) break;
        }
    }

    NodeType type;
    if (bestScore <= origAlpha) type = NodeType::UPPER;       // fail-low
    else if (bestScore >= beta) type = NodeType::LOWER;       // fail-high
    else type = NodeType::EXACT;                             // exact score

    TT.store(key, depth, bestScore, type, bestMove);

    return bestScore;
}

template<Color Us>
Move find_best_move(Position &p, int maxDepth, int timeLimitMs) {
    //TT.clear();

    auto start = chrono::steady_clock::now();

    // Opening book query
    Move book_move;
    MoveList<Us> rootMoves(p);
    if (maxDepth >= 3 && opening_db.probe(p, book_move)) {
        for (auto &m : rootMoves) {
            if (m.to() == book_move.to() && m.from() == book_move.from()) {
                cout << "Opening Book move found: " << book_move << "\n";
                return m;
            }
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
        auto now = chrono::steady_clock::now();
        if (chrono::duration_cast<chrono::milliseconds>(now - start).count() >= timeLimitMs) {
            break; // stop deepening
        }

        cout << "Searching at depth " << depth << "..." << endl;

        Score window = 500; // aspiration window
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
                now = chrono::steady_clock::now();
                if (duration_cast<chrono::milliseconds>(now - start).count() >= timeLimitMs) {
                    goto TIMEOUT;
                }

                p.play<Us>(m);
                bool tryParallel = depth > 5;
                Score score = -parralel_alphabeta_pvs<~Us>(p, depth - 1, -beta, -alpha, tryParallel);
                p.undo<Us>(m);
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
                // reset scores before retry
                currentBestScore = -INF;
                currentBestMove = Move();
                continue;
            } else if (currentBestScore >= high) {
                // fail high → widen up
                if (window >= INF / 2) {
                    alpha = -INF; beta = INF;
                } else {
                    window = std::min(window * 2, INF / 2);
                    alpha = prevScore - window;
                    beta  = prevScore + window;
                }
                // reset scores before retry
                currentBestScore = -INF;
                currentBestMove = Move();
                continue;
            } else {
                // success
                prevScore = currentBestScore;
                bestMove = currentBestMove;
                haveScore = true;
                break;
            }
        }
    }
TIMEOUT:
    return bestMove;
}

template int quiescence<WHITE>(Position&, int, int);
template int quiescence<BLACK>(Position&, int, int);
template int parralel_alphabeta_pvs<WHITE>(Position&, int, int, int, bool);
template int parralel_alphabeta_pvs<BLACK>(Position&, int, int, int, bool);
template Move find_best_move<WHITE>(Position&, int, int);
template Move find_best_move<BLACK>(Position&, int, int);