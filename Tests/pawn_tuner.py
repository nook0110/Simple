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
    "DoubledPawnMG": Parameter(-15, -30, 0, 5),
    "DoubledPawnEG": Parameter(-15, -40, 0, 5),
    "IsolatedPawnMG": Parameter(-10, -30, 0, 5),
    "IsolatedPawnEG": Parameter(-10, -40, 0, 5),
    "PassedPawn4MG": Parameter(5, 0, 40, 5),
    "PassedPawn4EG": Parameter(18, 0, 60, 10),
    "PassedPawn5MG": Parameter(30, 0, 70, 10),
    "PassedPawn5EG": Parameter(40, 0, 100, 15),
    "PassedPawn6MG": Parameter(25, 0, 100, 15),
    "PassedPawn6EG": Parameter(70, 0, 150, 20),
    "PassedPawn7MG": Parameter(90, 0, 160, 20),
    "PassedPawn7EG": Parameter(120, 0, 200, 30),
}
STRUCTURE_PARAMETERS = list(PARAMETERS)

# Conservative manual corrections for the visibly anomalous sixth- and
# seventh-rank pawn PSQT entries.  They leave a small, smooth advancement
# score for non-passed pawns instead of either preserving the original
# 50--187 cp endgame values or zeroing the ranks completely.  Passed-pawn
# bonuses remain independent parameters above.
ADVANCED_PAWN_PSQT = {
    6: {
        "A": (6, -59), "B": (-2, -60), "C": (-11, -50),
        "D": (-11, -37), "E": (-40, -26), "F": (-31, -23),
        "G": (-10, -47), "H": (20, -49),
    },
    7: {
        "A": (-83, -128), "B": (-109, -118), "C": (-41, -108),
        "D": (-70, -89), "E": (-48, -102), "F": (-101, -87),
        "G": (-19, -115), "H": (21, -132),
    },
}
for rank, files in ADVANCED_PAWN_PSQT.items():
    for file_name, (middlegame, endgame) in files.items():
        PARAMETERS[f"PawnPSQT{file_name}{rank}MG"] = Parameter(
            middlegame, -250, 250, 15
        )
        PARAMETERS[f"PawnPSQT{file_name}{rank}EG"] = Parameter(
            endgame, -250, 250, 25
        )

PAWN_PARAMETERS = list(PARAMETERS)

BASELINE_MATERIAL_VALUES = {
    "PawnValueMG": 82,
    "PawnValueEG": 94,
    "KnightValueMG": 337,
    "KnightValueEG": 281,
    "BishopValueMG": 365,
    "BishopValueEG": 297,
    "RookValueMG": 477,
    "RookValueEG": 512,
    "QueenValueMG": 1025,
    "QueenValueEG": 936,
}

MATERIAL_PARAMETERS = {
    "PawnValueMG": Parameter(82, 40, 160, 5),
    "PawnValueEG": Parameter(94, 40, 160, 5),
    "KnightValueMG": Parameter(337, 200, 500, 10),
    "KnightValueEG": Parameter(275, 200, 500, 10),
    "BishopValueMG": Parameter(375, 200, 500, 10),
    "BishopValueEG": Parameter(297, 200, 500, 10),
    "RookValueMG": Parameter(477, 350, 700, 15),
    "RookValueEG": Parameter(512, 350, 700, 15),
    "QueenValueMG": Parameter(1025, 700, 1300, 25),
    "QueenValueEG": Parameter(936, 700, 1300, 25),
}
PARAMETERS.update(MATERIAL_PARAMETERS)

MOBILITY_PARAMETERS = {
    "KnightMobilityMG": Parameter(4, 0, 12, 1),
    "KnightMobilityEG": Parameter(2, 0, 12, 1),
    "BishopMobilityMG": Parameter(7, 0, 12, 1),
    "BishopMobilityEG": Parameter(3, 0, 12, 1),
    "RookMobilityMG": Parameter(2, 0, 10, 1),
    "RookMobilityEG": Parameter(4, 0, 10, 1),
    "QueenMobilityMG": Parameter(2, 0, 8, 1),
    "QueenMobilityEG": Parameter(2, 0, 8, 1),
}
PARAMETERS.update(MOBILITY_PARAMETERS)

PSQT_ADJUSTMENT_PARAMETERS = {}
for piece_name in ("Knight", "Bishop", "Rook", "Queen", "King"):
    for rank in range(1, 9):
        for file_index in range(4):
            square = f"{chr(ord('A') + file_index)}{rank}"
            PSQT_ADJUSTMENT_PARAMETERS[f"{piece_name}PSQT{square}MG"] = (
                Parameter(0, -100, 100, 1)
            )
            PSQT_ADJUSTMENT_PARAMETERS[f"{piece_name}PSQT{square}EG"] = (
                Parameter(0, -100, 100, 1)
            )
