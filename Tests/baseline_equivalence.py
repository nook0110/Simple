#!/usr/bin/env python3

"""Compare the tunable engine's explicit control mode with pristine upstream."""

import argparse
import subprocess
from pathlib import Path

from pawn_tuner import BASELINE_PARAMETERS


class Engine:
    def __init__(self, executable: Path, options: dict[str, int]) -> None:
        self.process = subprocess.Popen(
            [str(executable.resolve())],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
        )
        self.send("uci")
        self.read_until("uciok")
        for name, value in options.items():
            self.send(f"setoption name {name} value {value}")
        self.send("isready")
        self.read_until("readyok")

    def send(self, command: str) -> None:
        assert self.process.stdin is not None
        self.process.stdin.write(command + "\n")
        self.process.stdin.flush()

    def read_until(self, prefix: str) -> str:
        assert self.process.stdout is not None
        while True:
            line = self.process.stdout.readline()
            if not line:
                raise RuntimeError(
                    f"engine exited before producing {prefix!r}"
                )
            line = line.rstrip()
            if line.startswith(prefix):
                return line

    def evaluate(self, fen: str) -> str:
        self.send(f"position fen {fen}")
        self.send("go evaluate")
        return self.read_until("eval: ")

    def close(self) -> None:
        if self.process.poll() is None:
            self.send("quit")
            self.process.wait(timeout=10)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--candidate", type=Path, required=True)
    parser.add_argument("--upstream", type=Path, required=True)
    parser.add_argument("--positions", type=Path, required=True)
    parser.add_argument("--limit", type=int, default=0)
    args = parser.parse_args()

    positions = [
        line.split("#", 1)[0].strip()
        for line in args.positions.read_text().splitlines()
        if line.strip() and not line.lstrip().startswith("#")
    ]
    if args.limit:
        positions = positions[:args.limit]

    candidate = Engine(args.candidate, BASELINE_PARAMETERS)
    upstream = Engine(args.upstream, {})
    try:
        for index, fen in enumerate(positions, start=1):
            candidate_eval = candidate.evaluate(fen)
            upstream_eval = upstream.evaluate(fen)
            if candidate_eval != upstream_eval:
                raise RuntimeError(
                    f"position {index} differs: {fen}\n"
                    f"candidate control: {candidate_eval}\n"
                    f"pristine upstream: {upstream_eval}"
                )
    finally:
        candidate.close()
        upstream.close()

    print(f"matched {len(positions)} positions")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
