@ECHO OFF
cutechess-cli -engine proto=uci cmd="bin/Chess" name="SCE" -engine cmd="bin/Chess_ef" proto=uci name="SCEEF" -each tc=10+0.1 -games 2 -rounds 20000 -repeat -concurrency 4 -ratinginterval 10 -openings file="Pohl.epd" format=epd order=random -sprt elo0=0 elo1=10 alpha=0.05 beta=0.05 -pgnout "games.pgn"
PAUSE