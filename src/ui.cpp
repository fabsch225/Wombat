//
// Created by fabian on 9/19/25.
//

#include "ui.h"

void print_board(const Board &b) {
    std::cout << "\n   +-----------------+\n";
    for (int rank = 7; rank >= 0; --rank) {
        std::cout << " " << (rank + 1) << " | ";
        for (int file = 0; file < 8; ++file) {
            int sq = rank * 8 + file;
            char c = piece_to_char(b.sq_piece[sq]);
            std::cout << c << ' ';
        }
        std::cout << "|\n";
    }
    std::cout << "   +-----------------+\n";
    std::cout << "     a b c d e f g h\n\n";

    std::cout << "Side to move: " << (b.side_to_move == WHITE ? "White" : "Black") << "\n";
    std::cout << "Castling rights: "
            << (b.can_castle_white_kingside ? "K" : "")
            << (b.can_castle_white_queenside ? "Q" : "")
            << (b.can_castle_black_kingside ? "k" : "")
            << (b.can_castle_black_queenside ? "q" : "")
            << ((!b.can_castle_white_kingside && !b.can_castle_white_queenside &&
                 !b.can_castle_black_kingside && !b.can_castle_black_queenside)
                    ? "-"
                    : "")
            << "\n";

    if (b.en_passant != -1) {
        int f = b.en_passant % 8;
        int r = b.en_passant / 8;
        std::cout << "En passant: " << char('a' + f) << char('1' + r) << "\n";
    } else std::cout << "En passant: -\n";

    std::cout << "Halfmove clock: " << b.halfmove_clock << "\n";
    std::cout << "Fullmove number: " << b.fullmove_number << "\n\n";
}