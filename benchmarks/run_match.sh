#!/usr/bin/env bash
# Plays Wombat against Stockfish with limited strength.
# Usage: benchmarks/run_match.sh <stockfish-elo> [rounds] [tc]
# Requires fastchess (https://github.com/Disservin/fastchess) and stockfish on PATH,
# or set FASTCHESS / STOCKFISH. Build Wombat first (cmake -B build-rel && cmake --build build-rel).
set -euo pipefail

ELO=${1:?usage: run_match.sh <stockfish-elo> [rounds] [tc]}
ROUNDS=${2:-100}
TC=${3:-10+0.1}
DIR="$(cd "$(dirname "$0")" && pwd)"
WOMBAT=${WOMBAT:-$DIR/../build-rel/Wombat}
FASTCHESS=${FASTCHESS:-fastchess}
STOCKFISH=${STOCKFISH:-stockfish}
OUT="$DIR/results/${LABEL:-latest}/wombat_vs_sf${ELO}"

mkdir -p "$(dirname "$OUT")"
"$FASTCHESS" \
    -engine cmd="$WOMBAT" name=Wombat \
    -engine cmd="$STOCKFISH" name="SF$ELO" option.UCI_LimitStrength=true option.UCI_Elo="$ELO" \
    -each tc="$TC" option.Hash=32 \
    -rounds "$ROUNDS" -games 2 -repeat -concurrency "${CONCURRENCY:-8}" \
    -openings file="$DIR/book.epd" format=epd order=random \
    -pgnout file="$OUT.pgn" -recover | tee "$OUT.log"
