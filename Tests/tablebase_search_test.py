#!/usr/bin/env python3

import argparse
import os
import pathlib
import queue
import re
import subprocess
import tempfile
import threading
import time


TABLEBASE = pathlib.Path("/home/blokhtin/github/lichess-bot/engines/syzygy")
INVALID_FENS = (
    "8/8/8/4k3/8/8/2K5/4R3 w - - 0 1",
    "8/8/8/4k3/8/8/2K5/4Q3 w - - 0 1",
    "8/8/8/4k3/8/8/2K5/4R2R w - - 0 1",
)
VALID_FENS = (
    "8/8/8/4k3/8/8/2K5/R7 w - - 0 1",
    "8/8/8/4k3/8/8/2K5/3Q4 w - - 0 1",
    "8/8/8/4k3/8/8/2K5/R6R w - - 0 1",
    "8/8/8/4k3/8/8/2K5/4R3 b - - 0 1",
    "8/8/8/4k3/8/8/2K5/4Q3 b - - 0 1",
    "8/8/8/4k3/8/8/2K5/4R2R b - - 0 1",
    "8/8/8/4k3/8/8/2KP4/8 w - - 0 1",
    "8/8/8/4k3/8/8/2KPP3/8 w - - 0 1",
)
CLOCK = "wtime 2000 btime 2000 winc 0 binc 0 movestogo 20"


class UciEngine:
    def __init__(self, executable: str):
        self.process = subprocess.Popen(
            [executable], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT, text=True, bufsize=1)
        self.output: queue.Queue[str | None] = queue.Queue()
        self.lines: list[str] = []
        threading.Thread(target=self.read_output, daemon=True).start()

    def read_output(self) -> None:
        assert self.process.stdout is not None
        for line in self.process.stdout:
            self.output.put(line.rstrip())
        self.output.put(None)

    def send(self, command: str) -> None:
        assert self.process.stdin is not None
        self.process.stdin.write(command + "\n")
        self.process.stdin.flush()

    def wait_for(self, pattern: str, timeout: float = 3.0) -> str:
        deadline = time.monotonic() + timeout
        while True:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                raise RuntimeError(f"timed out waiting for {pattern}: {self.lines[-20:]}")
            try:
                line = self.output.get(timeout=remaining)
            except queue.Empty as error:
                raise RuntimeError(f"timed out waiting for {pattern}: {self.lines[-20:]}") from error
            if line is None:
                raise RuntimeError(f"engine exited before {pattern}: {self.lines[-20:]}")
            self.lines.append(line)
            if re.fullmatch(pattern, line):
                return line

    def ready(self) -> None:
        self.send("isready")
        self.wait_for("readyok")

    def search(self, fen: str, control: str, invalid: bool = False,
               root_hit: bool = False) -> None:
        self.send(f"position fen {fen}")
        start = len(self.lines)
        self.send(f"go {control}")
        expected = "0000" if invalid else r"[a-h][1-8][a-h][1-8][qrbn]?"
        self.wait_for(rf"bestmove {expected}( ponder [a-h][1-8][a-h][1-8][qrbn]?)?",
                      timeout=1.0 if control == CLOCK else 2.0)
        lines = self.lines[start:]
        if invalid and "info string invalid position: illegal kings" not in lines:
            raise RuntimeError(f"invalid root was not rejected: {fen}")
        if root_hit and any(line.startswith("info depth ") for line in lines):
            raise RuntimeError(f"root tablebase probe failed: {fen}")

    def close(self) -> None:
        if self.process.poll() is None:
            self.process.kill()
        self.process.wait(timeout=3)
        if self.process.stdin:
            self.process.stdin.close()
        if self.process.stdout:
            self.process.stdout.close()


def run(executable: str) -> None:
    engine = UciEngine(executable)
    try:
        engine.send("uci")
        engine.wait_for("uciok", timeout=10)
        for option, variable in (("EvalFile", "SCE_NNUE_BIG"),
                                 ("EvalFileSmall", "SCE_NNUE_SMALL")):
            if path := os.environ.get(variable):
                engine.send(f"setoption name {option} value {path}")
        for threads in (1, 2):
            engine.send(f"setoption name Threads value {threads}")
            engine.send(f"setoption name SyzygyPath value {TABLEBASE}")
            engine.ready()
            for fen in INVALID_FENS:
                for control in ("depth 2", "movetime 1000", CLOCK):
                    engine.search(fen, control, invalid=True)
            for fen in VALID_FENS:
                for control in ("movetime 1000", CLOCK):
                    engine.search(fen, control, root_hit=True)
            for fen in (*INVALID_FENS, *VALID_FENS[:3]):
                engine.send(f"position fen {fen}")
                engine.send(f"go ponder {CLOCK}")
                engine.ready()
                engine.send("stop")
                engine.wait_for(r"bestmove .*", timeout=1)

            with tempfile.TemporaryDirectory(prefix="sce-wdl-only-") as directory:
                path = pathlib.Path(directory)
                for name in ("KRvK", "KQvK", "KRRvK"):
                    (path / f"{name}.rtbw").symlink_to(TABLEBASE / f"{name}.rtbw")
                engine.send(f"setoption name SyzygyPath value {path}")
                engine.ready()
                for fen in VALID_FENS[:3]:
                    engine.search(fen, "movetime 100")
                    engine.search(fen, CLOCK)
                    engine.send(f"position fen {fen}")
                    engine.send(f"go ponder {CLOCK}")
                    engine.wait_for("info depth 2")
                    engine.send("stop")
                    engine.wait_for(r"bestmove .*", timeout=1)
                engine.send(f"setoption name SyzygyPath value {TABLEBASE}")
                engine.ready()

        engine.send(f"position fen {INVALID_FENS[0]}")
        engine.send(f"go ponder {CLOCK}")
        engine.ready()
        engine.send("quit")
        engine.process.wait(timeout=1)
        if engine.process.returncode != 0:
            raise RuntimeError(f"engine exited with {engine.process.returncode}")
    finally:
        engine.close()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--engine", required=True)
    args = parser.parse_args()
    if not TABLEBASE.is_dir():
        print(f"Skipped: tablebase directory is missing: {TABLEBASE}")
        return 77
    run(args.engine)
    print("Syzygy search regression passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
