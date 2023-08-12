// ReSharper disable once CppPrecompiledHeaderIsNotIncluded
#include "../Chess/MoveGenerator.h"
#include "../Chess/PositionFactory.h"
#include "pch.h"

using namespace SimpleChessEngine;

namespace MoveGenerator
{
TEST(MoveGenerator, MoveGenerator)
{
  constexpr auto kDefault = PositionFactory{}();
}
}  // namespace MoveGenerator