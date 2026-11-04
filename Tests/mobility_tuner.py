#!/usr/bin/env python3

from search_tuner import Parameter


PARAMETERS = {
    "KnightMobilityMG": Parameter(4, 0, 12, 1),
    "KnightMobilityEG": Parameter(2, 0, 12, 1),
    "BishopMobilityMG": Parameter(7, 0, 12, 1),
    "BishopMobilityEG": Parameter(3, 0, 12, 1),
    "RookMobilityMG": Parameter(2, 0, 10, 1),
    "RookMobilityEG": Parameter(4, 0, 10, 1),
    "QueenMobilityMG": Parameter(2, 0, 8, 1),
    "QueenMobilityEG": Parameter(2, 0, 8, 1),
}
