#pragma once
#include<array>
#include<string>
#undef class
#undef new
#undef struct
#undef delete

extern std::string board;

extern std::string info;

inline constexpr unsigned int sideToMove = 0;
inline constexpr unsigned int enPassantTargetColumn = 2;
inline constexpr unsigned int enPassantTargetRow = 3;