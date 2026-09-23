#!/usr/bin/env python3
"""
Trains Wombat's NNUE: (768 -> 256)x2 -> 1 with SCReLU, and exports the quantised network.

Input: text files written by `Wombat datagen`, one position per line:
    <fen> | <score, white pov, centipawns> | <result, white pov: 1.0 / 0.5 / 0.0>

Usage:
    python3 nnue/train.py --data data/*.txt --out nnue/wombat.nnue [--epochs 20]

The binary layout must match src/nnue.cpp:
    int16 feature_weights[768 * 256]   (scaled by QA)
    int16 feature_bias[256]            (scaled by QA)
    int16 output_weights[512]          (scaled by QB; side to move first)
    int32 output_bias                  (scaled by QA * QB)
"""

import argparse
import glob
import os
import time
from multiprocessing import Pool

import numpy as np
import torch
import torch.nn as nn

INPUTS, HIDDEN = 768, 256
QA, QB, SCALE = 255, 64, 400
MAX_PIECES = 32
PAD = INPUTS  # index of the all-zero padding row

PIECE_INDEX = {c: i for i, c in enumerate("pnbrqk")}


def encode(line):
    """Returns (stm features, nstm features, stm score, stm result) or None."""
    try:
        fen, score, result = [part.strip() for part in line.split("|")]
        score, result = int(score), float(result)
    except ValueError:
        return None
    fields = fen.split()
    board, stm_white = fields[0], fields[1] == "w"

    stm = np.full(MAX_PIECES, PAD, dtype=np.int16)
    nstm = np.full(MAX_PIECES, PAD, dtype=np.int16)
    n = 0
    rank, file = 7, 0
    for ch in board:
        if ch == "/":
            rank, file = rank - 1, 0
        elif ch.isdigit():
            file += int(ch)
        else:
            sq = rank * 8 + file
            white_piece = ch.isupper()
            pt = PIECE_INDEX[ch.lower()]
            # White perspective and black perspective (board flipped vertically)
            white_feat = (0 if white_piece else 1) * 384 + pt * 64 + sq
            black_feat = (1 if white_piece else 0) * 384 + pt * 64 + (sq ^ 56)
            stm[n], nstm[n] = (white_feat, black_feat) if stm_white else (black_feat, white_feat)
            n += 1
            file += 1
    if not stm_white:
        score, result = -score, 1.0 - result
    return stm, nstm, score, result


def encode_file(path):
    rows = [encode(line) for line in open(path)]
    rows = [r for r in rows if r is not None]
    return (np.stack([r[0] for r in rows]), np.stack([r[1] for r in rows]),
            np.array([r[2] for r in rows], dtype=np.float32), np.array([r[3] for r in rows], dtype=np.float32))


def load(paths, cache):
    if cache and os.path.exists(cache):
        d = np.load(cache)
        return d["stm"], d["nstm"], d["score"], d["result"]
    with Pool() as pool:
        parts = pool.map(encode_file, paths)
    stm, nstm, score, result = (np.concatenate([p[i] for p in parts]) for i in range(4))
    if cache:
        np.savez(cache, stm=stm, nstm=nstm, score=score, result=result)
    return stm, nstm, score, result


class Net(nn.Module):
    def __init__(self):
        super().__init__()
        self.ft = nn.Embedding(INPUTS + 1, HIDDEN, padding_idx=PAD)
        self.ft_bias = nn.Parameter(torch.zeros(HIDDEN))
        self.out = nn.Linear(2 * HIDDEN, 1)
        nn.init.normal_(self.ft.weight, std=0.05)
        with torch.no_grad():
            self.ft.weight[PAD].zero_()

    def forward(self, stm, nstm):
        a = self.ft(stm).sum(1) + self.ft_bias
        b = self.ft(nstm).sum(1) + self.ft_bias
        h = torch.cat([a, b], 1).clamp(0, 1).pow(2)  # SCReLU
        return self.out(h).squeeze(1)

    def clip(self):
        # Keeps quantised values inside int16 (see src/nnue.cpp)
        with torch.no_grad():
            self.ft.weight.clamp_(-1.98, 1.98)
            self.out.weight.clamp_(-1.98, 1.98)


def export(net, path):
    ft = net.ft.weight.detach().cpu().numpy()[:INPUTS]
    fb = net.ft_bias.detach().cpu().numpy()
    ow = net.out.weight.detach().cpu().numpy().reshape(-1)
    ob = net.out.bias.detach().cpu().numpy()[0]
    with open(path, "wb") as f:
        f.write(np.round(ft * QA).astype("<i2").tobytes())
        f.write(np.round(fb * QA).astype("<i2").tobytes())
        f.write(np.round(ow * QB).astype("<i2").tobytes())
        f.write(np.array([round(ob * QA * QB)], dtype="<i4").tobytes())


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--data", nargs="+", required=True)
    ap.add_argument("--out", default="nnue/wombat.nnue")
    ap.add_argument("--cache", default=None, help="optional .npz cache of the encoded data")
    ap.add_argument("--epochs", type=int, default=20)
    ap.add_argument("--batch", type=int, default=16384)
    ap.add_argument("--lr", type=float, default=1e-3)
    # CPU is fast enough (~1 min/epoch for 8M positions); MPS hit GPU watchdog hangs on long runs
    ap.add_argument("--device", default="cpu")
    ap.add_argument("--wdl", type=float, default=0.3, help="weight of the game result vs. the search score")
    args = ap.parse_args()

    paths = sorted(p for pattern in args.data for p in glob.glob(pattern))
    t = time.time()
    stm, nstm, score, result = load(paths, args.cache)
    n = len(score)
    print(f"{n} positions loaded in {time.time() - t:.0f}s")

    device = args.device
    stm_t = torch.from_numpy(stm)
    nstm_t = torch.from_numpy(nstm)
    target = torch.from_numpy((1 - args.wdl) / (1 + np.exp(-score / SCALE)) + args.wdl * result).float()

    # Hold out 1% for validation
    perm = torch.randperm(n)
    val_idx, train_idx = perm[: n // 100], perm[n // 100:]

    net = Net().to(device)
    opt = torch.optim.Adam(net.parameters(), lr=args.lr)
    sched = torch.optim.lr_scheduler.CosineAnnealingLR(opt, args.epochs)

    def batch_loss(idx):
        s, ns = stm_t[idx].long().to(device), nstm_t[idx].long().to(device)
        y = target[idx].to(device)
        return ((torch.sigmoid(net(s, ns)) - y) ** 2).mean()

    for epoch in range(args.epochs):
        t = time.time()
        net.train()
        order = train_idx[torch.randperm(len(train_idx))]
        total, batches = 0.0, 0
        for i in range(0, len(order), args.batch):
            loss = batch_loss(order[i:i + args.batch])
            opt.zero_grad()
            loss.backward()
            opt.step()
            net.clip()
            total += loss.item()
            batches += 1
        sched.step()
        net.eval()
        with torch.no_grad():
            val = np.mean([batch_loss(val_idx[i:i + args.batch]).item() for i in range(0, len(val_idx), args.batch)])
        print(f"epoch {epoch + 1:2d}  train {total / batches:.5f}  val {val:.5f}  {time.time() - t:.0f}s", flush=True)
        export(net, args.out)

    print(f"wrote {args.out}")


if __name__ == "__main__":
    main()
