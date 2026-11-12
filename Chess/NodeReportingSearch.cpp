#include "SimpleChessEngine.h"

namespace SimpleChessEngine {
void ChessEngine::ComputeBestMoveImpl(
    NodeReportingCondition<TimeCondition>& condition) {
  ComputeBestMoveImpl<NodeReportingCondition<TimeCondition>>(condition);
}

void ChessEngine::ComputeBestMoveImpl(
    NodeReportingCondition<DepthCondition>& condition) {
  ComputeBestMoveImpl<NodeReportingCondition<DepthCondition>>(condition);
}

void ChessEngine::ComputeBestMoveImpl(
    NodeReportingCondition<NodeCondition>& condition) {
  ComputeBestMoveImpl<NodeReportingCondition<NodeCondition>>(condition);
}

void ChessEngine::ComputeBestMoveImpl(
    NodeReportingCondition<Pondering>& condition) {
  ComputeBestMoveImpl<NodeReportingCondition<Pondering>>(condition);
}
}
