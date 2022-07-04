#ifndef GLOBALS_H
#define GLOBALS_H

#include <string>
#include <array>
#include <unordered_map>

#define SQUARE(x,y) (x << 3 | y)

inline const std::unordered_map<char, const int> PIECES = { {'P', 0}, {'N', 1}, {'B', 2}, {'R', 3}, {'Q', 4}, {'K', 5},
															{'p', 6}, {'n', 7}, {'b', 8}, {'r', 9}, {'q', 10}, {'k', 11},
															{'-', 12} };

#endif // !GLOBALS_H