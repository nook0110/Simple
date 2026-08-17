#!/usr/bin/env python3

import argparse
import math
import re
from pathlib import Path

from search_tuner import TECHNICAL_RE, cutechess_llr, elo


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
    scores = list(SCORE_RE.finditer(text))
    if not scores or "Finished match" not in text:
        raise RuntimeError(f"incomplete match: {path}")
    wins, losses, draws, games = map(int, scores[-1].groups())
    if wins + losses + draws != games:
        raise RuntimeError(f"invalid score in {path}: {scores[-1].groups()}")
    return wins, losses, draws


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("logs", type=Path, nargs="+")
    parser.add_argument("--minimum-games", type=int, default=256)
    args = parser.parse_args()

    results = [parse_log(path) for path in args.logs]
    aggregate = tuple(sum(result[index] for result in results)
                      for index in range(3))
    games = sum(aggregate)
    aggregate_elo = elo(aggregate)
    llr = cutechess_llr(*aggregate, 0.0, 10.0)
    upper = math.log(0.95 / 0.05)
    passed = games >= args.minimum_games and aggregate_elo > 0 and llr >= upper
    print(
        f"{'PASS' if passed else 'FAIL'} games={games} wld={aggregate} "
        f"elo={aggregate_elo:.3f} llr={llr:.3f} upper={upper:.3f}"
    )
    return 0 if passed else 2


if __name__ == "__main__":
    raise SystemExit(main())
