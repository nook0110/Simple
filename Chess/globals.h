#pragma once
#include<array>
#include<string>
#undef class
#undef new
#undef struct
#undef delete

extern std::string board;

extern std::string info;

extern unsigned char sideToMove;
inline constexpr unsigned int enPassantTargetColumn = 2;
inline constexpr unsigned int enPassantTargetRow = 3;