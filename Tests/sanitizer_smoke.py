import argparse
import subprocess


POSITIONS = (
    "startpos",
    "fen r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
    "fen 4k3/P7/8/8/8/8/7p/4K3 w - - 0 1",
)


def read_until(process, prefix):
    while True:
        line = process.stdout.readline()
        if not line:
            raise RuntimeError(f"engine exited before {prefix}")
        if line.startswith(prefix):
            return line


def send(process, command):
    process.stdin.write(command + "\n")
    process.stdin.flush()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--engine", required=True)
    args = parser.parse_args()
    with subprocess.Popen(
        [args.engine], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
        text=True, bufsize=1,
    ) as process:
        send(process, "uci")
        read_until(process, "uciok")
        send(process, "setoption name Threads value 1")
        for position in POSITIONS:
            send(process, "ucinewgame")
            send(process, f"position {position}")
            send(process, "go depth 5")
            read_until(process, "bestmove ")
        send(process, "quit")
        if process.wait(timeout=30) != 0:
            raise RuntimeError("engine failed")


if __name__ == "__main__":
    main()
