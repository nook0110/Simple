#pragma once
#include "globals.h"

struct Square {
    unsigned char x, y;
    inline const size_t getInd() const noexcept { return SQUARE(x, y); }
};

enum MoveType {
    DEFAULT,
    EN_PASSANT,
    PROMOTION,
};

struct Move {
    Square from;
    Square to;
    MoveType moveType;
    char pieceTo = '-';
};