//
// Created by fabian on 9/20/25.
//

#include "EndgameDB.h"

#include "tbprobe.h"

#define BOARD_RANK_1            0x00000000000000FFull
#define BOARD_FILE_A            0x8080808080808080ull
#define square(r, f)            (8 * (r) + (f))
#define rank(s)                 ((s) >> 3)
#define file(s)                 ((s) & 0x07)
#define board(s)                ((uint64_t)1 << (s))

static const char *wdl_to_str[5] =
{
    "0-1",
    "1/2-1/2",
    "1/2-1/2",
    "1/2-1/2",
    "1-0"
};

struct tb_pos
{
    uint64_t white;
    uint64_t black;
    uint64_t kings;
    uint64_t queens;
    uint64_t rooks;
    uint64_t bishops;
    uint64_t knights;
    uint64_t pawns;
    uint8_t castling;
    uint8_t rule50;
    uint8_t ep;
    bool turn;
    uint16_t move;
};

static bool is_en_passant(const struct tb_pos *pos, uint64_t from, uint64_t to)
{
    uint64_t us = (pos->turn? pos->white: pos->black);
    if (pos->ep == 0)
        return false;
    if (to != pos->ep)
        return false;
    if ((board(from) & us & pos->pawns) == 0)
        return false;
    return true;
}

static void move_parts_to_str(const struct tb_pos *pos, int from, int to, int promotes, char *str) {
    unsigned r        = rank(from);
    unsigned f        = file(from);
    uint64_t occ      = pos->black | pos->white;
    uint64_t us       = (pos->turn? pos->white: pos->black);
    bool     capture  = (occ & board(to)) != 0 || is_en_passant(pos,from,to);
    uint64_t b = board(from), att = 0;
    if (b & pos->kings)
        *str++ = 'K';
    else if (b & pos->queens)
    {
        *str++ = 'Q';
        att = tb_queen_attacks(to, occ) & us & pos->queens;
    }
    else if (b & pos->rooks)
    {
        *str++ = 'R';
        att = tb_rook_attacks(to, occ) & us & pos->rooks;
    }
    else if (b & pos->bishops)
    {
        *str++ = 'B';
        att = tb_bishop_attacks(to, occ) & us & pos->bishops;
    }
    else if (b & pos->knights)
    {
        *str++ = 'N';
        att = tb_knight_attacks(to) & us & pos->knights;
    }
    else
        att = tb_pawn_attacks(to, !pos->turn) & us & pos->pawns;
    if ((b & pos->pawns) && capture)
        *str++ = 'a' + f;
    else if (tb_pop_count(att) > 1)
    {
        if (tb_pop_count(att & (BOARD_FILE_A >> f)) == 1)
            *str++ = 'a' + f;
        else if (tb_pop_count(att & (BOARD_RANK_1 << (8*r))) == 1)
            *str++ = '1' + r;
        else
        {
            *str++ = 'a' + f;
            *str++ = '1' + r;
        }
    }
    if (capture)
        *str++ = 'x';
    *str++ = 'a' + file(to);
    *str++ = '1' + rank(to);
    if (promotes != TB_PROMOTES_NONE)
    {
        *str++ = '=';
        switch (promotes)
        {
            case TB_PROMOTES_QUEEN:
                *str++ = 'Q'; break;
            case TB_PROMOTES_ROOK:
                *str++ = 'R'; break;
            case TB_PROMOTES_BISHOP:
                *str++ = 'B'; break;
            case TB_PROMOTES_KNIGHT:
                *str++ = 'N'; break;
        }
    }
    *str++ = '\0';
}

static void move_to_str(const struct tb_pos *pos, unsigned move, char *str)
{
    unsigned from     = TB_GET_FROM(move);
    unsigned to       = TB_GET_TO(move);
    unsigned promotes = TB_GET_PROMOTES(move);
    move_parts_to_str(pos, from, to, promotes, str);
}

static bool print_moves(struct tb_pos *pos, unsigned *results, bool prev,
    unsigned wdl)
{
    for (unsigned i = 0; results[i] != TB_RESULT_FAILED; i++)
    {
        if (TB_GET_WDL(results[i]) != wdl)
            continue;
        if (prev)
            printf(", ");
        prev = true;
        char str[32];
        move_to_str(pos, results[i], str);
        printf("%s", str);
    }
    return prev;
}


