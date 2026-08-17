#!/usr/bin/env python3

import argparse
import json
import math
import os
import random
import re
import resource
import subprocess
from dataclasses import dataclass
from pathlib import Path


SCORE_RE = re.compile(
    r"^Score of Candidate vs Baseline: (\d+) - (\d+) - (\d+).+ (\d+)$",
    re.MULTILINE,
)
TECHNICAL_RE = re.compile(
    r"illegal move|loses on time|disconnects|\bno result\b|crash|timeout|"
    r"unexpected move",
    re.IGNORECASE,
)


@dataclass(frozen=True)
class Parameter:
    default: int
    minimum: int
    maximum: int
    step: int


PARAMETERS = {
    "RFPDepth": Parameter(5, 1, 8, 1),
    "RFPThreshold": Parameter(100, 25, 200, 25),
    "NMPReduction": Parameter(3, 2, 5, 1),
    "IIRBaseDepth": Parameter(2, 1, 8, 1),
    "IIRCutPenalty": Parameter(1, 0, 4, 1),
    "IIRReduction": Parameter(1, 1, 3, 1),
    "LMRDepth": Parameter(3, 2, 8, 1),
    "LMRInCheckPenalty": Parameter(1, 0, 3, 1),
    "LMRGivesCheckPenalty": Parameter(2, 0, 3, 1),
}


def probabilities(bayes_elo: float, draw_elo: float):
    win = 1.0 / (1.0 + 10.0 ** ((draw_elo - bayes_elo) / 400.0))
    loss = 1.0 / (1.0 + 10.0 ** ((draw_elo + bayes_elo) / 400.0))
    return win, loss, 1.0 - win - loss


def cutechess_llr(wins: int, losses: int, draws: int,
                  elo0: float, elo1: float) -> float:
    win_count = wins + 0.5
    loss_count = losses + 0.5
    draw_count = draws + 0.5
    count = win_count + loss_count + draw_count
    p_win = win_count / count
    p_loss = loss_count / count
    bayes_elo = 200.0 * math.log10(
        p_win / p_loss * (1.0 - p_loss) / (1.0 - p_win)
    )
    draw_elo = 200.0 * math.log10(
        (1.0 - p_loss) / p_loss * (1.0 - p_win) / p_win
    )
    x = 10.0 ** (-draw_elo / 400.0)
    scale = 4.0 * x / ((1.0 + x) ** 2)
    p0 = probabilities(elo0 / scale, draw_elo)
    p1 = probabilities(elo1 / scale, draw_elo)
    return sum(
        samples * math.log(prob1 / prob0)
        for samples, prob0, prob1 in zip(
            (win_count, loss_count, draw_count), p0, p1
        )
    )


def option_arguments(values: dict[str, int]) -> list[str]:
    return [f"option.{name}={value}" for name, value in values.items()]


