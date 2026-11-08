#!/usr/bin/env python3

import argparse
import subprocess
import sys

CASES = [
    ("unknown option", ["setoption name Hash value 64"], "info string unknown option or invalid value"),
    ("option name with spaces", ["setoption name Clear Hash"], "info string unknown option or invalid value"),
    ("option name is case insensitive", ["setoption name threads value 2"], None),
    ("non-numeric spin value", ["setoption name Threads value abc"], "info string unknown option or invalid value"),
    ("missing spin value", ["setoption name Threads value"], "info string unknown option or invalid value"),
    ("go depth without a number", ["position startpos", "go depth", "stop"], "bestmove "),
    ("go movetime without a number", ["position startpos", "go movetime", "stop"], "bestmove "),
    ("short fen followed by moves", ["position fen 4k3/8/8/8/8/8/4P3/4K3 w - - moves e2e4", "debug"],
     "4k3/8/8/8/4P3/8/8/4K3 b"),
    ("fen without clocks", ["position fen 4k3/8/8/8/8/8/4P3/4K3 w -", "debug"], "4k3/8/8/8/8/8/4P3/4K3 w - - 0 1"),
    ("trailing space after moves", ["position startpos moves e2e4 ", "debug"],
     "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq"),
    ("invalid initial fen", ["position fen ?", "debug"],
     "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"),
    ("castling right without its rook", ["position fen 4k3/8/8/8/8/8/8/4K3 w K - 0 1"],
     "info string invalid fen"),
    ("en passant square without a pushed pawn", ["position fen 4k3/8/8/8/8/8/8/4K3 w - e6 0 1"],
     "info string invalid fen"),
    ("side to move can capture the king", ["position fen 7k/8/8/8/3Q4/8/8/K7 w - - 0 1"],
     "info string invalid fen"),
    ("pawn on the back rank", ["position fen P3k3/8/8/8/8/8/8/4K3 w - - 0 1"], "info string invalid fen"),
    ("two white kings", ["position fen 4k3/8/8/8/8/8/8/3KK3 w - - 0 1"], "info string invalid fen"),
    ("illegal move", ["position startpos moves e2e5", "debug"], "info string illegal move e2e5"),
    ("shredder castling file is ignored", ["position fen 4k3/8/8/8/8/8/8/6KR w H - 0 1", "debug"],
     "4k3/8/8/8/8/8/8/6KR w - - 0 1"),
    ("en passant after a double push",
     ["position fen rnbqkbnr/ppp1pppp/8/8/3pP3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 3", "debug"],
     "rnbqkbnr/ppp1pppp/8/8/3pP3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 3"),
]


def run_case(engine: str, commands: list[str]) -> subprocess.CompletedProcess:
    script = "\n".join(["uci", *commands, "isready", "quit"]) + "\n"
    return subprocess.run([engine], input=script, capture_output=True, text=True, timeout=30)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--engine", required=True)
    args = parser.parse_args()

    failures = []
    for name, commands, expected in CASES:
        result = run_case(args.engine, commands)
        if result.returncode != 0:
            failures.append(f"{name}: engine exited with {result.returncode}")
        elif "readyok" not in result.stdout:
            failures.append(f"{name}: no readyok")
        elif expected is not None and expected not in result.stdout:
            failures.append(f"{name}: expected {expected!r} in output")

    for failure in failures:
        print(failure, file=sys.stderr)
    if failures:
        return 1
    print("UCI input handling passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
