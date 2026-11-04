#!/usr/bin/env python3

import argparse
import json
import statistics
from pathlib import Path

from evaluation_benchmark import POSITIONS, measure
from king_safety_tuner import BASELINE_PARAMETERS, PARAMETERS


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--engine", type=Path, required=True)
    parser.add_argument("--depth", type=int, default=14)
    parser.add_argument("--repeats", type=int, default=7)
    parser.add_argument("--component", choices=("all", "pawns", "attacks"),
                        default="all")
    args = parser.parse_args()

    candidate = {name: parameter.default
                 for name, parameter in PARAMETERS.items()}
    if args.component == "pawns":
        candidate.update({
            name: 0 for name in candidate if "Attack" in name
        })
    elif args.component == "attacks":
        candidate.update({
            name: 0 for name in candidate if "Attack" not in name
        })
    modes = {
        "baseline": BASELINE_PARAMETERS,
        "candidate": candidate,
    }
    samples = {mode: {} for mode in modes}
    for repeat in range(args.repeats):
        order = tuple(modes) if repeat % 2 == 0 else tuple(reversed(modes))
        for position_index, position in enumerate(POSITIONS):
            for mode in order:
                sample = measure(
                    args.engine.resolve(), position, args.depth, modes[mode]
                )
                samples[mode][(repeat, position_index)] = sample
                print(json.dumps({
                    "mode": mode,
                    "position": position,
                    "repeat": repeat,
                    **sample,
                }, sort_keys=True), flush=True)

    reported_ratios = []
    wall_ratios = []
    for key, sample in samples["candidate"].items():
        baseline = samples["baseline"][key]
        reported_ratios.append(
            sample["reported_nps"] / baseline["reported_nps"]
        )
        wall_ratios.append(sample["wall_nps"] / baseline["wall_nps"])
    print(json.dumps({
        "median_reported_nps_ratio": statistics.median(reported_ratios),
        "median_wall_nps_ratio": statistics.median(wall_ratios),
        "samples": len(reported_ratios),
    }, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
