#!/usr/bin/env python3

import argparse
import hashlib
import json
from collections import Counter
from pathlib import Path

import chess.pgn


RESULTS = {"1-0": 1.0, "1/2-1/2": 0.5, "0-1": 0.0}


def game_split(game_id: str) -> str:
    bucket = int.from_bytes(
        hashlib.blake2b(game_id.encode(), digest_size=8).digest(), "big"
    ) % 20
    if bucket < 14:
        return "train"
    if bucket < 17:
        return "validation"
    return "holdout"


def normalized_fen(board: chess.Board) -> str:
    fields = board.fen(en_passant="fen").split()
    return " ".join((*fields[:4], "0", "1"))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("inputs", type=Path, nargs="+")
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--book-plies", type=int, default=20)
    parser.add_argument("--sample-every", type=int, default=4)
    parser.add_argument("--max-games", type=int, default=0)
    parser.add_argument("--max-positions", type=int, default=0)
    args = parser.parse_args()

    args.output.parent.mkdir(parents=True, exist_ok=True)
    seen_positions: set[str] = set()
    split_counts: Counter[str] = Counter()
    result_counts: Counter[str] = Counter()
    games = positions = 0

    with args.output.open("w") as output:
        for input_path in args.inputs:
            with input_path.open(errors="replace") as pgn:
                game_index = 0
                while True:
                    game = chess.pgn.read_game(pgn)
                    if game is None:
                        break
                    result_text = game.headers.get("Result", "*")
                    if result_text not in RESULTS:
                        game_index += 1
                        continue
                    game_id = f"{input_path.resolve()}:{game_index}"
                    split = game_split(game_id)
                    result = RESULTS[result_text]
                    board = game.board()
                    for ply, move in enumerate(game.mainline_moves(), start=1):
                        board.push(move)
                        if ply <= args.book_plies:
                            continue
                        if (ply - args.book_plies) % args.sample_every:
                            continue
                        if board.is_game_over():
                            continue
                        fen = normalized_fen(board)
                        key = " ".join(fen.split()[:4])
                        if key in seen_positions:
                            continue
                        seen_positions.add(key)
                        output.write(json.dumps({
                            "fen": fen,
                            "result": result,
                            "split": split,
                            "game_id": game_id,
                        }, sort_keys=True) + "\n")
                        positions += 1
                        split_counts[split] += 1
                        result_counts[result_text] += 1
                        if args.max_positions and positions >= args.max_positions:
                            break
                    games += 1
                    game_index += 1
                    if args.max_positions and positions >= args.max_positions:
                        break
                    if args.max_games and games >= args.max_games:
                        break
            if args.max_positions and positions >= args.max_positions:
                break
            if args.max_games and games >= args.max_games:
                break

    summary = {
        "games": games,
        "positions": positions,
        "splits": dict(split_counts),
        "results": dict(result_counts),
    }
    args.output.with_suffix(".summary.json").write_text(
        json.dumps(summary, indent=2, sort_keys=True) + "\n"
    )
    print(json.dumps(summary, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
