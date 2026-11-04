#!/usr/bin/env python3

import argparse
import json
import re
import statistics
import subprocess
import time
from pathlib import Path


NODES_RE = re.compile(r"\bnodes (\d+)\b")
NPS_RE = re.compile(r"\bnps (\d+)\b")
POSITIONS = (
    "startpos",
    "fen r3k2r/p1ppqpb1/bn2pnp1/2pP4/1p2P3/2N2N2/PPPBBPPP/"
    "R2Q1RK1 w kq - 0 1",
    "fen 8/5pk1/3p2p1/2pPp3/2P1P1P1/5PK1/8/8 w - - 0 1",
)
MATERIAL_DEFAULTS = {
    "PawnValueMG": 82,
    "PawnValueEG": 94,
    "KnightValueMG": 337,
    "KnightValueEG": 281,
    "BishopValueMG": 365,
    "BishopValueEG": 297,
    "RookValueMG": 477,
    "RookValueEG": 512,
    "QueenValueMG": 1025,
    "QueenValueEG": 936,
}


def read_until(process: subprocess.Popen, prefix: str) -> list[str]:
    lines = []
    assert process.stdout is not None
    while True:
        line = process.stdout.readline()
        if not line:
            raise RuntimeError(f"engine exited before {prefix!r}")
        line = line.rstrip()
        lines.append(line)
        if line.startswith(prefix):
            return lines


def send(process: subprocess.Popen, command: str) -> None:
    assert process.stdin is not None
    process.stdin.write(command + "\n")
    process.stdin.flush()


def measure(engine: Path, position: str, depth: int,
            parameters: dict[str, int]) -> dict[str, float | int]:
    process = subprocess.Popen(
        [str(engine)], text=True, stdin=subprocess.PIPE,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
    )
    try:
        send(process, "uci")
        read_until(process, "uciok")
        for name, value in parameters.items():
            send(process, f"setoption name {name} value {value}")
        send(process, "isready")
        read_until(process, "readyok")
        send(process, f"position {position}")
        started = time.perf_counter()
        send(process, f"go depth {depth}")
        output = read_until(process, "bestmove ")
        elapsed = time.perf_counter() - started
        node_matches = [NODES_RE.search(line) for line in output]
        nps_matches = [NPS_RE.search(line) for line in output]
        searched = max(
            int(match.group(1)) for match in node_matches if match is not None
        )
        reported_nps = int(next(
            match.group(1) for match in reversed(nps_matches)
            if match is not None
        ))
        return {
            "nodes": searched,
            "reported_nps": reported_nps,
            "wall_nps": searched / elapsed,
        }
    finally:
        if process.poll() is None:
            send(process, "quit")
            process.wait(timeout=10)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--engine", type=Path, required=True)
    parser.add_argument("--state", type=Path, required=True)
    parser.add_argument("--depth", type=int, default=14)
    parser.add_argument("--repeats", type=int, default=5)
    args = parser.parse_args()

    parameters = json.loads(args.state.read_text())["parameters"]
    # Always set every evaluation option explicitly.  In particular, do not
    # let the benchmark depend on whether the executable has the candidate or
    # the pre-change values compiled in as its UCI defaults.
    baseline = {
        name: MATERIAL_DEFAULTS[name] if name in MATERIAL_DEFAULTS else 0
        for name in parameters
    }
    pawn = dict(baseline)
    pawn.update({
        name: value for name, value in parameters.items()
        if name not in MATERIAL_DEFAULTS
    })
    material = dict(baseline)
    material.update({
        name: value for name, value in parameters.items()
        if name in MATERIAL_DEFAULTS
    })
    modes = {
        "baseline": baseline,
        "pawn": pawn,
        "material": material,
        "candidate": parameters,
    }
    samples = {mode: {} for mode in modes}
    for repeat in range(args.repeats):
        order = tuple(modes) if repeat % 2 == 0 else tuple(reversed(modes))
        for position_index, position in enumerate(POSITIONS):
            for mode in order:
                sample = measure(
                    args.engine.resolve(), position, args.depth,
                    modes[mode],
                )
                samples[mode][(repeat, position_index)] = sample
                print(json.dumps({
                    "mode": mode,
                    "position": position,
                    "repeat": repeat,
                    **sample,
                }, sort_keys=True), flush=True)

    ratios = {}
    for mode in modes:
        if mode == "baseline":
            continue
        reported_ratios = []
        wall_ratios = []
        for key, sample in samples[mode].items():
            baseline = samples["baseline"][key]
            reported_ratios.append(
                sample["reported_nps"] / baseline["reported_nps"]
            )
            wall_ratios.append(sample["wall_nps"] / baseline["wall_nps"])
        ratios[mode] = {
            "median_reported_nps_ratio": statistics.median(reported_ratios),
            "median_wall_nps_ratio": statistics.median(wall_ratios),
        }
    print(json.dumps({
        "ratios": ratios,
        "samples_per_mode": len(samples["baseline"]),
    }, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
