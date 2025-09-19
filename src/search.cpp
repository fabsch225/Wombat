//
// Created by fabian on 9/19/25.
//

#include "search.h"

// Quiescence: capture-only
int quiescence(Board &bd, int alpha, int beta) {
    int stand = evaluate(bd);
    if (stand >= beta) return beta;
    if (alpha < stand) alpha = stand;
    auto moves = legal_moves(bd);
    // only consider captures and promotions
    vector<Move> caps;
    for (auto &m: moves) if (m.capture || m.promo) caps.push_back(m);
    // sort captures by simple MVV-LVA heuristic (here by captured piece value)
    sort(caps.begin(), caps.end(), [&](const Move &a, const Move &b) {
        int va = a.capture ? piece_value(bd.sq_piece[a.to]) : 0;
        int vb = b.capture ? piece_value(bd.sq_piece[b.to]) : 0;
        return va > vb;
    });
    for (auto &m: caps) {
        Board copy = bd;
        Undo u;
        make_move(copy, m, u);
        int score = -quiescence(copy, -beta, -alpha);
        if (score >= beta) return beta;
        if (score > alpha) alpha = score;
    }
    return alpha;
}

// Alpha-beta search
int alphabeta(Board &bd, int depth, int alpha, int beta) {
    if (depth == 0) return quiescence(bd, alpha, beta);
    auto moves = legal_moves(bd);
    if (moves.empty()) {
        // checkmate or stalemate
        // if king is attacked -> checkmate
        // find king
        Color us = bd.side_to_move;
        int ksq = -1;
        u64 kings = bd.by_color[us] & (us == WHITE ? bd.pieces[WK] : bd.pieces[BK]);
        if (kings) ksq = lsb_index(kings);
        if (ksq != -1 && is_square_attacked(bd, ksq, (us == WHITE ? BLACK : WHITE))) return -99999 + (10 - depth);
        else return 0; // stalemate
    }
    // move ordering: simple: captures first
    sort(moves.begin(), moves.end(), [&](const Move &a, const Move &b) {
        int va = a.capture ? piece_value(bd.sq_piece[a.to]) : 0;
        int vb = b.capture ? piece_value(bd.sq_piece[b.to]) : 0;
        return va > vb;
    });
    for (auto &m: moves) {
        Board copy = bd;
        Undo u;
        make_move(copy, m, u);
        int score = -alphabeta(copy, depth - 1, -beta, -alpha);
        if (score >= beta) return beta;
        if (score > alpha) alpha = score;
    }
    return alpha;
}

Move find_best_move(Board &bd, int depth) {
    // --- Opening book query ---
    std::string book_uci;
    if (depth >= 3 && opening_db.get_random_move(bd, book_uci)) {
        auto moves = legal_moves(bd);
        Move book_move;
        if (parse_move(book_uci, moves, book_move)) {
            return book_move;
        }
    }

    // --- Normal search if no book move found ---
    auto moves = legal_moves(bd);
    Move best;
    int bestScore = -1000000;
    for (auto &m: moves) {
        Board copy = bd;
        Undo u;
        make_move(copy, m, u);
        int score = -alphabeta(copy, depth - 1, -1000000, 1000000);
        if (score > bestScore) {
            bestScore = score;
            best = m;
        }
    }
    return best;
}
