#!/usr/bin/env python3

import argparse
import json
import math
import subprocess
import sys
from pathlib import Path

import numpy as np

from pawn_tuner import PARAMETERS
from search_tuner import collect_match, cutechess_llr, elo, launch_match, score
from texel_optimize import mse


def write_state(path: Path, state: dict) -> None:
    temporary = path.with_suffix(".tmp")
    temporary.write_text(json.dumps(state, indent=2, sort_keys=True) + "\n")
    temporary.replace(path)


def run_match(root: Path, engine: Path, candidate: dict[str, int],
              baseline: dict[str, int], games: int, concurrency: int,
              seed: int, label: str, timemargin: int):
    process, output, log = launch_match(
        root, engine, candidate, baseline, games, concurrency, seed, label,
        timemargin,
    )
    return collect_match(process, output, log, games)


def run_batch(root: Path, engine: Path, candidates, baseline, games,
              concurrency, seed, prefix, timemargin, batch_size=3):
    results = {}
    for start in range(0, len(candidates), batch_size):
        launched = []
        for offset, (name, values) in enumerate(
                candidates[start:start + batch_size]):
            label = f"{prefix}-{name}"
            launched.append((
                name,
                *launch_match(
                    root, engine, values, baseline, games, concurrency,
                    seed + start + offset, label, timemargin,
                ),
            ))
        for name, process, output, log in launched:
            result = collect_match(process, output, log, games)
            results[name] = result
            print(json.dumps({
                "stage": prefix,
                "parameter": name,
                "wld": result,
                "elo": elo(result),
            }, sort_keys=True), flush=True)
    return results


