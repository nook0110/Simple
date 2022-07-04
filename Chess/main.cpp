#include <iostream>
#include <array>
#include <string>
#include "position.h"
#include "globals.h"


// rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w -- 0 1

int main()
{
	std::string FEN;
	std::getline(std::cin, FEN);
	position.init(FEN);
	position.logAttackMap();
	return 0;
}