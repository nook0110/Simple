#!/usr/bin/env python3

import argparse
import json
import math
import os
import re
import resource
import subprocess
from pathlib import Path

from search_tuner import (Parameter, SCORE_RE, collect_match, cutechess_llr,
                          elo, launch_match, score)


SPRT_DECISION_RE = re.compile(r"SPRT: llr .+ - (H[01]) was accepted")
HARD_TECHNICAL_RE = re.compile(
    r"illegal move|loses on time|disconnects|crash|timeout|unexpected move",
    re.IGNORECASE,
)
TUNE_ACCEPTANCE_SCORE = 0.55

PARAMETERS = {
    "KingShieldNearMG": Parameter(12, 0, 30, 4),
    "KingShieldFarMG": Parameter(10, 0, 20, 3),
    "KingSemiOpenFileMG": Parameter(-14, -30, 0, 4),
    "KingOpenFileMG": Parameter(-14, -30, 0, 4),
    "KingPawnStormNearMG": Parameter(-12, -30, 0, 4),
    "KingPawnStormFarMG": Parameter(-8, -20, 0, 3),
    "KnightKingAttackMG": Parameter(8, 0, 20, 3),
    "BishopKingAttackMG": Parameter(10, 0, 20, 3),
    "RookKingAttackMG": Parameter(18, 0, 30, 4),
    "QueenKingAttackMG": Parameter(20, 0, 40, 5),
}
BASELINE_PARAMETERS = {name: 0 for name in PARAMETERS}


def write_state(path: Path, state: dict) -> None:
    temporary = path.with_suffix(".tmp")
    temporary.write_text(json.dumps(state, indent=2, sort_keys=True) + "\n")
    temporary.replace(path)


