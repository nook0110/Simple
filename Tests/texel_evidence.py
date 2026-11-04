#!/usr/bin/env python3

import argparse
import json
import re
from pathlib import Path

from search_tuner import TECHNICAL_RE, elo


SCORE_RE = re.compile(
    r"^Score of Candidate vs Baseline: (\d+) - (\d+) - (\d+).+ (\d+)$",
    re.MULTILINE,
)


def parse_log(path: Path) -> tuple[int, int, int]:
    text = path.read_text(errors="replace")
    technical = TECHNICAL_RE.search(text)
    if technical:
        line = text[:technical.end()].splitlines()[-1]
        raise RuntimeError(f"technical result in {path.name}: {line}")
    matches = list(SCORE_RE.finditer(text))
    if not matches or "Finished match" not in text:
        raise RuntimeError(f"incomplete match: {path}")
    wins, losses, draws, games = map(int, matches[-1].groups())
    if wins + losses + draws != games:
        raise RuntimeError(f"invalid score in {path.name}")
    return wins, losses, draws


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--dataset-summary", type=Path, required=True)
    parser.add_argument("--texel-state", type=Path, required=True)
    parser.add_argument("logs", type=Path, nargs="+")
    args = parser.parse_args()

    dataset = json.loads(args.dataset_summary.read_text())
    state = json.loads(args.texel_state.read_text())
    results = [parse_log(path) for path in args.logs]
    aggregate = tuple(sum(result[index] for result in results)
                      for index in range(3))
    games = sum(aggregate)
    aggregate_elo = elo(aggregate)
    passed = (
        dataset["games"] >= 10_000
        and state["parameter_count"] >= 40
        and state["holdout_improvement"] > 0
        and games >= 1024
        and aggregate_elo > 0
    )
    print(json.dumps({
        "status": "PASS" if passed else "FAIL",
        "dataset_games": dataset["games"],
        "parameter_count": state["parameter_count"],
        "holdout_improvement": state["holdout_improvement"],
        "validation_games": games,
        "wld": aggregate,
        "elo": aggregate_elo,
    }, sort_keys=True))
    return 0 if passed else 2


if __name__ == "__main__":
    raise SystemExit(main())
