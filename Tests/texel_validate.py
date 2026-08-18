#!/usr/bin/env python3

import argparse
import json
from pathlib import Path

from pawn_tuner import PARAMETERS
from search_tuner import collect_match, cutechess_llr, elo, launch_match


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, required=True)
    parser.add_argument("--engine", type=Path, default=Path("texel-engine"))
    parser.add_argument("--state", type=Path, required=True)
    parser.add_argument("--games", type=int, default=256)
    parser.add_argument("--concurrency", type=int, default=72)
    parser.add_argument("--seed", type=int, default=91000)
    parser.add_argument("--timemargin", type=int, default=1000)
    parser.add_argument("--label", default="texel-v-default")
    args = parser.parse_args()

    root = args.root.resolve()
    engine = args.engine
    if not engine.is_absolute():
        engine = (root / engine).resolve()
    state = json.loads(args.state.read_text())
    candidate = state["parameters"]
    baseline = {name: parameter.default for name, parameter in PARAMETERS.items()}
    process, output, log = launch_match(
        root, engine, candidate, baseline, args.games, args.concurrency,
        args.seed, args.label, args.timemargin,
    )
    result = collect_match(process, output, log, args.games)
    validation = {
        "games": args.games,
        "wld": result,
        "elo": elo(result),
        "llr_h0_0_h1_10": cutechess_llr(*result, 0.0, 10.0),
    }
    state["game_validation"] = validation
    args.state.write_text(json.dumps(state, indent=2, sort_keys=True) + "\n")
    print(json.dumps(validation, sort_keys=True))
    return 0 if validation["elo"] > 0 else 2


if __name__ == "__main__":
    raise SystemExit(main())
