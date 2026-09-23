#include <iostream>
#include <string>

#include "src/board.h"
#include "src/datagen.h"
#include "src/nnue.h"
#include "src/uci.h"

using namespace std;

// Human (White) vs engine (Black) in the terminal
static void play_cli(Engine &engine, int64_t movetime_ms) {
    while (true) {
        Position &p = engine.position();
        cout << p << "\n";

        Move moves[218];
        Move *end = p.turn() == WHITE ? p.generate_legals<WHITE>(moves) : p.generate_legals<BLACK>(moves);
        if (end == moves) {
            cout << (board::in_check(p) ? "Checkmate!" : "Stalemate!") << "\n";
            break;
        }

        if (p.turn() == WHITE) {
            cout << "Enter your move (e.g. e2e4, quit to exit): ";
            string input;
            if (!(cin >> input) || input == "quit") break;
            Move m = board::parse_uci(p, input);
            if (board::is_null(m)) {
                cout << "Invalid move, try again.\n";
                continue;
            }
            engine.play(m);
        } else {
            cout << "Wombat is thinking...\n";
            SearchLimits limits;
            limits.movetime = movetime_ms;
            Move m = engine.best_move(limits, false);
            cout << "Wombat plays: " << board::move_to_uci(m) << "\n";
            engine.play(m);
        }
    }
    cout << "Final FEN: " << engine.game().fen << "\n";
}

int main(int argc, char **argv) {
    board::init();
    if (!nnue::load_default()) cerr << "info string no embedded network, using the classical evaluation" << endl;
    Engine engine;

    string mode = argc > 1 ? argv[1] : "";
    if (mode == "bench") {
        engine.bench(argc > 2 ? stoi(argv[2]) : 13);
    } else if (mode == "datagen") {
        // Wombat datagen <out-file> <games> <nodes-per-move> <seed>
        if (argc < 6) {
            cerr << "usage: Wombat datagen <out-file> <games> <nodes> <seed>\n";
            return 1;
        }
        run_datagen(argv[2], stoi(argv[3]), stoi(argv[4]), stoull(argv[5]));
    } else if (mode == "play") {
        play_cli(engine, argc > 2 ? stoll(argv[2]) : 3000);
    } else {
        engine.loop();
    }
    return 0;
}