void convertPosition(const Position& P, tb_pos& out) {
    // Collect piece bitboards
    uint64_t WK = P.bitboard_of(WHITE_KING);
    uint64_t WQ = P.bitboard_of(WHITE_QUEEN);
    uint64_t WR = P.bitboard_of(WHITE_ROOK);
    uint64_t WB = P.bitboard_of(WHITE_BISHOP);
    uint64_t WN = P.bitboard_of(WHITE_KNIGHT);
    uint64_t WP = P.bitboard_of(WHITE_PAWN);

    uint64_t BK = P.bitboard_of(BLACK_KING);
    uint64_t BQ = P.bitboard_of(BLACK_QUEEN);
    uint64_t BR = P.bitboard_of(BLACK_ROOK);
    uint64_t BB = P.bitboard_of(BLACK_BISHOP);
    uint64_t BN = P.bitboard_of(BLACK_KNIGHT);
    uint64_t BP = P.bitboard_of(BLACK_PAWN);

    out.kings   = WK | BK;
    out.queens  = WQ | BQ;
    out.rooks   = WR | BR;
    out.bishops = WB | BB;
    out.knights = WN | BN;
    out.pawns   = WP | BP;

    out.white = WK | WQ | WR | WB | WN | WP;
    out.black = BK | BQ | BR | BB | BN | BP;

    // Side to move
    out.turn = (P.turn() == WHITE);

    // Move counter (half-moves since start)
    out.move = (P.ply() / 2) + 1;

    // Rule 50 (not tracked explicitly in Position)
    out.rule50 = 0; // Placeholder unless you track it separately

    // En passant
    Square ep = P.history[P.ply()].epsq;
    out.ep = (ep == NO_SQUARE ? 0 : ep);

    // Castling rights — derive from entry bitboard
    out.castling = 0;
    const UndoInfo& hist = P.history[P.ply()];

    // White O-O
    if (!(hist.entry & (SQUARE_BB[e1] | SQUARE_BB[h1])))
        out.castling |= TB_CASTLING_K;
    // White O-O-O
    if (!(hist.entry & (SQUARE_BB[e1] | SQUARE_BB[a1])))
        out.castling |= TB_CASTLING_Q;
    // Black O-O
    if (!(hist.entry & (SQUARE_BB[e8] | SQUARE_BB[h8])))
        out.castling |= TB_CASTLING_k;
    // Black O-O-O
    if (!(hist.entry & (SQUARE_BB[e8] | SQUARE_BB[a8])))
        out.castling |= TB_CASTLING_q;
}

bool EndgameDB::probe_next_move(const Position &p, Move &move_out, int &dtz_out) {
    tb_pos pos;
    convertPosition(p, pos);

    if (tb_pop_count(pos.white | pos.black) > TB_LARGEST)
    {
        return false;
    }
    unsigned results[TB_MAX_MOVES];
    unsigned res = tb_probe_root(pos.white, pos.black, pos.kings,
        pos.queens, pos.rooks, pos.bishops, pos.knights, pos.pawns,
        pos.rule50, pos.castling, pos.ep, pos.turn, results);
    if (res == TB_RESULT_FAILED)
    {
        fprintf(stderr, "error: unable to probe tablebase; position "
            "invalid, illegal or not in tablebase\n");
        return false;
    }
    int wdl = TB_GET_WDL(res);
    if (wdl == TB_LOSS || wdl == TB_BLESSED_LOSS) {
        // No winning move available
        return false;
    }
    if (wdl == TB_DRAW) {
        dtz_out = 0;
    } else {
        dtz_out = TB_GET_DTZ(res);
    }

    int from = TB_GET_FROM(res);
    int to = TB_GET_TO(res);

    move_out = Move(Square(from), Square(to));

    return true;
}

bool EndgameDB::probe_wdl(const Position &pos, int &result) {

}


EndgameDB::EndgameDB() : initialized(false) {}

void EndgameDB::load(const std::string& path) {
    // TODO: implement actual tablebase loading
    // For now consider non-empty path as "loaded"
    initialized = !path.empty();
}

bool EndgameDB::probe_dtz(const Position& /*pos*/, int& /*result*/) {
    // TODO: implement DTZ probe against loaded tablebases
    return false;
}