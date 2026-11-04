#!/usr/bin/env python3

import argparse
import json
from pathlib import Path

from pawn_tuner import PARAMETERS
from search_tuner import collect_match, elo, launch_match


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, required=True)
    parser.add_argument("--engine", type=Path, default=Path("texel-engine"))
    parser.add_argument("--state", type=Path, required=True)
    parser.add_argument("--games", type=int, default=128)
    parser.add_argument("--concurrency", type=int, default=48)
    parser.add_argument("--batch-size", type=int, default=3)
    parser.add_argument("--seed", type=int, default=146000)
    parser.add_argument("--timemargin", type=int, default=1000)
    parser.add_argument("--names", default="")
    args = parser.parse_args()

    root = args.root.resolve()
    engine = args.engine
    if not engine.is_absolute():
        engine = (root / engine).resolve()
    state = json.loads(args.state.read_text())
    baseline = {name: parameter.default for name, parameter in PARAMETERS.items()}
    changed = [name for name, offset in state["step_offsets"].items() if offset]
    if args.names:
        requested = args.names.split(",")
        unknown = set(requested).difference(changed)
        if unknown:
            raise ValueError(f"parameters are not changed in state: {unknown}")
        changed = requested
    results = {}

    for start in range(0, len(changed), args.batch_size):
        launched = []
        for index, name in enumerate(changed[start:start + args.batch_size]):
            candidate = dict(baseline)
            candidate[name] = state["parameters"][name]
            label = f"texel-ablate-{name}"
            launched.append((
                name,
                *launch_match(
                    root, engine, candidate, baseline, args.games,
                    args.concurrency, args.seed + start + index, label,
                    args.timemargin,
                ),
            ))
        for name, process, output, log in launched:
            result = collect_match(process, output, log, args.games)
            results[name] = {"wld": result, "elo": elo(result)}
            print(json.dumps({name: results[name]}, sort_keys=True), flush=True)

    output = root / "texel-ablation.json"
    output.write_text(json.dumps(results, indent=2, sort_keys=True) + "\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
