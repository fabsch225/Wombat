//
// Created by fabian on 9/21/25.
//

// build_opening_db.cpp
// Compile and link with your engine code that defines Position, Move, types.h, tables.h, etc.

#include <bits/stdc++.h>
#include <filesystem>
#include <regex>
#include "../lib/surge/src/position.h"

using namespace std;
namespace fs = std::filesystem;

static inline string square_to_string(Square s) {
    int sq = int(s);
    int file = sq % 8;
    int rank = sq / 8;
    string out;
    out.push_back(char('a' + file));
    out.push_back(char('1' + rank));
    return out;
}

static string move_to_uci(const Move &m) {
    string s = square_to_string(m.from()) + square_to_string(m.to());
    switch (m.flags()) {
        case PR_KNIGHT: s.push_back('n'); break;
        case PR_BISHOP: s.push_back('b'); break;
        case PR_ROOK:   s.push_back('r'); break;
        case PR_QUEEN:  s.push_back('q'); break;
        default: break;
    }
    return s;
}

// Minimal piece-letter mapping used in SAN generation (uppercase for piece types except pawn)
static char piece_letter(Piece pc) {
    if (pc == NO_PIECE) return '?';
    PieceType pt = type_of(pc);
    switch (pt) {
        case KING: return 'K';
        case QUEEN: return 'Q';
        case ROOK: return 'R';
        case BISHOP: return 'B';
        case KNIGHT: return 'N';
        case PAWN: return '\0';
    }
    return '\0';
}

// Trim helper
static inline string trim(const string &s) {
    size_t a = 0;
    while (a < s.size() && isspace((unsigned char)s[a])) ++a;
    size_t b = s.size();
    while (b > a && isspace((unsigned char)s[b-1])) --b;
    return s.substr(a, b-a);
}

template<Color Us>
static bool resolve_san_to_move(Position &pos, const string &san_token, Move &resolved) {
    if (san_token == "1-0" || san_token == "0-1" || san_token == "1/2-1/2" || san_token == "*")
        return false;

    // Strip + and # from SAN
    string token = san_token;
    while (!token.empty() && (token.back() == '+' || token.back() == '#'))
        token.pop_back();

    // Handle castling explicitly
    string lower = token;
    transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    if (lower == "o-o" || lower == "0-0") {
        MoveList<Us> moves(pos);
        for (auto &m : moves) {
            if (m.flags() == OO) { resolved = m; return true; }
        }
        return false;
    }
    if (lower == "o-o-o" || lower == "0-0-0") {
        MoveList<Us> moves(pos);
        for (auto &m : moves) {
            if (m.flags() == OOO) { resolved = m; return true; }
        }
        return false;
    }

    char piece_char = 'P';
    bool has_piece = false;
    if (isupper(token[0]) && token[0] != 'O') {
        piece_char = token[0];
        has_piece = true;
    }
    size_t start_idx = 0;
    if (isupper(token[0]) && token[0] != 'O') {
        piece_char = token[0];
        start_idx = 1;
    }

    // Optional disambiguation (file/rank or pawn file in capture)
    char disamb_file = '\0';
    char disamb_rank = '\0';
    for (size_t i = start_idx; i < token.size(); i++) {
        if (token[i] == 'x') continue;
        if (i+1 < token.size() && isalpha(token[i]) && isdigit(token[i+1]))
            break; // hit destination square
        if (isalpha(token[i]) && islower(token[i]))
            disamb_file = token[i];
        else if (isdigit(token[i]))
            disamb_rank = token[i];
    }

    // Try to match by destination square and promotion
    string dest;
    if (token.size() >= 2 && isalpha(token[token.size()-2]) && isdigit(token[token.size()-1]))
        dest = token.substr(token.size()-2);

    char promo = '\0';
    if (token.find('=') != string::npos)
        promo = toupper(token.back());
    else if (token.size() > 2 && isalpha(token.back()) && isupper(token.back()))
        promo = token.back();

    MoveList<Us> moves(pos);
    for (auto &m : moves) {
        string to = square_to_string(m.to());
        if (!dest.empty() && to != dest) continue;

        if (promo) {
            if (m.flags() == PR_QUEEN && promo != 'Q') continue;
            if (m.flags() == PR_ROOK  && promo != 'R') continue;
            if (m.flags() == PR_BISHOP&& promo != 'B') continue;
            if (m.flags() == PR_KNIGHT&& promo != 'N') continue;
        }

        char pl = piece_letter(pos.at(m.from()));
        if (has_piece) {
            // Token specifies a piece, must match exactly
            if (pl != token[0]) continue;
        } else {
            // Token does not specify a piece, must be a pawn move
            if (pl != '\0') continue;
        }

        if (disamb_file && file_of(m.from()) != disamb_file - 'a')
            continue;
        if (disamb_rank && rank_of(m.from()) != disamb_rank - '1')
            continue;

        resolved = m;
        return true;
    }


    for (auto &m : moves) {
        cout << m << " ";
    }
    cout << endl;

    return false;
}

