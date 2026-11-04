#!/usr/bin/env python3

import argparse
import json
from pathlib import Path

from mobility_tuner import PARAMETERS
from search_tuner import collect_match, cutechess_llr, elo, launch_match


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, required=True)
    parser.add_argument("--engine", type=Path, required=True)
    parser.add_argument("--parameters", required=True)
    parser.add_argument("--baseline-parameters", default="{}")
    parser.add_argument("--games", type=int, default=256)
    parser.add_argument("--concurrency", type=int, default=48)
    parser.add_argument("--seed", type=int, default=420000)
    parser.add_argument("--timemargin", type=int, default=1000)
    parser.add_argument("--label", required=True)
    args = parser.parse_args()

    root = args.root.resolve()
    engine = args.engine
    if not engine.is_absolute():
        engine = (root / engine).resolve()
    baseline = {name: 0 for name in PARAMETERS}
    baseline.update(json.loads(args.baseline_parameters))
    candidate = dict(baseline)
    candidate.update(json.loads(args.parameters))
    process, output, log = launch_match(
        root, engine, candidate, baseline, args.games, args.concurrency,
        args.seed, args.label, args.timemargin,
    )
    result = collect_match(process, output, log, args.games)
    evidence = {
        "elo": elo(result),
        "games": args.games,
        "llr_h0_0_h1_10": cutechess_llr(*result, 0.0, 10.0),
        "parameters": candidate,
        "wld": result,
    }
    print(json.dumps(evidence, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
