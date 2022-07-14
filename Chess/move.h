#pragma once
#include "globals.h"

struct Square {
    Square() {};
    square rank, file;
    Square(square rank, square file) :rank(rank), file(file) {}
    Square(square ind) : rank(ind >> 3), file(ind & 7) {}
    inline const square getInd() const noexcept { return SQUARE(rank, file); }
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
    char captured = '-';
};