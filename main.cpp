#include <bits/stdc++.h>
#include <iostream>
#include <iomanip>
#include <string>

#include "tbprobe.h"
#include "lib/surge/src/position.h"

#include "src/eval.h"
#include "src/OpeningDB.h"
#include "src/search.h"

using namespace std;

OpeningDB opening_db;

int main() {
    // Initialze Syzygy
    if (!tb_init("/home/fabian/CLionProjects/Chess/data/syzygy")) {
        cerr << "Failed to initialize Syzygy tablebases.\n";
        return 1;
    }

    // Initialize surge
    initialise_all_databases();
    zobrist::initialise_zobrist_keys();

    opening_db.load_from_csv("/home/fabian/CLionProjects/Chess/data/my_openings_l.csv");

    Position p;
    //Position::set("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -", p);
    Position::set("1rq2rk1/pb1nbp1p/2p1p1p1/3nP3/Np1PQ3/1P1B1NP1/P1R2P1P/2BR2K1 b -  -", p);
    cout << "Starting FEN: " << p.fen() << "\n";

    int max_depth = 12; // AI search depth
    int time_limit_ms = 7000;

    while (true) {
        cout << p << "\n";

        MoveList<WHITE> list(p);
        if (list.size() == 0) {
            cout << "Game over!\n";
            break;
        }

        if (p.turn() == WHITE) {
            // Human move
            Move user_move;
            string input;
            bool valid = false;

            while (!valid) {
                cout << "Enter your move (e.g., e2e4): ";
                cin >> input;
                user_move = Move(input);
                for (const auto &m: list) {
                    cout << m << " ";
                    if (m.to() == user_move.to() && m.from() == user_move.from()) {
                        user_move = m;
                        valid = true;
                        break;
                    }
                }

                cout << "Your move is" << user_move << "\n";
                if (valid) {
                    p.play<WHITE>(user_move);
                } else {
                    cout << "Invalid move, try again.\n";
                }
            }
        } else {
            MoveList<BLACK> ai_list(p);
            if (ai_list.size() == 0) {
                cout << "Game over!\n";
                break;
            }

            // AI move
            cout << "AI thinking...\n";
            Move best = find_best_move<BLACK>(p, max_depth, time_limit_ms);
            cout << "AI plays: " << best << "\n";
            p.play<BLACK>(best);
        }
    }

    cout << "Final FEN: " << p.fen() << "\n";
    return 0;
}
