#!/usr/bin/env python3

import argparse
import json
import math
from pathlib import Path

import numpy as np


LN10_OVER_400 = math.log(10.0) / 400.0


def probabilities(scores: np.ndarray, k: float) -> np.ndarray:
    values = np.clip(k * scores * LN10_OVER_400, -40.0, 40.0)
    return 1.0 / (1.0 + np.exp(-values))


def mse(scores: np.ndarray, results: np.ndarray, k: float) -> float:
    return float(np.mean(np.square(probabilities(scores, k) - results)))


def fit_k(scores: np.ndarray, results: np.ndarray) -> float:
    low, high = 0.05, 4.0
    for _ in range(80):
        left = (2 * low + high) / 3
        right = (low + 2 * high) / 3
        if mse(scores, results, left) <= mse(scores, results, right):
            high = right
        else:
            low = left
    return (low + high) / 2


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--features", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--strides", default="16,8,4,2,1")
    parser.add_argument("--passes-per-stride", type=int, default=20)
    parser.add_argument("--l2", type=float, default=0.0)
    args = parser.parse_args()

    data = np.load(args.features)
    x = data["features"].astype(np.float64)
    baseline = data["baseline"].astype(np.float64)
    results = data["results"].astype(np.float64)
    splits = data["splits"]
    names = data["names"]
    defaults = data["defaults"].astype(np.int64)
    steps = data["steps"].astype(np.int64)
    minimums = data["minimums"].astype(np.int64)
    maximums = data["maximums"].astype(np.int64)
    train = splits == 0
    validation = splits == 1
    holdout = splits == 2
    if min(train.sum(), validation.sum(), holdout.sum()) == 0:
        raise RuntimeError("train, validation, and holdout must all be non-empty")

    k = fit_k(baseline[train], results[train])
    z = np.zeros(x.shape[1], dtype=np.int64)
    lower = ((minimums - defaults) / steps).astype(np.int64)
    upper = ((maximums - defaults) / steps).astype(np.int64)
    train_scores = baseline[train].copy()
    train_results = results[train]
    train_mse = mse(train_scores, train_results, k)
    train_objective = train_mse
    optimization_history = []
    best_z = z.copy()
    best_validation_mse = mse(
        baseline[validation], results[validation], k
    )

    for stride in (int(value) for value in args.strides.split(",")):
        for pass_index in range(args.passes_per_stride):
            improved = 0
            for index in range(x.shape[1]):
                best_delta = 0
                best_mse = train_objective
                for delta in (-stride, stride):
                    candidate = z[index] + delta
                    if candidate < lower[index] or candidate > upper[index]:
                        continue
                    candidate_data_mse = mse(
                        train_scores + x[train, index] * delta,
                        train_results, k,
                    )
                    candidate_z = z.copy()
                    candidate_z[index] = candidate
                    candidate_objective = (
                        candidate_data_mse
                        + args.l2 * float(np.mean(np.square(candidate_z)))
                    )
                    if candidate_objective < best_mse:
                        best_mse = candidate_objective
                        best_delta = delta
                if best_delta:
                    z[index] += best_delta
                    train_scores += x[train, index] * best_delta
                    train_mse = mse(train_scores, train_results, k)
                    train_objective = best_mse
                    improved += 1
                    candidate_validation_mse = mse(
                        baseline[validation] + x[validation] @ z,
                        results[validation], k,
                    )
                    if candidate_validation_mse < best_validation_mse:
                        best_validation_mse = candidate_validation_mse
                        best_z = z.copy()
            validation_mse = mse(
                baseline[validation] + x[validation] @ z,
                results[validation], k,
            )
            optimization_history.append({
                "stride": stride,
                "pass": pass_index,
                "accepted_parameters": improved,
                "train_mse": train_mse,
                "train_objective": train_objective,
                "validation_mse": validation_mse,
            })
            print(json.dumps(optimization_history[-1], sort_keys=True),
                  flush=True)
            if improved == 0:
                break

    z = best_z
    tuned_values = defaults + z * steps
    baseline_mse = {}
    tuned_mse = {}
    for name, mask in (("train", train), ("validation", validation),
                       ("holdout", holdout)):
        baseline_mse[name] = mse(baseline[mask], results[mask], k)
        tuned_mse[name] = mse(
            baseline[mask] + x[mask] @ z,
            results[mask], k,
        )

    state = {
        "k": k,
        "l2": args.l2,
        "positions": {
            "train": int(train.sum()),
            "validation": int(validation.sum()),
            "holdout": int(holdout.sum()),
        },
        "parameters": {
            str(name): int(value) for name, value in zip(names, tuned_values)
        },
        "parameter_count": len(names),
        "step_offsets": {
            str(name): int(value) for name, value in zip(names, z)
        },
        "optimization_history": optimization_history,
        "baseline_mse": baseline_mse,
        "tuned_mse": tuned_mse,
        "holdout_improvement": baseline_mse["holdout"] - tuned_mse["holdout"],
    }
    args.output.write_text(json.dumps(state, indent=2, sort_keys=True) + "\n")
    print(json.dumps(state, sort_keys=True))
    return 0 if state["holdout_improvement"] > 0 else 2


if __name__ == "__main__":
    raise SystemExit(main())