PARAMETERS.update(PSQT_ADJUSTMENT_PARAMETERS)

BASELINE_PARAMETERS = {
    name: (BASELINE_MATERIAL_VALUES[name] if name in MATERIAL_PARAMETERS else 0)
    for name, parameter in PARAMETERS.items()
}


def write_state(path: Path, state: dict) -> None:
    temporary = path.with_suffix(".tmp")
    temporary.write_text(json.dumps(state, indent=2, sort_keys=True) + "\n")
    temporary.replace(path)


def tune(root: Path, engine: Path, passes: int, games: int,
         concurrency: int, seed: int, timemargin: int,
         initial: dict[str, int] | None = None,
         selected: list[str] | None = None,
         existing_history: list | None = None) -> dict[str, int]:
    current = {name: parameter.default
               for name, parameter in PARAMETERS.items()}
    if initial is not None:
        current.update(initial)
    history = list(existing_history or [])
    match_index = 0
    parameter_index = 0

    for pass_index in range(passes):
        names = (selected if selected is not None else sorted(
            PARAMETERS,
            key=lambda name: (
                not name.startswith("PawnPSQT"),
                -abs(PARAMETERS[name].default),
            ),
        ))
        for name in names:
            comparison_seed = seed + parameter_index
            parameter_index += 1
            parameter = PARAMETERS[name]
            values = sorted({
                max(parameter.minimum, current[name] - parameter.step),
                min(parameter.maximum, current[name] + parameter.step),
            } - {current[name]})
            launched = []
            for value in values:
                candidate = dict(current)
                candidate[name] = value
                label = f"tune-p{pass_index}-{match_index}-{name}-{value}"
                launched.append((
                    value,
                    *launch_match(
                        root, engine, candidate, current, games,
                        concurrency, comparison_seed, label, timemargin,
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
            write_state(root / "pawn-tuning-result.json", {
                "status": "tuning",
                "parameters": current,
                "history": history,
            })
            print(json.dumps(record, sort_keys=True), flush=True)

    write_state(root / "pawn-tuning-result.json", {
        "status": "tuned",
        "parameters": current,
        "history": history,
    })
    return current


def launch_sprt(root: Path, engine: Path, candidate: dict[str, int],
                concurrency: int, seed: int, timemargin: int,
                max_games: int) -> tuple[subprocess.Popen, object, Path]:
    label = f"sprt-pawns-v-zero-tm{timemargin}"
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
            raise RuntimeError(
                f"non-SPRT no-result game in {log.name}"
            )
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
    parser.add_argument("--engine", type=Path, default=Path("./pawn-engine"))
    parser.add_argument("--passes", type=int, default=1)
    parser.add_argument("--tune-games", type=int, default=64)
    parser.add_argument("--tune-concurrency", type=int, default=64)
    parser.add_argument("--sprt-concurrency", type=int, default=64)
    parser.add_argument("--max-sprt-games", type=int, default=10000)
    parser.add_argument("--sprt-only", action="store_true")
    parser.add_argument("--structure-retune", action="store_true")
    parser.add_argument("--material-retune", action="store_true")
    parser.add_argument("--seed-pawn-defaults", action="store_true")
    parser.add_argument("--tune-only", action="store_true")
    parser.add_argument("--seed", type=int, default=65000)
    parser.add_argument("--timemargin", type=int, default=1000)
    args = parser.parse_args()

    root = args.root.resolve()
    engine = args.engine
    if not engine.is_absolute():
        engine = (root / engine).resolve()
    (root / "logs").mkdir(parents=True, exist_ok=True)
    (root / "pgn").mkdir(parents=True, exist_ok=True)

    state_path = root / "pawn-tuning-result.json"
    if args.sprt_only:
        tuned = json.loads(state_path.read_text())["parameters"]
    elif args.structure_retune:
        saved = json.loads(state_path.read_text())
        initial = dict(saved["parameters"])
        if args.seed_pawn_defaults:
            initial.update({
                name: PARAMETERS[name].default for name in PAWN_PARAMETERS
            })
        tuned = tune(
            root, engine, args.passes, args.tune_games,
            args.tune_concurrency, args.seed, args.timemargin,
            initial=initial, selected=STRUCTURE_PARAMETERS,
            existing_history=saved.get("history", []),
        )
    elif args.material_retune:
        saved = (
            json.loads(state_path.read_text())
            if state_path.exists()
            else {"parameters": BASELINE_PARAMETERS, "history": []}
        )
        tuned = tune(
            root, engine, args.passes, args.tune_games,
            args.tune_concurrency, args.seed, args.timemargin,
            initial=saved["parameters"], selected=list(MATERIAL_PARAMETERS),
            existing_history=saved.get("history", []),
        )
    else:
        tuned = tune(root, engine, args.passes, args.tune_games,
                     args.tune_concurrency, args.seed, args.timemargin,
                     selected=PAWN_PARAMETERS)
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
