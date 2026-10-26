from subprocess import Popen, PIPE, STDOUT
import re
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("pgn")
args = parser.parse_args()

with open(args.pgn) as f:
    lc = Popen(["../player/leiserchess"], stdout=PIPE, stdin=PIPE, text=True)

    # get all games in pgn (game = list of moves)
    games = []
    current_game = []
    white_score = 0
    black_score = 0
    draws = 0
    for line in f.readlines():
        # line before moves is always [Result ...]
        if line.startswith("[Result "):
            if line.startswith('[Result "1-0"]'):
                white_score += 1
            elif line.startswith('[Result "0-1"]'):
                black_score += 1
            else:
                white_score += 0.5
                black_score += 0.5
                draws += 1
            if len(current_game) != 0:
                games.append(current_game)
            current_game = []
        current_game.extend(re.findall(r"(\w*)\s*{[^{}]*}", line))

    # make a list of commands to replay each game in
    command = "\n".join(
        [
            command
            for game in games
            for command in (
                ["position startpos"]
                + [f"move {move}" for move in game]
                + ["isready", "display", "isready"]
            )
        ]
    )
    command += "\nquit\n"
    board = lc.communicate(input=command)[0]
    ready_indices = [m.start(0) for m in re.finditer(r"readyok\n", board)]
    prev_idx = None
    print("Pawns at end of game:")
    print("White\tBlack")
    white_pawns = []
    black_pawns = []
    for idx in ready_indices:
        if prev_idx is not None:
            pawn_ori = ["sw", "se", "nw", "ne"]
            pawns = [
                len(
                    re.findall(
                        "|".join(
                            [f"\s{ori if color else ori.upper()}\s" for ori in pawn_ori]
                        ),
                        board[prev_idx:idx],
                    )
                )
                for color in range(2)
            ]
            print(f"{pawns[0]}\t{pawns[1]}")
            white_pawns.append(pawns[0])
            black_pawns.append(pawns[1])
            prev_idx = None
        else:
            prev_idx = idx
    print(
        f"Mean pawns by end: {sum(w + b for w, b in zip(white_pawns, black_pawns)) / len(white_pawns)}"
    )
    print(f"Mean game length (ply): {sum(len(game)for game in games) / len(games)}")
    print(f"Total score: {white_score} - {black_score}")
    print(f"Draw rate: {draws / (white_score + black_score)}")