// Tokenize PGN movetext into SAN tokens (ignores move numbers and comments)

static vector<string> tokenize_movetext(const string &movetext) {
    vector<string> tokens;
    string cur;
    for (size_t i = 0; i < movetext.size(); ++i) {
        char c = movetext[i];
        if (c == '{') {
            // skip comment until '}'
            while (i < movetext.size() && movetext[i] != '}') ++i;
            continue;
        }
        if (c == '(') {
            // skip variations (naive: skip until matching ')')
            int depth = 1;
            ++i;
            while (i < movetext.size() && depth > 0) {
                if (movetext[i] == '(') ++depth;
                else if (movetext[i] == ')') --depth;
                ++i;
            }
            --i;
            continue;
        }
        // whitespace or move-number or results separate tokens
        if (isspace((unsigned char)c)) {
            if (!cur.empty()) {
                tokens.push_back(cur);
                cur.clear();
            }
            continue;
        }
        if (c == '.' ) {
            // skip periods in move numbers like "1." or "1..."
            if (!cur.empty()) {
                tokens.push_back(cur);
                cur.clear();
            }
            // skip contiguous dots
            continue;
        }

        // normal character into current token
        cur.push_back(c);
    }
    if (!cur.empty()) tokens.push_back(cur);
    // Remove move numbers like "1.", "23." and tokens like "1-0" we'll treat as results
    vector<string> out;
    std::regex move_num_only(R"(^\d+$|^\d+\.+$)");
    std::regex move_num_with_move(R"(^\d+\.+([a-zA-Z].*)$)");
    std::smatch m;
    for (auto &t : tokens) {
        if (std::regex_match(t, move_num_only)) continue;
        if (std::regex_match(t, m, move_num_with_move)) {
            out.push_back(m[1]);
            continue;
        }
        out.push_back(t);
    }
    return out;
}

// ----------------- PGN parsing -----------------

// Read whole file into string
static bool read_file_to_string(const fs::path &p, string &out) {
    ifstream ifs(p, ios::in | ios::binary);
    if (!ifs) return false;
    std::ostringstream ss;
    ss << ifs.rdbuf();
    out = ss.str();
    return true;
}

// Extract tokens of header and movetext for a single game from a PGN file content starting at pos
// This function is simple: finds next blank-line-separated game (headers then movetext)
static bool extract_next_game(const string &filecontent, size_t &pos, string &headers, string &movetext) {
    size_t n = filecontent.size();
    if (pos >= n) return false;
    // Skip leading whitespace
    while (pos < n && isspace((unsigned char)filecontent[pos])) ++pos;
    if (pos >= n) return false;

    // Headers: consecutive lines beginning with '['
    headers.clear();
    movetext.clear();
    while (pos < n) {
        size_t line_end = filecontent.find('\n', pos);
        if (line_end == string::npos) line_end = n;
        string line = filecontent.substr(pos, line_end - pos);
        if (!line.empty() && line[0] == '[') {
            headers += line + '\n';
            pos = line_end + 1;
        } else break;
    }

    // Now movetext until a blank line separating games or EOF
    bool any = false;
    while (pos < n) {
        size_t line_end = filecontent.find('\n', pos);
        if (line_end == string::npos) line_end = n;
        string line = filecontent.substr(pos, line_end - pos);
        // blank line that separates games
        if (trim(line).empty()) {
            pos = line_end + 1;
            if (any) break;
            else continue;
        }
        movetext += line + ' ';
        any = true;
        pos = line_end + 1;
    }

    return any || !headers.empty();
}

// ----------------- Main tallying logic -----------------

