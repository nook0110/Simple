#!/usr/bin/env python3

import argparse
import queue
import re
import subprocess
import threading


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--engine", required=True)
    args = parser.parse_args()

    process = subprocess.Popen(
        [args.engine],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        bufsize=1,
    )
    assert process.stdin is not None
    assert process.stdout is not None
    output: queue.Queue[str | None] = queue.Queue()
    lines: list[str] = []

    def read_output() -> None:
        for line in process.stdout:
            output.put(line.rstrip("\n"))
        output.put(None)

    reader = threading.Thread(target=read_output, daemon=True)
    reader.start()

    def send(command: str) -> None:
        process.stdin.write(command + "\n")
        process.stdin.flush()

    def wait_for(predicate, description: str, timeout: float = 15.0) -> str:
        while True:
            try:
                line = output.get(timeout=timeout)
            except queue.Empty as error:
                raise RuntimeError(f"timed out waiting for {description}") from error
            if line is None:
                raise RuntimeError(f"engine exited while waiting for {description}")
            lines.append(line)
            if predicate(line):
                return line

    send("uci")
    wait_for(lambda line: line == "uciok", "uciok")
    if "option name Threads type spin default 1 min 1 max 256" not in lines:
        raise RuntimeError("Threads UCI option was not advertised")
    expected_tuning_options = {
        "option name RFPDepth type spin default 5 min 1 max 8",
        "option name RFPThreshold type spin default 100 min 25 max 200",
        "option name NMPReduction type spin default 3 min 2 max 5",
        "option name IIRBaseDepth type spin default 2 min 1 max 8",
        "option name IIRCutPenalty type spin default 1 min 0 max 4",
        "option name IIRReduction type spin default 1 min 1 max 3",
        "option name LMRDepth type spin default 3 min 2 max 8",
        "option name LMRInCheckPenalty type spin default 1 min 0 max 3",
        "option name LMRGivesCheckPenalty type spin default 2 min 0 max 3",
        "option name PawnValueMG type spin default 82 min 40 max 160",
        "option name PawnValueEG type spin default 94 min 40 max 160",
        "option name KnightValueMG type spin default 337 min 200 max 500",
        "option name KnightValueEG type spin default 291 min 200 max 500",
        "option name BishopValueMG type spin default 375 min 200 max 500",
        "option name BishopValueEG type spin default 297 min 200 max 500",
        "option name RookValueMG type spin default 477 min 350 max 700",
        "option name RookValueEG type spin default 512 min 350 max 700",
        "option name QueenValueMG type spin default 1025 min 700 max 1300",
        "option name QueenValueEG type spin default 936 min 700 max 1300",
        "option name DoubledPawnMG type spin default -15 min -50 max 0",
        "option name DoubledPawnEG type spin default -15 min -50 max 0",
        "option name IsolatedPawnMG type spin default -10 min -50 max 0",
        "option name IsolatedPawnEG type spin default -10 min -50 max 0",
        "option name PassedPawn4MG type spin default 5 min 0 max 200",
        "option name PassedPawn4EG type spin default 10 min 0 max 200",
        "option name PassedPawn5MG type spin default 30 min 0 max 200",
        "option name PassedPawn5EG type spin default 40 min 0 max 200",
        "option name PassedPawn6MG type spin default 25 min 0 max 200",
        "option name PassedPawn6EG type spin default 70 min 0 max 200",
        "option name PassedPawn7MG type spin default 90 min 0 max 200",
        "option name PassedPawn7EG type spin default 120 min 0 max 200",
    }
    psqt_defaults = {
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
    for rank, files in psqt_defaults.items():
        for file_name, (middlegame, endgame) in files.items():
            expected_tuning_options.add(
                f"option name PawnPSQT{file_name}{rank}MG "
                f"type spin default {middlegame} min -250 max 250"
            )
            expected_tuning_options.add(
                f"option name PawnPSQT{file_name}{rank}EG "
                f"type spin default {endgame} min -250 max 250"
            )
    missing_options = expected_tuning_options.difference(lines)
    if missing_options:
        raise RuntimeError(f"missing tuning options: {sorted(missing_options)}")

    tuning_values = {
        "RFPDepth": 6,
        "RFPThreshold": 100,
        "NMPReduction": 4,
        "IIRBaseDepth": 3,
        "IIRCutPenalty": 1,
        "IIRReduction": 2,
        "LMRDepth": 5,
        "LMRInCheckPenalty": 2,
        "LMRGivesCheckPenalty": 2,
    }
    for name, value in tuning_values.items():
        send(f"setoption name {name} value {value}")

    send("setoption name Threads value 4")
    send("isready")
    wait_for(lambda line: line == "readyok", "readyok")
    send("position startpos")
    send("go depth 30")
    wait_for(lambda line: line.startswith("info depth "), "active SMP search")
    send("stop")
    wait_for(lambda line: line.startswith("bestmove "), "SMP bestmove")

    send("setoption name Threads value 1")
    send("position startpos moves e2e4 e7e5")
    send("go movetime 500")
    wait_for(lambda line: line.startswith("bestmove "), "single-thread bestmove")

    send("quit")
    process.wait(timeout=15)
    reader.join(timeout=1)
    while not output.empty():
        line = output.get_nowait()
        if line is not None:
            lines.append(line)

    stderr = process.stderr.read() if process.stderr is not None else ""
    if process.returncode != 0:
        raise RuntimeError(f"engine exited with {process.returncode}: {stderr}")

    bestmoves = [line for line in lines if line.startswith("bestmove ")]
    if len(bestmoves) != 2:
        raise RuntimeError(f"expected two bestmoves, got {len(bestmoves)}")
    for line in bestmoves:
        if not re.fullmatch(
            r"bestmove (?:[a-h][1-8][a-h][1-8][qrbn]?|0000)"
            r"(?: ponder [a-h][1-8][a-h][1-8][qrbn]?)?",
            line,
        ):
            raise RuntimeError(f"invalid bestmove line: {line}")

    print("Lazy SMP UCI lifecycle passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
