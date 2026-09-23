//
// UCI protocol front end.
//

#include "uci.h"

#include <iostream>
#include <sstream>

#include "board.h"
#include "EndgameDB.h"
#include "eval.h"
#include "nnue.h"

using namespace std;

namespace {

const string START_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

const vector<string> BENCH_FENS = {
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
    "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
    "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
    "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
    "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
    "1rq2rk1/pb1nbp1p/2p1p1p1/3nP3/Np1PQ3/1P1B1NP1/P1R2P1P/2BR2K1 b - - 0 1",
    "r1bqkb1r/pppp1ppp/2n2n2/4p2Q/2B1P3/8/PPPP1PPP/RNB1K1NR w KQkq - 4 4",
    "6k1/5ppp/8/8/8/8/5PPP/3R2K1 w - - 0 1",
    "8/8/4k3/8/2p5/8/B2K4/8 w - - 0 1",
    "2r3k1/p4p2/3Rp2p/1p2P1pK/8/1P4P1/P3Q2P/1q6 b - - 0 1",
    "r1bq1rk1/ppp2ppp/2np1n2/2b1p3/2B1P3/2NP1N2/PPP2PPP/R1BQ1RK1 w - - 0 7",
};

template<Color Us>
uint64_t perft_impl(Position &p, int depth) {
    MoveList<Us> moves(p);
    if (depth == 1) return moves.size();
    uint64_t n = 0;
    for (Move m: moves) {
        p.play<Us>(m);
        n += perft_impl<~Us>(p, depth - 1);
        p.undo<Us>(m);
    }
    return n;
}

} // namespace

uint64_t perft(Position &p, int depth) {
    if (depth <= 0) return 1;
    return p.turn() == WHITE ? perft_impl<WHITE>(p, depth) : perft_impl<BLACK>(p, depth);
}

Engine::Engine() : pos(make_unique<Position>()) {
    set_position(START_FEN, {});
}

void Engine::set_position(const string &fen, const vector<string> &moves) {
    pos = make_unique<Position>();
    state.halfmove = board::set_fen(*pos, fen);
    state.key_history.clear();

    // Full move number from the FEN, if present
    istringstream ss(fen);
    string field;
    fullmove = 1;
    for (int i = 0; i < 6 && ss >> field; ++i)
        if (i == 5) fullmove = max(1, atoi(field.c_str()));

    for (const string &uci: moves) {
        Move m = board::parse_uci(*pos, uci);
        if (board::is_null(m)) {
            cerr << "info string illegal move " << uci << endl;
            break;
        }
        play(m);
    }
    state.fen = board::fen(*pos, state.halfmove, fullmove);
}

void Engine::play(Move m) {
    state.key_history.push_back(board::key(*pos));
    bool resets = board::is_capture(m) || type_of(pos->at(m.from())) == PAWN;
    state.halfmove = resets ? 0 : state.halfmove + 1;
    if (pos->turn() == BLACK) ++fullmove;

    if (pos->turn() == WHITE) pos->play<WHITE>(m);
    else pos->play<BLACK>(m);

    // surge keeps a fixed 256-entry undo history; rebuild the board before it can overflow
    if (pos->ply() > 200) {
        string fen = board::fen(*pos, state.halfmove, fullmove);
        pos = make_unique<Position>();
        board::set_fen(*pos, fen);
    }
    state.fen = board::fen(*pos, state.halfmove, fullmove);
}

bool Engine::book_or_tablebase_move(Move &m) {
    if (own_book) {
        if (!book.loaded()) {
            if (book.load_from_csv(book_file)) cout << "info string loaded " << book.size() << " book positions" << endl;
            else {
                cout << "info string could not open book " << book_file << endl;
                own_book = false;
            }
        }
        if (book.loaded() && book.probe(*pos, m)) return true;
    }
    EndgameDB::WDL wdl;
    return endgame_db.probe_root(*pos, state.halfmove, m, wdl);
}

Move Engine::best_move(const SearchLimits &limits, bool print_info) {
    Move m;
    if (book_or_tablebase_move(m)) return m;
    return search.think(state, limits, print_info).best;
}

void Engine::go(istringstream &is) {
    SearchLimits limits;
    string token;
    bool perft_mode = false;
    int perft_depth = 0;

    while (is >> token) {
        if (token == "wtime") is >> limits.time[WHITE];
        else if (token == "btime") is >> limits.time[BLACK];
        else if (token == "winc") is >> limits.inc[WHITE];
        else if (token == "binc") is >> limits.inc[BLACK];
        else if (token == "movestogo") is >> limits.movestogo;
        else if (token == "movetime") is >> limits.movetime;
        else if (token == "depth") is >> limits.depth;
        else if (token == "nodes") is >> limits.nodes;
        else if (token == "infinite") limits.infinite = true;
        else if (token == "perft") { perft_mode = true; is >> perft_depth; }
    }

    if (perft_mode) {
        auto start = chrono::steady_clock::now();
        uint64_t n = perft(*pos, perft_depth);
        auto ms = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - start).count();
        cout << "nodes " << n << " time " << ms << endl;
        return;
    }

    limits.depth = clamp(limits.depth, 1, MAX_PLY - 1);

    Move m;
    if (!limits.infinite && book_or_tablebase_move(m)) {
        cout << "bestmove " << board::move_to_uci(m) << endl;
        return;
    }
    search.start(state, limits, true);
}

