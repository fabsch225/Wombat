//
// Created by fabian on 9/19/25.
//

#include "search.h"
#include <algorithm>
#include <vector>
#include <random>
#include <ranges>

#include "openingdb.h"

using namespace std;

extern OpeningDB opening_db;

template<Color Us>
int quiescence(Position &p, int alpha, int beta) {
    int stand = evaluate(p);
    if (stand >= beta) return beta;
    if (alpha < stand) alpha = stand;
    MoveList<Us> moves(p);
    // only consider captures and promotions
    vector<Move> caps;

    for (auto &m: moves) {
        MoveFlags f = m.flags();
        bool isPromotion = (f >= MoveFlags::PR_KNIGHT && f <= MoveFlags::PR_QUEEN) ||
                           (f >= MoveFlags::PC_KNIGHT && f <= MoveFlags::PC_QUEEN);
        if (m.is_capture() || isPromotion) caps.push_back(m);
    }

    // sort captures by simple MVV-LVA heuristic (here by captured piece value)
    sort(caps.begin(), caps.end(), [&](const Move &a, const Move &b) {
        int va = a.is_capture() ? piece_value(p.at(a.to())) : 0;
        int vb = b.is_capture() ? piece_value(p.at(b.to())) : 0;
        return va > vb;
    });
    for (auto &m: caps) {
        Position copy = p;
        copy.play<Us>(m);
        int score = -quiescence<Us>(copy, -beta, -alpha);
        if (score >= beta) return beta;
        if (score > alpha) alpha = score;
    }
    return alpha;
}

// Alpha-beta search
template<Color Us>
int alphabeta(Position &p, int depth, int alpha, int beta) {
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
    for (auto &m: moves) {
        Position copy = p;
        copy.play<Us>(m);
        int score = -alphabeta<Us>(copy, depth - 1, -beta, -alpha);
        if (score >= beta) return beta;
        if (score > alpha) alpha = score;
    }
    return alpha;
}

template<Color Us>
Move find_best_move(Position &p, int depth) {
    // --- Opening book query ---
    std::string book_uci;
    MoveList<Us> moves(p);
    if (depth >= 3 && opening_db.get_random_move(p, book_uci)) {
        Move book_move;
        if (parse_move(p, book_uci, book_move)) {
            return book_move;
        }
    }

    // --- Normal search if no book move found ---
    Move best;
    int bestScore = -1000000;
    for (auto &m: moves) {
        Position copy = p;
        copy.play<Us>(m);
        int score = -alphabeta<Us>(copy, depth - 1, -1000000, 1000000);
        if (score > bestScore) {
            bestScore = score;
            best = m;
        }
    }
    return best;
}

template int quiescence<WHITE>(Position&, int, int);
template int quiescence<BLACK>(Position&, int, int);
template int alphabeta<WHITE>(Position&, int, int, int);
template int alphabeta<BLACK>(Position&, int, int, int);
template Move find_best_move<WHITE>(Position&, int);
template Move find_best_move<BLACK>(Position&, int);