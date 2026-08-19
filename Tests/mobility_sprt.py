#!/usr/bin/env python3

import argparse
import json
import math
import os
import re
import resource
import subprocess
from pathlib import Path

from mobility_tuner import PARAMETERS
from search_tuner import SCORE_RE, cutechess_llr, elo


DECISION_RE = re.compile(r"SPRT: llr .+ - (H[01]) was accepted")
TECHNICAL_RE = re.compile(
    r"illegal move|loses on time|disconnects|crash|timeout|unexpected move",
    re.IGNORECASE,
)


def option_arguments(values: dict[str, int]) -> list[str]:
    return [f"option.{name}={value}" for name, value in values.items()]


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, required=True)
    parser.add_argument("--engine", type=Path, required=True)
    parser.add_argument("--parameters", required=True)
    parser.add_argument("--concurrency", type=int, default=72)
    parser.add_argument("--max-games", type=int, default=10000)
    parser.add_argument("--seed", type=int, default=470000)
    parser.add_argument("--timemargin", type=int, default=1000)
    parser.add_argument("--label", required=True)
    args = parser.parse_args()

    root = args.root.resolve()
    engine = args.engine
    if not engine.is_absolute():
        engine = (root / engine).resolve()
    baseline = {name: 0 for name in PARAMETERS}
    candidate = {name: parameter.default for name, parameter in PARAMETERS.items()}
    candidate.update(json.loads(args.parameters))
    log = root / "logs" / f"{args.label}.log"
    pgn = root / "pgn" / f"{args.label}.pgn"
    command = [
        str(root / "cutechess-cli"),
        "-engine", "name=Candidate", f"cmd={engine}", "proto=uci",
        "option.Threads=1", *option_arguments(candidate), "restart=on",
        f"timemargin={args.timemargin}",
        "-engine", "name=Baseline", f"cmd={engine}", "proto=uci",
        "option.Threads=1", *option_arguments(baseline), "restart=on",
        f"timemargin={args.timemargin}",
        "-each", "tc=inf/8+0.08", "book=gm2001.bin", "bookdepth=10",
        "-games", "2", "-rounds", str(args.max_games // 2), "-repeat", "2",
        "-maxmoves", "200", "-concurrency", str(args.concurrency),
        "-srand", str(args.seed), "-ratinginterval", "100",
        "-sprt", "elo0=0", "elo1=10", "alpha=0.05", "beta=0.05",
        "-pgnout", str(pgn),
    ]
    environment = os.environ.copy()
    environment["LD_LIBRARY_PATH"] = str(root / "lib")

    def configure_limits() -> None:
        resource.setrlimit(resource.RLIMIT_NOFILE, (65536, 65536))

    with log.open("w") as output:
        process = subprocess.run(
            command, cwd=root, stdout=output, stderr=subprocess.STDOUT,
            env=environment, preexec_fn=configure_limits,
        )
    if process.returncode != 0:
        raise RuntimeError(f"cutechess exited with {process.returncode}: {log}")
    text = log.read_text(errors="replace")
    technical = TECHNICAL_RE.search(text)
    if technical:
        line = text[:technical.end()].splitlines()[-1]
        raise RuntimeError(f"technical result in {log.name}: {line}")
    matches = list(SCORE_RE.finditer(text))
    decision = DECISION_RE.search(text)
    if not matches or decision is None or "Finished match" not in text:
        raise RuntimeError(f"incomplete SPRT match: {log}")
    wins, losses, draws, games = map(int, matches[-1].groups())
    result = (wins, losses, draws)
    bound = math.log(0.95 / 0.05)
    evidence = {
        "decision": decision.group(1),
        "elo": elo(result),
        "games": games,
        "llr": cutechess_llr(*result, 0.0, 10.0),
        "lower": -bound,
        "parameters": candidate,
        "upper": bound,
        "wld": result,
    }
    print(json.dumps(evidence, sort_keys=True))
    return 0 if decision.group(1) == "H1" else 2


if __name__ == "__main__":
    raise SystemExit(main())
