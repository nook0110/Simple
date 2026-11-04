#!/usr/bin/env python3

import argparse
import re
import subprocess


QevalPattern = re.compile(r"qeval white_cp (-?\d+) nodes (\d+)")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--engine", required=True)
    args = parser.parse_args()
    process = subprocess.Popen(
        [args.engine], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT, text=True, bufsize=1,
    )
    assert process.stdin is not None
    assert process.stdout is not None

    def send(command: str) -> None:
        process.stdin.write(command + "\n")
        process.stdin.flush()

    def read(prefix: str) -> str:
        while True:
            raw_line = process.stdout.readline()
            if raw_line == "" and process.poll() is not None:
                raise RuntimeError(f"engine exited before {prefix!r}")
            line = raw_line.strip()
            if line.startswith(prefix):
                return line

    try:
        send("uci")
        read("uciok")
        send("position fen 8/5pk1/3p2p1/2pPp3/2P1P1P1/5PK1/8/8 w - - 0 1")
        send("go hash")
        initial_hash = read("hash: ")
        values = []
        for _ in range(2):
            send("go qeval")
            line = read("qeval ")
            match = QevalPattern.fullmatch(line)
            if match is None:
                raise RuntimeError(f"invalid qeval line: {line}")
            values.append((int(match.group(1)), int(match.group(2))))
        if values[0] != values[1]:
            raise RuntimeError(f"qeval is not deterministic: {values}")
        send("go hash")
        if read("hash: ") != initial_hash:
            raise RuntimeError("qeval changed the current position")
        send("quit")
        process.wait(timeout=10)
    finally:
        if process.poll() is None:
            process.terminate()
            process.wait(timeout=10)
    print(f"deterministic qeval: score={values[0][0]} nodes={values[0][1]}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