int main(int argc, char** argv) {

    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " <pgn-directory> <output-csv> [max_positions]\n";
        return 2;
    }
    fs::path pgn_dir = argv[1];
    string out_csv = argv[2];
    size_t max_positions = 100000;
    if (argc >= 4) max_positions = stoull(argv[3]);

    zobrist::initialise_zobrist_keys();
    initialise_all_databases();

    // Map from position-hash -> map move_uci -> count
    unordered_map<uint64_t, unordered_map<string, uint32_t>> db;
    db.reserve(1<<20);

    // Also track overall frequencies of positions
    unordered_map<uint64_t, uint64_t> pos_freq;
    pos_freq.reserve(1<<20);

    // Iterate over .pgn files
    for (auto& entry : fs::recursive_directory_iterator(pgn_dir)) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != ".pgn") continue;

        string filecontent;
        if (!read_file_to_string(entry.path(), filecontent)) {
            cerr << "Warning: failed to read " << entry.path() << "\n";
            continue;
        }

        size_t pos = 0;
        string headers, movetext;
        size_t games_in_file = 0;
        while (extract_next_game(filecontent, pos, headers, movetext)) {
            ++games_in_file;
            // Tokenize movetext
            vector<string> tokens = tokenize_movetext(movetext);
            // Initialize starting position (standard chess start)
            Position cur;
            Position::set("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -", cur);

            int tlen = tokens.size();
            tlen = tlen <  18 ? tlen : 18;
            for (size_t t = 0; t < tlen; ++t) {
                string tok = trim(tokens[t]);
                if (tok.empty()) continue;
                // Record current position hash (before applying move)
                uint64_t h = cur.get_hash();
                pos_freq[h] += 1;

                // Resolve SAN token to Move
                Move m;
                bool ok = cur.turn() == WHITE ? resolve_san_to_move<WHITE>(cur, tok, m) : resolve_san_to_move<BLACK>(cur, tok, m);
                if (!ok) {
                    /*cout << cur << endl;
                    cout << headers << endl;
                    cout << movetext  << endl << flush;
                    cout << "Warning: couldn't resolve SAN '" << tok << "' in file " << entry.path()
                         << " (game " << games_in_file << "). Skipping rest of game.\n" << endl;
                    for (auto t : tokens) cout << t << " ";
                    cout << endl;*/
                    break;
                }
                // increment move count for this position
                string uci = move_to_uci(m);
                db[h][uci] += 1;

                // Play move
                if (cur.turn() == WHITE) cur.play<WHITE>(m);
                else cur.play<BLACK>(m);
            }
        }
        cout << "Processed " << games_in_file << " games in " << entry.path() << "\n";
    }

    // Now select top max_positions by pos_freq
    vector<pair<uint64_t, uint64_t>> freq_list;
    freq_list.reserve(pos_freq.size());
    for (auto &p : pos_freq) freq_list.emplace_back(p.first, p.second);
    sort(freq_list.begin(), freq_list.end(), [](auto &a, auto &b){ return a.second > b.second; });

    if (freq_list.size() > max_positions) freq_list.resize(max_positions);

    // Write CSV header and rows
    ofstream ofs(out_csv);
    if (!ofs) {
        cout << "Failed to open output file " << out_csv << "\n";
        return 3;
    }
    ofs << "hash,total_occurrences,move1_uci,move1_count,move2_uci,move2_count,move3_uci,move3_count\n";

    for (auto &p : freq_list) {
        uint64_t h = p.first;
        uint64_t total_occ = p.second;

        // get move counts for this position and pick top 3
        vector<pair<string,uint32_t>> moves;
        auto it = db.find(h);
        if (it != db.end()) {
            for (auto &kv : it->second) moves.emplace_back(kv.first, kv.second);
            sort(moves.begin(), moves.end(), [](auto &a, auto &b){ return a.second > b.second; });
        }

        // pad to 3 moves
        while (moves.size() < 3) moves.emplace_back(string(), 0);

        ofs << h << "," << total_occ << ","
            << moves[0].first << "," << moves[0].second << ","
            << moves[1].first << "," << moves[1].second << ","
            << moves[2].first << "," << moves[2].second << "\n";
    }

    ofs.close();
    cout << "Wrote " << freq_list.size() << " positions to " << out_csv << "\n";
    return 0;
}
