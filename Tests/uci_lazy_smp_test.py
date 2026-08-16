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
