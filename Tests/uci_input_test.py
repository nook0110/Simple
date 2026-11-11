#!/usr/bin/env python3

import argparse
import subprocess
import sys

CASES = [
    ("disable small NNUE", ["setoption name EvalFileSmall value none"], "info string small NNUE disabled"),
    ("small NNUE option is case insensitive", ["setoption name evalfilesmall value none"], "info string small NNUE disabled"),
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
