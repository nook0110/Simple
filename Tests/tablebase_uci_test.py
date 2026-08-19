#!/usr/bin/env python3

import argparse
import pathlib
import queue
import re
import subprocess
import threading


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--engine", required=True)
    parser.add_argument("--probe", required=True)
    parser.add_argument("--tablebase", type=pathlib.Path, required=True)
    args = parser.parse_args()

    process = subprocess.Popen(
        [args.engine], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
        stderr=subprocess.PIPE, text=True, bufsize=1)
    assert process.stdin is not None
    assert process.stdout is not None
    output: queue.Queue[str | None] = queue.Queue()

    def read_output() -> None:
        for line in process.stdout:
            output.put(line.rstrip())
        output.put(None)

    threading.Thread(target=read_output, daemon=True).start()

    def send(command: str) -> None:
        process.stdin.write(command + "\n")
        process.stdin.flush()

    def wait_for(pattern: str, timeout: float = 30.0) -> str:
        regex = re.compile(pattern)
        while True:
            line = output.get(timeout=timeout)
            if line is None:
                raise RuntimeError(f"engine exited before {pattern}")
            if regex.match(line):
                return line

    send("uci")
    wait_for(r"option name SyzygyPath type string default syzygy")
    wait_for(r"uciok")
    def load_and_probe() -> None:
        send(f"setoption name SyzygyPath value {args.tablebase}")
        send("isready")
        wait_for(r"readyok")
        send("position fen 7k/8/8/8/8/8/8/KQ6 w - - 0 1")
        send("go depth 20")
        wait_for(r"bestmove [a-h][1-8][a-h][1-8][qrbn]?")

    load_and_probe()
    exact = subprocess.check_output(
        [args.probe, str(args.tablebase),
         "7k/8/8/8/8/8/8/KQ6 w - - 87 1"], text=True)
    cursed = subprocess.check_output(
        [args.probe, str(args.tablebase),
         "7k/8/8/8/8/8/8/KQ6 w - - 88 1"], text=True)
    if not exact.startswith("wdl=2 dtz=13 move="):
        raise RuntimeError(f"unexpected exact Syzygy result: {exact}")
    if not cursed.startswith("wdl=1 dtz=13 move="):
        raise RuntimeError(f"unexpected cursed Syzygy result: {cursed}")

    send("setoption name SyzygyPath value /missing/syzygy")
    wait_for(r"info string failed to load Syzygy tablebases")
    load_and_probe()

    send("quit")
    process.wait(timeout=30)
    if process.returncode != 0:
        stderr = process.stderr.read() if process.stderr else ""
        raise RuntimeError(stderr)
    print("Syzygy UCI lifecycle passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
