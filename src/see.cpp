//
// Static exchange evaluation (swap algorithm, pins are ignored).
//

#include "see.h"

#include "board.h"
#include "eval.h"

static Bitboard attackers_to(const Position &p, Square s, Bitboard occ) {
    Bitboard kings = p.bitboard_of(WHITE_KING) | p.bitboard_of(BLACK_KING);
    return p.attackers_from<WHITE>(s, occ) | p.attackers_from<BLACK>(s, occ) |
           (attacks<KING>(s, occ) & kings);
}

bool see_ge(const Position &p, Move m, int threshold) {
    MoveFlags f = m.flags();
    if (f == OO || f == OOO) return threshold <= 0;

    const Square from = m.from(), to = m.to();
    const PieceType captured = board::captured_type(p, m);
    int swap = (int(captured) >= 0 ? PIECE_VALUE[captured] : 0) - threshold;
    if (swap < 0) return false;

    swap = PIECE_VALUE[type_of(p.at(from))] - swap;
    if (swap <= 0) return true;

    Bitboard occ = (p.all_pieces<WHITE>() | p.all_pieces<BLACK>()) ^ SQUARE_BB[from] ^ SQUARE_BB[to];
    if (f == EN_PASSANT) occ ^= SQUARE_BB[p.turn() == WHITE ? to - 8 : to + 8];

    const Bitboard diag = p.bitboard_of(WHITE_BISHOP) | p.bitboard_of(BLACK_BISHOP) |
                          p.bitboard_of(WHITE_QUEEN) | p.bitboard_of(BLACK_QUEEN);
    const Bitboard orth = p.bitboard_of(WHITE_ROOK) | p.bitboard_of(BLACK_ROOK) |
                          p.bitboard_of(WHITE_QUEEN) | p.bitboard_of(BLACK_QUEEN);

    Bitboard attackers = attackers_to(p, to, occ);
    Color stm = color_of(p.at(from));
    int res = 1;

    while (true) {
        stm = ~stm;
        attackers &= occ;
        Bitboard stm_attackers = attackers & (stm == WHITE ? p.all_pieces<WHITE>() : p.all_pieces<BLACK>());
        if (!stm_attackers) break;
        res ^= 1;

        Bitboard bb;
        if ((bb = stm_attackers & p.bitboard_of(stm, PAWN))) {
            if ((swap = PIECE_VALUE[PAWN] - swap) < res) break;
            occ ^= SQUARE_BB[board::lsb(bb)];
            attackers |= attacks<BISHOP>(to, occ) & diag;
        } else if ((bb = stm_attackers & p.bitboard_of(stm, KNIGHT))) {
            if ((swap = PIECE_VALUE[KNIGHT] - swap) < res) break;
            occ ^= SQUARE_BB[board::lsb(bb)];
        } else if ((bb = stm_attackers & p.bitboard_of(stm, BISHOP))) {
            if ((swap = PIECE_VALUE[BISHOP] - swap) < res) break;
            occ ^= SQUARE_BB[board::lsb(bb)];
            attackers |= attacks<BISHOP>(to, occ) & diag;
        } else if ((bb = stm_attackers & p.bitboard_of(stm, ROOK))) {
            if ((swap = PIECE_VALUE[ROOK] - swap) < res) break;
            occ ^= SQUARE_BB[board::lsb(bb)];
            attackers |= attacks<ROOK>(to, occ) & orth;
        } else if ((bb = stm_attackers & p.bitboard_of(stm, QUEEN))) {
            if ((swap = PIECE_VALUE[QUEEN] - swap) < res) break;
            occ ^= SQUARE_BB[board::lsb(bb)];
            attackers |= (attacks<BISHOP>(to, occ) & diag) | (attacks<ROOK>(to, occ) & orth);
        } else {
            // King: can only capture if the opponent has no attackers left
            Bitboard them = stm == WHITE ? p.all_pieces<BLACK>() : p.all_pieces<WHITE>();
            return (attackers & them) ? res ^ 1 : res;
        }
    }
    return bool(res);
}
