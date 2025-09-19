//
// Created by fabian on 9/19/25.
//

#ifndef CHESS_BOARD_H
#define CHESS_BOARD_H

#include "constants.h"
#include <cstring>

using namespace std;

static inline int popcount(u64 x) { return __builtin_popcountll(x); }
static inline int lsb_index(u64 x) { return __builtin_ctzll(x); }
static inline int msb_index(u64 x) { return 63 - __builtin_clzll(x); }

static inline u64 pop_lsb(u64 &b) {
    int i = lsb_index(b);
    u64 bit = 1ULL << i;
    b &= b - 1;
    return bit;
}

static const char *SQUARES =
        "a1b1c1d1e1f1g1h1a2b2c2d2e2f2g2h2a3b3c3d3e3f3g3h3a4b4c4d4e4f4g4h4a5b5c5d5e5f5g5h5a6b6c6d6e6f6g6h6a7b7c7d7e7f7g7h7a8b8c8d8e8f8g8h8";

inline char piece_to_char(int piece) {
    switch (piece) {
        case WP: return 'P';
        case WN: return 'N';
        case WB: return 'B';
        case WR: return 'R';
        case WQ: return 'Q';
        case WK: return 'K';
        case BP: return 'p';
        case BN: return 'n';
        case BB: return 'b';
        case BR: return 'r';
        case BQ: return 'q';
        case BK: return 'k';
        default: return '.';
    }
}

inline int char_to_piece(char c) {
    switch (c) {
        case 'P': return WP;
        case 'N': return WN;
        case 'B': return WB;
        case 'R': return WR;
        case 'Q': return WQ;
        case 'K': return WK;
        case 'p': return BP;
        case 'n': return BN;
        case 'b': return BB;
        case 'r': return BR;
        case 'q': return BQ;
        case 'k': return BK;
        default: return NO_PIECE;
    }
}


struct Board {
    // bitboards per piece type
    u64 pieces[13]; // index by Piece enum (0..12)
    u64 by_color[2];
    u64 occupancy;
    int sq_piece[64]; // piece at square
    Color side_to_move;
    int en_passant; // square index or -1
    bool can_castle_white_kingside, can_castle_white_queenside;
    bool can_castle_black_kingside, can_castle_black_queenside;
    int halfmove_clock;
    int fullmove_number;

    Board() { clear(); }

    void clear() {
        memset(pieces, 0, sizeof(pieces));
        by_color[0] = by_color[1] = 0ULL;
        occupancy = 0ULL;
        for (int i = 0; i < 64; ++i) sq_piece[i] = NO_PIECE;
        side_to_move = WHITE;
        en_passant = -1;
        can_castle_white_kingside = can_castle_white_queenside = true;
        can_castle_black_kingside = can_castle_black_queenside = true;
        halfmove_clock = 0;
        fullmove_number = 1;
    }

    void set_piece(int sq, int piece) {
        pieces[piece] |= (1ULL << sq);
        by_color[piece_color(piece)] |= (1ULL << sq);
        occupancy |= (1ULL << sq);
        sq_piece[sq] = piece;
    }

    void remove_piece(int sq) {
        int p = sq_piece[sq];
        if (p == NO_PIECE) return;
        pieces[p] &= ~(1ULL << sq);
        by_color[piece_color(p)] &= ~(1ULL << sq);
        occupancy &= ~(1ULL << sq);
        sq_piece[sq] = NO_PIECE;
    }

    void move_piece(int from, int to) {
        int p = sq_piece[from];
        if (p == NO_PIECE) return;
        remove_piece(from);
        set_piece(to, p);
    }

