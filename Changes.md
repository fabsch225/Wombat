# Changes

Goal: make Wombat play at 1800 Elo or better. The old search and evaluation were replaced,
the code was split into small modules, and the result was measured against Stockfish.

## Correctness fixes
- **Hashing**: surge's hash only covers where the pieces stand. The engine now uses a full key
  that also covers side to move, castling rights and en passant. Before, the transposition
  table mixed up unrelated positions.
- **Move handling**: worked around three surge problems. `is_capture()` is true for castles
  and double pushes. Castling is stored as "king takes rook". FEN import/export drops
  en passant and move counters.
- **Game length**: surge's undo history has a fixed size, so long games could overflow it.
  The board is now rebuilt before that can happen.
- **Draws**: the search now detects threefold repetition, the fifty-move rule and
  insufficient material.
- **Build**: the project now compiles and links with clang/macOS as well as GCC. A small
  shim header adds attack functions that surge declares but does not define in its headers.
  The surge submodule itself is unchanged.
- **Threads**: removed the broken thread-pool parallel search.

## Search
- Rewrote the search as iterative deepening principal-variation search with aspiration windows.
- Added the usual pruning and reduction techniques: null move, reverse futility, razoring,
  futility pruning, late move pruning and reductions, SEE-based pruning, and reducing depth
  when no TT move exists. Checks are extended.
- Move ordering now uses the TT move, captures ranked by static exchange evaluation and
  MVV-LVA, killer moves, counter moves and a history table.
- Quiescence search now handles check evasions and uses delta pruning and SEE pruning.
- New transposition table: fixed size, lock-free, in buckets, with aging and mate scores
  adjusted per ply.
- Multithreading now uses Lazy SMP: threads share only the transposition table.
- Time management uses soft and hard limits and adapts to how stable the best move is.

## Evaluation
- Tapered midgame/endgame evaluation built on the PeSTO piece-square tables.
- Added pawn structure (passed, isolated and doubled pawns), mobility, bishop pair,
  rooks on open files, king safety (attacks on the king zone, pawn shield) and tempo.
- Endgame knowledge: drawish material is scaled down, and a mop-up term drives the
  defending king to the edge in won endgames without pawns.

## NNUE
- New evaluation: a (768 -> 256)x2 -> 1 network with SCReLU activation (`src/nnue.*`). The
  accumulator is updated incrementally on every move (including castling, en passant and
  promotions) and copied across null moves. It is quantised to int16 and embedded into the binary
  at build time from `nnue/wombat.nnue`. Without that file the engine uses the classical evaluation.
- UCI options `UseNNUE` and `EvalFile`; `eval` prints both evaluations.
- Training data: `Wombat datagen` plays self-play games at 5000 nodes per move from 8 random
  opening plies and keeps quiet positions with their search score and the game result.
  The current network was trained on 8.2M positions (90k games) for 20 epochs with
  `nnue/train.py` (target: 70% search score + 30% game result, validation loss 0.00753).
- Checked: incremental updates match a full refresh on 199k positions (perft walk to depth 3).
  Search speed went up from ~1.5M to ~1.85M nodes/s, because the network is cheaper than
  the hand-crafted evaluation.

## Interface & structure
- Full UCI support, so Wombat works in any chess GUI and in match runners. Options:
  Hash, Threads, Move Overhead, OwnBook, BookFile, SyzygyPath, UseNNUE, EvalFile. Extra commands: `perft`, `bench`, `d`, `eval`.
- The interactive human-vs-engine mode is still available as `Wombat play`.
- Opening book and Syzygy tablebases are optional and configured through UCI options
  instead of hardcoded paths.
- Code layout: `board` (position helpers), `eval`, `see`, `movepick`, `tt`, `timeman`,
  `search`, `uci`, `nnue`, `datagen`, `OpeningDB`, `EndgameDB`.

## Validation
- Move generation checked with perft on the standard test positions; all counts match
  the published reference numbers.
- Matches against Stockfish 19 in limited-strength mode (UCI_Elo), 10s+0.1s, varied openings
  (`benchmarks/`):

  | Opponent        | Games | Result (W-L-D) | Score |
  |-----------------|-------|----------------|-------|
  | Stockfish @1800 | 200   | 200-0-0        | 100%  |
  | Stockfish @2400 | 120   | 110-7-3        | 93%   |
  | Stockfish @3000 | 80    | 13-41-26       | 33%   |

  On Stockfish's own Elo scale that puts Wombat around 2850 at this time control. That scale
  probably overstates strength at fast time controls, but Wombat is clearly well past the
  1800 target. Rerun with `benchmarks/run_match.sh <elo>`.
- Before/after NNUE, 60 games each against Stockfish @3000, 10s+0.1s, same settings
  (`benchmarks/results/before_nnue_60`, `benchmarks/results/after_nnue_60`):

  | Evaluation          | Games | Result (W-L-D) | Score | Elo diff vs SF3000 |
  |---------------------|-------|----------------|-------|--------------------|
  | Classical (before)  | 60    | 9-33-18        | 30.0% | -147 ± 69          |
  | NNUE (after)        | 60    | 9-35-16        | 28.3% | -161 ± 72          |

  The two results are within error of each other, so there is no measurable gain yet. That is expected
  for a first network: it learned from the classical engine's own search scores, so it mostly
  reproduces that evaluation. More data, deeper search when generating it, and retraining on
  NNUE self-play are the usual ways to make a network stronger than its teacher.