def launch_match(root: Path, engine: Path, candidate: dict[str, int],
                 baseline: dict[str, int], games: int, concurrency: int,
                 seed: int, label: str, timemargin: int):
    log = root / "logs" / f"{label}.log"
    pgn = root / "pgn" / f"{label}.pgn"
    command = [
        str(root / "cutechess-cli"),
        "-engine", "name=Candidate", f"cmd={engine}", "proto=uci",
        "option.Threads=1", *option_arguments(candidate), "restart=on",
        f"timemargin={timemargin}",
        "-engine", "name=Baseline", f"cmd={engine}", "proto=uci",
        "option.Threads=1", *option_arguments(baseline), "restart=on",
        f"timemargin={timemargin}",
        "-each", "tc=inf/8+0.08", "book=gm2001.bin", "bookdepth=10",
        "-games", "2", "-rounds", str(games // 2), "-repeat", "2",
        "-maxmoves", "200", "-concurrency", str(concurrency),
        "-srand", str(seed), "-ratinginterval", str(games),
        "-pgnout", str(pgn),
    ]
    output = log.open("w")
    environment = os.environ.copy()
    environment["LD_LIBRARY_PATH"] = str(root / "lib")

    def configure_limits() -> None:
        resource.setrlimit(resource.RLIMIT_NOFILE, (65536, 65536))

    process = subprocess.Popen(
        command, cwd=root, stdout=output, stderr=subprocess.STDOUT,
        env=environment, preexec_fn=configure_limits,
    )
    return process, output, log


def collect_match(process, output, log: Path, expected_games: int):
    return_code = process.wait()
    output.close()
    if return_code != 0:
        raise RuntimeError(f"cutechess exited with {return_code}: {log}")
    text = log.read_text(errors="replace")
    technical = TECHNICAL_RE.search(text)
    if technical:
        line = text[:technical.end()].splitlines()[-1]
        raise RuntimeError(f"technical result in {log.name}: {line}")
    matches = list(SCORE_RE.finditer(text))
    if not matches or "Finished match" not in text:
        raise RuntimeError(f"incomplete match: {log}")
    wins, losses, draws, games = map(int, matches[-1].groups())
    if games != expected_games or wins + losses + draws != expected_games:
        raise RuntimeError(f"wrong game count in {log}: {games}")
    return wins, losses, draws


def score(result) -> float:
    wins, losses, draws = result
    return (wins + draws / 2.0) / (wins + losses + draws)


def elo(result) -> float:
    value = min(1 - 1e-9, max(1e-9, score(result)))
    return 400.0 * math.log10(value / (1.0 - value))


def write_state(path: Path, state: dict) -> None:
    temporary = path.with_suffix(".tmp")
    temporary.write_text(json.dumps(state, indent=2, sort_keys=True) + "\n")
    temporary.replace(path)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, required=True)
    parser.add_argument("--engine", type=Path, default=Path("./tune-engine"))
    parser.add_argument("--passes", type=int, default=2)
    parser.add_argument("--screen-games", type=int, default=64)
    parser.add_argument("--final-games", type=int, default=256)
    parser.add_argument("--screen-concurrency", type=int, default=64)
    parser.add_argument("--final-concurrency", type=int, default=144)
    parser.add_argument("--seed", type=int, default=51000)
    parser.add_argument("--final-only", action="store_true")
    parser.add_argument("--parameter", action="append", default=[])
    parser.add_argument("--timemargin", type=int, default=250)
    parser.add_argument("--final-label", default="final-tuned-v-default")
    args = parser.parse_args()

    root = args.root.resolve()
    engine = args.engine
    if not engine.is_absolute():
        engine = (root / engine).resolve()
    (root / "logs").mkdir(parents=True, exist_ok=True)
    (root / "pgn").mkdir(parents=True, exist_ok=True)
    state_path = root / "tuning-result.json"
    rng = random.Random(args.seed)
    if args.final_only:
        saved_state = json.loads(state_path.read_text())
        current = saved_state["parameters"]
        history = saved_state["history"]
    else:
        current = {
            name: parameter.default for name, parameter in PARAMETERS.items()
        }
        history = []
    for assignment in args.parameter:
        name, raw_value = assignment.split("=", 1)
        if name not in PARAMETERS:
            raise ValueError(f"unknown parameter: {name}")
        value = int(raw_value)
        parameter = PARAMETERS[name]
        if not parameter.minimum <= value <= parameter.maximum:
            raise ValueError(f"out-of-range value: {assignment}")
        current[name] = value
    match_index = 0

    for pass_index in range(0 if args.final_only else args.passes):
        names = list(PARAMETERS)
        rng.shuffle(names)
        for name in names:
            parameter = PARAMETERS[name]
            candidate_values = sorted({
                max(parameter.minimum, current[name] - parameter.step),
                min(parameter.maximum, current[name] + parameter.step),
            } - {current[name]})
            launched = []
            for value in candidate_values:
                candidate = dict(current)
                candidate[name] = value
                label = f"p{pass_index}-{match_index}-{name}-{value}"
                launched.append((
                    value,
                    *launch_match(
                        root, engine, candidate, current, args.screen_games,
                        args.screen_concurrency, args.seed + match_index, label,
                        args.timemargin,
                    ),
                ))
                match_index += 1

            results = []
            for value, process, output, log in launched:
                result = collect_match(
                    process, output, log, args.screen_games
                )
                results.append((score(result), value, result))
            results.sort(reverse=True)
            accepted = bool(results and results[0][0] > 0.515625)
            if accepted:
                current[name] = results[0][1]
            history.append({
                "pass": pass_index,
                "parameter": name,
                "accepted": accepted,
                "results": [
                    {"value": value, "wld": result,
                     "score": candidate_score}
                    for candidate_score, value, result in results
                ],
                "current": dict(current),
            })
            write_state(state_path, {
                "status": "tuning",
                "parameters": current,
                "history": history,
            })
            print(
                f"pass={pass_index} parameter={name} accepted={accepted} "
                f"current={current[name]} results={results}",
                flush=True,
            )

    defaults = {name: parameter.default for name, parameter in PARAMETERS.items()}
    process, output, log = launch_match(
        root, engine, current, defaults, args.final_games,
        args.final_concurrency, args.seed + 10000,
        args.final_label,
        args.timemargin,
    )
    final_result = collect_match(process, output, log, args.final_games)
    final_elo = elo(final_result)
    final_llr = cutechess_llr(*final_result, 0.0, 10.0)
    upper = math.log(0.95 / 0.05)
    passed = final_elo > 0 and final_llr >= upper
    state = {
        "status": "pass" if passed else "fail",
        "parameters": current,
        "defaults": defaults,
        "history": history,
        "final": {
            "games": args.final_games,
            "wld": final_result,
            "elo": final_elo,
            "llr": final_llr,
            "upper": upper,
        },
    }
    write_state(state_path, state)
    print(json.dumps(state["final"], sort_keys=True), flush=True)
    print(json.dumps(current, sort_keys=True), flush=True)
    return 0 if passed else 2


if __name__ == "__main__":
    raise SystemExit(main())