    void init_from_fen(const string &fen) {
        clear();
        std::stringstream ss(fen);
        string boardPart;
        ss >> boardPart;
        int sq = 56; // start a8 index? Our mapping is a1=0 etc; we'll parse ranks from 8 to1
        int i = 0;
        for (char c: boardPart) {
            if (c == '/') {
                sq -= 16;
                continue;
            }
            if (isdigit(c)) {
                int n = c - '0';
                sq += n;
                continue;
            }
            int piece = char_to_piece(c);
            set_piece(sq, piece);
            ++sq;
        }
        string stm;
        ss >> stm;
        side_to_move = (stm == "w" ? WHITE : BLACK);
        string cast;
        ss >> cast;
        if (cast == "-") {
            can_castle_white_kingside = can_castle_white_queenside =
                                        can_castle_black_kingside = can_castle_black_queenside = false;
        } else {
            can_castle_white_kingside = (cast.find('K') != string::npos);
            can_castle_white_queenside = (cast.find('Q') != string::npos);
            can_castle_black_kingside = (cast.find('k') != string::npos);
            can_castle_black_queenside = (cast.find('q') != string::npos);
        }
        string ep;
        ss >> ep;
        if (ep == "-") en_passant = -1;
        else {
            // convert algebraic to square
            char f = ep[0], r = ep[1];
            int file = f - 'a';
            int rank = r - '1';
            en_passant = rank * 8 + file;
        }
        ss >> halfmove_clock >> fullmove_number;
    }

    string to_fen() {
        string out;
        for (int rank = 7; rank >= 0; --rank) {
            int empty = 0;
            for (int file = 0; file < 8; ++file) {
                int sq = rank * 8 + file;
                int p = sq_piece[sq];
                if (p == NO_PIECE) { empty++; } else {
                    if (empty) {
                        out += char('0' + empty);
                        empty = 0;
                    }
                    char c = piece_to_char(p);
                    out.push_back(c);
                }
            }
            if (empty) out += char('0' + empty);
            if (rank) out.push_back('/');
        }
        out += ' ';
        out += (side_to_move == WHITE ? 'w' : 'b');
        out += ' ';
        string cast = "";
        if (can_castle_white_kingside) cast.push_back('K');
        if (can_castle_white_queenside) cast.push_back('Q');
        if (can_castle_black_kingside) cast.push_back('k');
        if (can_castle_black_queenside) cast.push_back('q');
        if (cast.empty()) out += "-";
        else out += cast;
        out += ' ';
        if (en_passant == -1) out += '-';
        else {
            char f = 'a' + (en_passant % 8);
            char r = '1' + (en_passant / 8);
            out.push_back(f);
            out.push_back(r);
        }
        out += ' ';
        out += to_string(halfmove_clock);
        out += ' ';
        out += to_string(fullmove_number);
        return out;
    }

    string to_epd(bool normalize_ep = true) const {
        string out;
        for (int rank = 7; rank >= 0; --rank) {
            int empty = 0;
            for (int file = 0; file < 8; ++file) {
                int sq = rank * 8 + file;
                int p = sq_piece[sq];
                if (p == NO_PIECE) {
                    empty++;
                } else {
                    if (empty) {
                        out += char('0' + empty);
                        empty = 0;
                    }
                    char c = piece_to_char(p);
                    out.push_back(c);
                }
            }
            if (empty) out += char('0' + empty);
            if (rank) out.push_back('/');
        }

        // side to move
        out += ' ';
        out += (side_to_move == WHITE ? 'w' : 'b');

        // castling rights
        string cast;
        if (can_castle_white_kingside) cast.push_back('K');
        if (can_castle_white_queenside) cast.push_back('Q');
        if (can_castle_black_kingside) cast.push_back('k');
        if (can_castle_black_queenside) cast.push_back('q');
        if (cast.empty()) out += " -";
        else out += ' ' + cast;

        // en passant
        out += ' ';
        if (normalize_ep || en_passant == -1) {
            out += '-';
        } else {
            char f = 'a' + (en_passant % 8);
            char r = '1' + (en_passant / 8);
            out.push_back(f);
            out.push_back(r);
        }

        return out;
    }
};

#endif //CHESS_BOARD_H
