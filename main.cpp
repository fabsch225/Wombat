// cpp_chess_engine.cpp
// Baseline C++ chess engine with magic bitboards, full rule support
// - En-passant (and en-passant x-ray checks)
// - Promotions
// - Castling (both sides)
// - Legal move generation via make/move & attack tests
// - Magic bitboard sliding attacks (rook & bishop) with precomputed magics
// - Alpha-Beta with quiescence search
// - Basic evaluation (material + simple piece-square tables)
//
// Compile: g++ -O3 -std=c++17 cpp_chess_engine.cpp -o engine
// Run: ./engine
// The program runs a simple search from the starting position and prints the best move found

#include <bits/stdc++.h>
#include <iostream>
#include <iomanip>
#include <string>

#include "src/movegen.h"
#include "src/board.h"
#include "src/move.h"
#include "src/eval.h"
#include "src/openingdb.h"
#include "src/search.h"
#include "src/ui.h"

using namespace std;

// board square indices 0..63

OpeningDB opening_db;

int main() {
    opening_db.load_all();

    init_kminor();
    init_magic_tables();

    Board bd;
    bd.init_from_fen("r1b1kbnr/2p1pppp/8/2p5/4PB2/p1N2N2/PPP2PPP/R2R2K1 w - - 0 12");

    cout << "Starting FEN: " << bd.to_fen() << "\n";

    int depth = 4; // AI search depth

    while (true) {
        print_board(bd);

        // Check game over (simplified)
        vector<Move> moves = legal_moves(bd);
        if (moves.empty()) {
            cout << "Game over!\n";
            break;
        }

        if (bd.side_to_move == WHITE) {
            // Human move
            Move user_move;
            string input;
            bool valid = false;

            while (!valid) {
                cout << "Enter your move (e.g., e2e4): ";
                cin >> input;

                for (const auto &m: moves) {
                    cout << move_to_str(m) << " ";
                }

                if (parse_move(input, moves, user_move) && is_legal_move(bd, user_move)) {
                    apply_move(bd, user_move);
                    valid = true;
                } else {
                    cout << "Invalid move, try again.\n";
                }
            }
        } else {
            // AI move
            cout << "AI thinking...\n";
            Move best = find_best_move(bd, depth);
            cout << "AI plays: " << move_to_str(best) << "\n";
            apply_move(bd, best);
        }
    }

    cout << "Final FEN: " << bd.to_fen() << "\n";
    return 0;
}
