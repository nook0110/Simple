import chess
import sys

def convert_line(line):
    line = line.strip()
    if not line:
        return line
    
    try:
        fen_part, bm_part = line.split('bm', 1)
    except ValueError:
        return line
    
    fen = fen_part.strip()
    
    bm_split = bm_part.split(';', 1)
    san_moves = bm_split[0].strip().split()
    
    board = chess.Board(fen)
    uci_moves = []
    for san in san_moves:
        try:
            move = board.parse_san(san)
            uci_moves.append(move.uci())
        except Exception as e:
            print(f"Error processing move '{san}' in line: {line} - {e}", file=sys.stderr)
            uci_moves.append(san)  # Keep the original if conversion fails
    
    return f"{fen}#{' '.join(uci_moves)}"

for line in sys.stdin:
    converted = convert_line(line)
    print(converted)