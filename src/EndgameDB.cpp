//
// Created by fabian on 9/20/25.
//
// Syzygy tablebase probing through Fathom.
//

#include "EndgameDB.h"

#include "board.h"
#include "tbprobe.h"

EndgameDB endgame_db;

namespace {

struct TbPosition {
    uint64_t white, black, kings, queens, rooks, bishops, knights, pawns;
    unsigned ep;
    bool turn;
};

TbPosition convert(const Position &p) {
    TbPosition t{};
    t.white = p.all_pieces<WHITE>();
    t.black = p.all_pieces<BLACK>();
    t.kings = p.bitboard_of(WHITE_KING) | p.bitboard_of(BLACK_KING);
    t.queens = p.bitboard_of(WHITE_QUEEN) | p.bitboard_of(BLACK_QUEEN);
    t.rooks = p.bitboard_of(WHITE_ROOK) | p.bitboard_of(BLACK_ROOK);
    t.bishops = p.bitboard_of(WHITE_BISHOP) | p.bitboard_of(BLACK_BISHOP);
    t.knights = p.bitboard_of(WHITE_KNIGHT) | p.bitboard_of(BLACK_KNIGHT);
    t.pawns = p.bitboard_of(WHITE_PAWN) | p.bitboard_of(BLACK_PAWN);
    Square ep = p.history[p.ply()].epsq;
    t.ep = ep == NO_SQUARE ? 0 : unsigned(ep);
    t.turn = p.turn() == WHITE;
    return t;
}

EndgameDB::WDL to_wdl(unsigned wdl) {
    // Cursed wins / blessed losses are draws under the fifty-move rule
    if (wdl == TB_WIN) return EndgameDB::WIN;
    if (wdl == TB_LOSS) return EndgameDB::LOSS;
    return EndgameDB::DRAW;
}

} // namespace

bool EndgameDB::load(const std::string &path) {
    if (path.empty() || path == "<empty>") {
        tb_init("");
        return false;
    }
    return tb_init(path.c_str()) && TB_LARGEST > 0;
}

bool EndgameDB::available() const { return TB_LARGEST > 0; }

int EndgameDB::max_pieces() const { return int(TB_LARGEST); }

EndgameDB::WDL EndgameDB::probe_wdl(const Position &p) const {
    if (!available() || board::castling_rights(p) != 0) return FAILED;
    TbPosition t = convert(p);
    if (unsigned(board::popcount(t.white | t.black)) > TB_LARGEST) return FAILED;

    unsigned res = tb_probe_wdl(t.white, t.black, t.kings, t.queens, t.rooks, t.bishops, t.knights, t.pawns,
                                0, 0, t.ep, t.turn);
    return res == TB_RESULT_FAILED ? FAILED : to_wdl(res);
}

bool EndgameDB::probe_root(Position &p, int halfmove, Move &move, WDL &wdl) const {
    if (!available() || board::castling_rights(p) != 0) return false;
    TbPosition t = convert(p);
    if (unsigned(board::popcount(t.white | t.black)) > TB_LARGEST) return false;

    unsigned res = tb_probe_root(t.white, t.black, t.kings, t.queens, t.rooks, t.bishops, t.knights, t.pawns,
                                 unsigned(halfmove), 0, t.ep, t.turn, nullptr);
    if (res == TB_RESULT_FAILED || res == TB_RESULT_CHECKMATE || res == TB_RESULT_STALEMATE) return false;

    std::string uci = std::string(SQSTR[TB_GET_FROM(res)]) + SQSTR[TB_GET_TO(res)];
    switch (TB_GET_PROMOTES(res)) {
        case TB_PROMOTES_QUEEN: uci += 'q'; break;
        case TB_PROMOTES_ROOK: uci += 'r'; break;
        case TB_PROMOTES_BISHOP: uci += 'b'; break;
        case TB_PROMOTES_KNIGHT: uci += 'n'; break;
        default: break;
    }
    move = board::parse_uci(p, uci);
    wdl = to_wdl(TB_GET_WDL(res));
    return !board::is_null(move);
}
