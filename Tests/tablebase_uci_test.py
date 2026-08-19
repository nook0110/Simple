#!/usr/bin/env python3

import argparse
import pathlib
import queue
import re
import struct
import subprocess
import tempfile
import threading


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--engine", required=True)
    parser.add_argument("--generator", required=True)
    args = parser.parse_args()

    with tempfile.TemporaryDirectory(prefix="sce-tb-uci-") as directory:
        tablebase = pathlib.Path(directory) / "two-piece.scetb"
        subprocess.run(
            [args.generator, "--output", str(tablebase), "--max-pieces", "2",
             "--threads", "2"],
            check=True,
            timeout=60,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
        )
        corrupt_tablebase = pathlib.Path(directory) / "corrupt.scetb"
        corrupt_payload = bytearray(tablebase.read_bytes())
        header = struct.unpack_from("<8sIIIIQQQ2Q", corrupt_payload)
        directory_offset = header[5]
        directory = struct.unpack_from(
            "<B2sBI10Q", corrupt_payload, directory_offset)
        wdl_offset = directory[10]
        corrupt_payload[wdl_offset] ^= 1
        corrupt_tablebase.write_bytes(corrupt_payload)

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

        def wait_for(pattern: str, timeout: float = 15.0) -> str:
            regex = re.compile(pattern)
            while True:
                line = output.get(timeout=timeout)
                if line is None:
                    raise RuntimeError(f"engine exited before {pattern}")
                if regex.match(line):
                    return line

        send("uci")
        wait_for(r"option name SceTablebasePath type string default sce-4.scetb")
        wait_for(r"uciok")
        send(f"setoption name SceTablebasePath value {tablebase}")
        send("isready")
        wait_for(r"readyok")
        send("position fen 7k/8/8/8/8/8/8/K7 w - - 0 1")
        send("go depth 20")
        bestmove = wait_for(r"bestmove [a-h][1-8][a-h][1-8][qrbn]?")
        if bestmove == "bestmove 0000":
            raise RuntimeError("tablebase root probe returned no move")

        send(f"setoption name SceTablebasePath value {corrupt_tablebase}")
        wait_for(r"info string failed to load SCE tablebase")
        send("setoption name SceTablebasePath value /missing/sce.scetb")
        wait_for(r"info string failed to load SCE tablebase")
        send("isready")
        wait_for(r"readyok")
        send("quit")
        process.wait(timeout=15)
        if process.returncode != 0:
            stderr = process.stderr.read() if process.stderr else ""
            raise RuntimeError(stderr)
    print("SCE tablebase UCI lifecycle passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
