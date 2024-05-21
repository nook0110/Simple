@ECHO OFF
cutechess-cli -engine proto=uci cmd="bin/Chess_ef" name="SCEEF" -engine cmd="bin/Chess" proto=uci name="SCE" -each tc=10+0.1 -games 2 -rounds 100 -repeat -concurrency 12 -ratinginterval 10 -openings file="Pohl.epd" format=epd order=random -sprt elo0=0 elo1=10 alpha=0.05 beta=0.05 -pgnout "games.pgn" > "output.txt"
PAUSE