void Engine::setoption(istringstream &is) {
    string token, name, value;
    is >> token; // "name"
    while (is >> token && token != "value") name += (name.empty() ? "" : " ") + token;
    while (is >> token) value += (value.empty() ? "" : " ") + token;

    for (auto &c: name) c = char(tolower(c));

    if (name == "hash") search.set_hash(clamp(stoi(value), 1, 65536));
    else if (name == "threads") search.set_threads(clamp(stoi(value), 1, 256));
    else if (name == "move overhead") search.time.move_overhead = clamp(stoi(value), 0, 5000);
    else if (name == "ownbook") own_book = value == "true";
    else if (name == "usennue") search.use_nnue = value == "true";
    else if (name == "evalfile") {
        if (nnue::load(value)) cout << "info string loaded network " << value << endl;
        else cout << "info string could not load network " << value << endl;
    }
    else if (name == "bookfile") book_file = value;
    else if (name == "syzygypath") {
        syzygy_path = value;
        if (endgame_db.load(value)) cout << "info string found " << endgame_db.max_pieces() << "-piece tablebases" << endl;
    } else cout << "info string unknown option " << name << endl;
}

void Engine::uci_position(istringstream &is) {
    string token, fen;
    is >> token;
    if (token == "startpos") {
        fen = START_FEN;
        is >> token; // "moves"
    } else if (token == "fen") {
        while (is >> token && token != "moves") fen += token + " ";
    } else {
        return;
    }
    vector<string> moves;
    while (is >> token) moves.push_back(token);
    set_position(fen, moves);
}

void Engine::bench(int depth) {
    uint64_t nodes = 0;
    auto start = chrono::steady_clock::now();
    for (const string &fen: BENCH_FENS) {
        set_position(fen, {});
        search.clear();
        SearchLimits limits;
        limits.depth = depth;
        search.think(state, limits, false);
        nodes += search.total_nodes();
    }
    auto ms = max<int64_t>(1, chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - start).count());
    cout << nodes << " nodes " << nodes * 1000 / ms << " nps" << endl;
    set_position(START_FEN, {});
}

bool Engine::command(const string &line) {
    istringstream is(line);
    string token;
    if (!(is >> token)) return true;

    if (token == "uci") {
        cout << "id name Wombat\n"
             << "id author Fabian Schuller\n"
             << "option name Hash type spin default 64 min 1 max 65536\n"
             << "option name Threads type spin default 1 min 1 max 256\n"
             << "option name Move Overhead type spin default 30 min 0 max 5000\n"
             << "option name UseNNUE type check default true\n"
             << "option name EvalFile type string default <embedded>\n"
             << "option name OwnBook type check default false\n"
             << "option name BookFile type string default " << book_file << "\n"
             << "option name SyzygyPath type string default <empty>\n"
             << "uciok" << endl;
    } else if (token == "isready") {
        cout << "readyok" << endl;
    } else if (token == "setoption") {
        search.wait();
        setoption(is);
    } else if (token == "ucinewgame") {
        search.stop();
        search.clear();
    } else if (token == "position") {
        search.wait();
        uci_position(is);
    } else if (token == "go") {
        search.stop();
        search.wait();
        go(is);
    } else if (token == "stop") {
        search.stop();
        search.wait();
    } else if (token == "ponderhit") {
        // pondering is not supported; nothing to do
    } else if (token == "quit") {
        search.stop();
        search.wait();
        return false;
    } else if (token == "d") {
        cout << *pos << "FEN: " << state.fen << "\nKey: " << hex << board::key(*pos) << dec << endl;
    } else if (token == "eval") {
        cout << "classical " << evaluate(*pos) << " (side to move)" << endl;
        if (nnue::loaded()) cout << "nnue " << nnue::evaluate(*pos) << " (side to move)" << endl;
    } else if (token == "bench") {
        int depth = 13;
        is >> depth;
        bench(depth);
    } else {
        cout << "info string unknown command " << token << endl;
    }
    return true;
}

void Engine::loop() {
    string line;
    while (getline(cin, line)) {
        if (!command(line)) break;
    }
    search.stop();
    search.wait();
}
