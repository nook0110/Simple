#pragma once
#include "globals.h"

struct Square {
    unsigned char x, y;
    Square(unsigned char x, unsigned char y) :x(x), y(y) {}
    Square(const int ind) : x(ind >> 3), y(ind & 7) {}
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
    char captured = '-';
};