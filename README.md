# Wombat
- UCI chess engine in C++
- PVS with aspiration windows, lock-free transposition table, Lazy SMP
- Null move, reverse futility, futility, late move pruning/reductions, SEE pruning
- NNUE evaluation ((768→256)x2→1, SCReLU, incrementally updated), trained on self-play data;
  the tapered hand-crafted evaluation remains as fallback (`UseNNUE false`)
- Beats Stockfish at UCI_Elo 1800 200-0 (10+0.1), see `benchmarks/` and `Changes.md`

### Build & Run
```
git submodule update --init lib/surge lib/Fathom
cmake -S . -B build-rel -DCMAKE_BUILD_TYPE=Release && cmake --build build-rel -j
./build-rel/Wombat          # UCI mode (use with any chess GUI)
./build-rel/Wombat play     # play against it in the terminal
./build-rel/Wombat bench    # node count / speed check
```

### Web demo
Play against Wombat in the browser: https://fabsch225.github.io/Wombat/ (multithreaded WebAssembly).
```
web/build.sh                 # needs Emscripten; builds build-wasm/site
python3 web/serve.py         # local test server on :8000 with the COOP/COEP headers
```
The site is deployed from the `gh-pages` branch (contents of `build-wasm/site`).

### Training the network
```
./build-rel/Wombat datagen nnue/data/d1.txt 10000 5000 1   # <out> <games> <nodes/move> <seed>, run several in parallel
python3 nnue/train.py --data "nnue/data/*.txt" --cache nnue/data/cache.npz --out nnue/wombat.nnue --epochs 20
# rebuild: nnue/wombat.nnue is embedded into the binary
```

### Third Party Libraries
- [surge](https://github.com/nkarve/surge), slightly modified (bitboards, move generation, zobrist hashing)
- [Fathom](https://github.com/jdart1/Fathom) (probing of endgame-tablebases)