def tune(root: Path, engine: Path, passes: int, games: int,
         concurrency: int, seed: int, timemargin: int) -> dict[str, int]:
    current = {name: parameter.default
               for name, parameter in PARAMETERS.items()}
    history = []
    match_index = 0
    parameter_index = 0

    for pass_index in range(passes):
        for name in PARAMETERS:
            comparison_seed = seed + parameter_index
            parameter_index += 1
            parameter = PARAMETERS[name]
            values = sorted({
                max(parameter.minimum, current[name] - parameter.step),
                min(parameter.maximum, current[name] + parameter.step),
            } - {current[name]})
            match_concurrency = max(1, concurrency // len(values))
            launched = []
            for value in values:
                candidate = dict(current)
                candidate[name] = value
                label = f"king-tune-p{pass_index}-{match_index}-{name}-{value}"
                launched.append((
                    value,
                    *launch_match(
                        root, engine, candidate, current, games,
                        match_concurrency, comparison_seed, label, timemargin,
                    ),
                ))
                match_index += 1

            results = []
            for value, process, output, log in launched:
                result = collect_match(process, output, log, games)
                results.append((score(result), value, result))
            results.sort(reverse=True)
            accepted = bool(
                results and results[0][0] >= TUNE_ACCEPTANCE_SCORE
            )
            if accepted:
                current[name] = results[0][1]
            record = {
                "pass": pass_index,
                "parameter": name,
                "seed": comparison_seed,
                "accepted": accepted,
                "results": [
                    {"value": value, "wld": result,
                     "score": candidate_score}
                    for candidate_score, value, result in results
                ],
                "current": dict(current),
            }
            history.append(record)
            write_state(root / "king-safety-tuning-result.json", {
                "status": "tuning",
                "parameters": current,
                "history": history,
            })
            print(json.dumps(record, sort_keys=True), flush=True)

    write_state(root / "king-safety-tuning-result.json", {
        "status": "tuned",
        "parameters": current,
        "history": history,
    })
    return current


def launch_sprt(root: Path, engine: Path, candidate: dict[str, int],
                concurrency: int, seed: int, timemargin: int,
                max_games: int) -> tuple[subprocess.Popen, object, Path]:
    label = f"sprt-king-safety-v-zero-tm{timemargin}"
    log = root / "logs" / f"{label}.log"
    pgn = root / "pgn" / f"{label}.pgn"

    def options(values: dict[str, int]) -> list[str]:
        return [f"option.{name}={value}" for name, value in values.items()]

    command = [
        str(root / "cutechess-cli"),
        "-engine", "name=Candidate", f"cmd={engine}", "proto=uci",
        "option.Threads=1", *options(candidate), "restart=on",
        f"timemargin={timemargin}",
        "-engine", "name=Baseline", f"cmd={engine}", "proto=uci",
        "option.Threads=1", *options(BASELINE_PARAMETERS), "restart=on",
        f"timemargin={timemargin}",
        "-each", "tc=inf/8+0.08", "book=gm2001.bin", "bookdepth=10",
        "-games", "2", "-rounds", str(max_games // 2), "-repeat", "2",
        "-maxmoves", "200", "-concurrency", str(concurrency),
        "-srand", str(seed), "-ratinginterval", "100",
        "-sprt", "elo0=0", "elo1=10", "alpha=0.05", "beta=0.05",
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


def collect_sprt(process: subprocess.Popen, output: object,
                 log: Path) -> dict:
    return_code = process.wait()
    output.close()
    if return_code != 0:
        raise RuntimeError(f"cutechess exited with {return_code}: {log}")
    text = log.read_text(errors="replace")
    technical = HARD_TECHNICAL_RE.search(text)
    if technical:
        line = text[:technical.end()].splitlines()[-1]
        raise RuntimeError(f"technical result in {log.name}: {line}")
    matches = list(SCORE_RE.finditer(text))
    if not matches or "Finished match" not in text:
        raise RuntimeError(f"incomplete SPRT match: {log}")
    decision = SPRT_DECISION_RE.search(text)
    first_no_result = text.lower().find("no result")
    if first_no_result >= 0:
        final_score = matches[-1].groups()
        scores_after_cancellation = [
            match.groups() for match in matches
            if match.start() > first_no_result
        ]
        if (decision is None or
                any(value != final_score
                    for value in scores_after_cancellation)):
            raise RuntimeError(f"non-SPRT no-result game in {log.name}")
    wins, losses, draws, games = map(int, matches[-1].groups())
    result = (wins, losses, draws)
    llr = cutechess_llr(*result, 0.0, 10.0)
    bound = math.log(0.95 / 0.05)
    return {
        "games": games,
        "wld": result,
        "elo": elo(result),
        "llr": llr,
        "lower": -bound,
        "upper": bound,
        "passed": (decision.group(1) == "H1" if decision is not None
                   else llr >= bound),
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, required=True)
    parser.add_argument("--engine", type=Path,
                        default=Path("./king-safety-engine"))
    parser.add_argument("--passes", type=int, default=1)
    parser.add_argument("--tune-games", type=int, default=64)
    parser.add_argument("--tune-concurrency", type=int, default=64)
    parser.add_argument("--sprt-concurrency", type=int, default=64)
    parser.add_argument("--max-sprt-games", type=int, default=10000)
    parser.add_argument("--sprt-only", action="store_true")
    parser.add_argument("--tune-only", action="store_true")
    parser.add_argument("--seed", type=int, default=87000)
    parser.add_argument("--timemargin", type=int, default=5000)
    args = parser.parse_args()

    root = args.root.resolve()
    engine = args.engine
    if not engine.is_absolute():
        engine = (root / engine).resolve()
    (root / "logs").mkdir(parents=True, exist_ok=True)
    (root / "pgn").mkdir(parents=True, exist_ok=True)

    state_path = root / "king-safety-tuning-result.json"
    if args.sprt_only:
        tuned = json.loads(state_path.read_text())["parameters"]
    else:
        tuned = tune(
            root, engine, args.passes, args.tune_games,
            args.tune_concurrency, args.seed, args.timemargin,
        )
    if args.tune_only:
        return 0
    process, output, log = launch_sprt(
        root, engine, tuned, args.sprt_concurrency, args.seed + 10000,
        args.timemargin, args.max_sprt_games,
    )
    result = collect_sprt(process, output, log)
    state = json.loads(state_path.read_text())
    state["status"] = "pass" if result["passed"] else "fail"
    state["sprt"] = result
    write_state(state_path, state)
    print(json.dumps({"parameters": tuned, "sprt": result},
                     sort_keys=True), flush=True)
    return 0 if result["passed"] else 2


if __name__ == "__main__":
    raise SystemExit(main())
