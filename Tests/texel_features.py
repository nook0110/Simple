#!/usr/bin/env python3

import argparse
import json
import multiprocessing
import re
import subprocess
from pathlib import Path

import numpy as np

from pawn_tuner import PARAMETERS


EVAL_RE = re.compile(r"eval: (-?\d+) cp")
QEVAL_RE = re.compile(r"qeval white_cp (-?\d+) nodes (\d+)")
SPLITS = {"train": 0, "validation": 1, "holdout": 2}


class Engine:
    def __init__(self, executable: Path) -> None:
        self.process = subprocess.Popen(
            [str(executable)], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT, text=True, bufsize=1,
        )
        self.send("uci")
        self.read("uciok")
        for name, parameter in PARAMETERS.items():
            self.set_option(name, parameter.default)
        self.send("isready")
        self.read("readyok")

    def send(self, command: str) -> None:
        assert self.process.stdin is not None
        self.process.stdin.write(command + "\n")
        self.process.stdin.flush()

    def read(self, prefix: str) -> str:
        assert self.process.stdout is not None
        while True:
            raw_line = self.process.stdout.readline()
            if raw_line == "" and self.process.poll() is not None:
                raise RuntimeError(f"engine exited before {prefix!r}")
            line = raw_line.strip()
            if line.startswith(prefix):
                return line

    def set_option(self, name: str, value: int) -> None:
        self.send(f"setoption name {name} value {value}")

    def set_position(self, fen: str) -> None:
        self.send(f"position fen {fen}")

    def qeval(self) -> tuple[int, int]:
        self.send("go qeval")
        line = self.read("qeval ")
        match = QEVAL_RE.fullmatch(line)
        if match is None:
            raise RuntimeError(f"invalid qeval output: {line}")
        return int(match.group(1)), int(match.group(2))

    def evaluate(self, white_to_move: bool) -> int:
        self.send("go evaluate")
        line = self.read("eval: ")
        match = EVAL_RE.fullmatch(line)
        if match is None:
            raise RuntimeError(f"invalid evaluate output: {line}")
        value = int(match.group(1))
        return value if white_to_move else -value

    def close(self) -> None:
        if self.process.poll() is None:
            self.send("quit")
            self.process.wait(timeout=10)


def score_chunk(arguments):
    engine_path, records, include_tactical = arguments
    engine = Engine(Path(engine_path))
    names = tuple(PARAMETERS)
    rows = []
    try:
        for record in records:
            fen = record["fen"]
            engine.set_position(fen)
            qscore, nodes = engine.qeval()
            if abs(qscore) >= 2000:
                continue
            white_to_move = fen.split()[1] == "w"
            baseline = qscore
            if not include_tactical and nodes != 1:
                continue
            if not include_tactical and engine.evaluate(white_to_move) != qscore:
                raise RuntimeError(
                    f"quiet qscore mismatch for {fen}"
                )
            features = []
            for name in names:
                parameter = PARAMETERS[name]
                direction = 1
                mutated = parameter.default + 1
                if mutated > parameter.maximum:
                    direction = -1
                    mutated = parameter.default - 1
                engine.set_option(name, mutated)
                changed = (
                    engine.qeval()[0]
                    if include_tactical else engine.evaluate(white_to_move)
                )
                engine.set_option(name, parameter.default)
                features.append(direction * (changed - baseline))
            rows.append((
                features, baseline, record["result"],
                SPLITS[record["split"]], record["game_id"], fen,
            ))
    finally:
        engine.close()
    return rows


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--engine", type=Path, required=True)
    parser.add_argument("--dataset", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--workers", type=int, default=1)
    parser.add_argument("--max-positions", type=int, default=0)
    parser.add_argument("--include-tactical", action="store_true")
    args = parser.parse_args()

    records = [json.loads(line) for line in args.dataset.read_text().splitlines()]
    if args.max_positions:
        records = records[:args.max_positions]
    chunks = [records[index::args.workers] for index in range(args.workers)]
    with multiprocessing.Pool(args.workers) as pool:
        nested_rows = pool.map(
            score_chunk,
            [(str(args.engine.resolve()), chunk, args.include_tactical)
             for chunk in chunks],
        )
    rows = [row for chunk in nested_rows for row in chunk]
    if not rows:
        raise RuntimeError("no quiet Texel positions were extracted")
    names = np.array(tuple(PARAMETERS))
    defaults = np.array([PARAMETERS[name].default for name in names])
    steps = np.ones(len(names), dtype=np.int64)
    minimums = np.array([PARAMETERS[name].minimum for name in names])
    maximums = np.array([PARAMETERS[name].maximum for name in names])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    np.savez_compressed(
        args.output,
        features=np.asarray([row[0] for row in rows], dtype=np.float32),
        baseline=np.asarray([row[1] for row in rows], dtype=np.float32),
        results=np.asarray([row[2] for row in rows], dtype=np.float32),
        splits=np.asarray([row[3] for row in rows], dtype=np.uint8),
        game_ids=np.asarray([row[4] for row in rows]),
        fens=np.asarray([row[5] for row in rows]),
        names=names,
        defaults=defaults,
        steps=steps,
        minimums=minimums,
        maximums=maximums,
    )
    counts = {name: int(sum(row[3] == value for row in rows))
              for name, value in SPLITS.items()}
    print(json.dumps({
        "input_positions": len(records),
        "scored_positions": len(rows),
        "include_tactical": args.include_tactical,
        "parameters": len(names),
        "splits": counts,
    }, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