def add_results(*results):
    return tuple(sum(result[index] for result in results)
                 for index in range(3))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, required=True)
    parser.add_argument("--engine", type=Path, default=Path("autotune-engine"))
    parser.add_argument("--features", type=Path, required=True)
    parser.add_argument("--dataset-summary", type=Path, required=True)
    parser.add_argument("--l2-values", default="0,1e-7,3e-7,1e-6,3e-6,1e-5,3e-5")
    parser.add_argument("--max-ablation", type=int, default=12)
    parser.add_argument("--screen-games", type=int, default=128)
    parser.add_argument("--confirm-games", type=int, default=384)
    parser.add_argument("--interaction-games", type=int, default=256)
    parser.add_argument("--final-batch-games", type=int, default=512)
    parser.add_argument("--minimum-final-games", type=int, default=1024)
    parser.add_argument("--maximum-final-games", type=int, default=4096)
    parser.add_argument("--concurrency", type=int, default=48)
    parser.add_argument("--final-concurrency", type=int, default=72)
    parser.add_argument("--seed", type=int, default=300000)
    parser.add_argument("--timemargin", type=int, default=1000)
    args = parser.parse_args()

    root = args.root.resolve()
    engine = args.engine
    if not engine.is_absolute():
        engine = (root / engine).resolve()
    features = args.features
    if not features.is_absolute():
        features = (root / features).resolve()
    summary_path = args.dataset_summary
    if not summary_path.is_absolute():
        summary_path = (root / summary_path).resolve()
    state_path = root / "texel-autotune-state.json"
    defaults = {name: parameter.default for name, parameter in PARAMETERS.items()}
    state = {
        "status": "optimizing",
        "dataset": json.loads(summary_path.read_text()),
        "parameter_count": len(PARAMETERS),
    }
    write_state(state_path, state)

    models = []
    for l2_text in args.l2_values.split(","):
        output = root / f"autotune-l2-{l2_text}.json"
        command = [
            sys.executable, str(root / "texel_optimize.py"),
            "--features", str(features), "--output", str(output),
            "--strides", "16,8,4,2,1", "--passes-per-stride", "20",
            "--l2", l2_text,
        ]
        subprocess.run(command, cwd=root, stdout=subprocess.DEVNULL,
                       check=False)
        model = json.loads(output.read_text())
        models.append((model["tuned_mse"]["validation"], output, model))
    models.sort(key=lambda item: item[0])
    _, model_path, model = models[0]
    if model["holdout_improvement"] <= 0:
        state.update({"status": "holdout_failed", "model": model})
        write_state(state_path, state)
        return 2
    state.update({"status": "ablating", "model_path": str(model_path),
                  "model": model})
    write_state(state_path, state)

    data = np.load(features)
    names = list(data["names"])
    baseline_scores = data["baseline"].astype(float)
    feature_matrix = data["features"].astype(float)
    results = data["results"].astype(float)
    validation = data["splits"] == 1
    k = model["k"]
    baseline_validation = mse(
        baseline_scores[validation], results[validation], k
    )
    ranked = []
    for index, name in enumerate(names):
        offset = model["step_offsets"][name]
        if not offset:
            continue
        candidate_mse = mse(
            baseline_scores[validation]
            + feature_matrix[validation, index] * offset,
            results[validation], k,
        )
        improvement = baseline_validation - candidate_mse
        if improvement > 0:
            ranked.append((improvement, name))
    ranked.sort(reverse=True)
    ranked = ranked[:args.max_ablation]
    screen_candidates = []
    for _, name in ranked:
        values = dict(defaults)
        values[name] = model["parameters"][name]
        screen_candidates.append((name, values))
    screen = run_batch(
        root, engine, screen_candidates, defaults, args.screen_games,
        args.concurrency, args.seed, "autotune-screen", args.timemargin,
    )
    positive = [
        (name, values) for name, values in screen_candidates
        if elo(screen[name]) > 0
    ]
    confirm = run_batch(
        root, engine, positive, defaults, args.confirm_games,
        args.concurrency, args.seed + 1000, "autotune-confirm",
        args.timemargin,
    )
    accepted = []
    evidence = {}
    for name, values in positive:
        aggregate = add_results(screen[name], confirm[name])
        evidence[name] = {
            "wld": aggregate,
            "games": sum(aggregate),
            "elo": elo(aggregate),
        }
        if evidence[name]["elo"] > 0:
            accepted.append((evidence[name]["elo"], name, values))
    accepted.sort(reverse=True)
    if not accepted:
        state.update({"status": "ablation_failed", "ablation": evidence})
        write_state(state_path, state)
        return 2

    current = dict(defaults)
    selected = []
    for _, name, values in accepted:
        candidate = dict(current)
        candidate[name] = values[name]
        result = run_match(
            root, engine, candidate, current, args.interaction_games,
            args.final_concurrency, args.seed + 2000 + len(selected),
            f"autotune-interaction-{name}", args.timemargin,
        )
        if elo(result) > 0:
            current = candidate
            selected.append(name)
    if not selected:
        selected = [accepted[0][1]]
        current = dict(accepted[0][2])

    aggregate = (0, 0, 0)
    final_batches = []
    while sum(aggregate) < args.maximum_final_games:
        batch = run_match(
            root, engine, current, defaults, args.final_batch_games,
            args.final_concurrency, args.seed + 3000 + len(final_batches),
            f"autotune-final-{len(final_batches)}", args.timemargin,
        )
        aggregate = add_results(aggregate, batch)
        llr = cutechess_llr(*aggregate, 0.0, 10.0)
        final_batches.append(batch)
        print(json.dumps({
            "stage": "final",
            "games": sum(aggregate),
            "wld": aggregate,
            "elo": elo(aggregate),
            "llr": llr,
        }, sort_keys=True), flush=True)
        if sum(aggregate) >= args.minimum_final_games and abs(llr) >= math.log(19):
            break

    llr = cutechess_llr(*aggregate, 0.0, 10.0)
    passed = llr >= math.log(19)
    state.update({
        "status": "pass" if passed else "fail",
        "ablation": evidence,
        "selected": selected,
        "parameters": current,
        "final": {
            "games": sum(aggregate),
            "wld": aggregate,
            "elo": elo(aggregate),
            "llr": llr,
            "batches": final_batches,
        },
    })
    write_state(state_path, state)
    return 0 if passed else 2


if __name__ == "__main__":
    raise SystemExit(main())
