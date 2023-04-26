#pragma once
#include <array>
#include <iostream>
#include <type_traits>

/**
 * \brief A simple implementation of the alpha-beta search algorithm with
 * transposition tables and iterative deepening.
 *
 * \tparam Position The type of the position.
 * \tparam MoveGenerator The type of the move generator.
 * \tparam Evaluator The type of the evaluator.
 * \tparam HashTable The type of the hash table.
 */
template <class Position, class MoveGenerator, class Evaluator, class HashTable>
class AlphaBetaSearcher
{
 public:
  using Moves = std::invoke_result_t<MoveGenerator, const Position&>;
  using Move = typename Moves::value_type;
  using Eval = std::invoke_result_t<Evaluator, const Position&>;

  /**
   * \brief Constructor.
   *
   * \param move_generator The move generator.
   * \param evaluator The evaluator.
   * \param position The initial position.
   */
  explicit AlphaBetaSearcher(Position position = Position(),
                             MoveGenerator move_generator = MoveGenerator(),
                             Evaluator evaluator = Evaluator())
      : current_position_(std::move(position)),
        move_generator_(std::move(move_generator)),
        evaluator_(std::move(evaluator))
  {}

  /**
   * \brief Sets the current position.
   *
   * \param position New position.
   */
  void SetPosition(Position position);

  /**
   * \brief Returns the current position.
   *
   * \return The current position.
   */
  [[nodiscard]] const Position& GetPosition() const;

  /**
   * \brief Returns the best move for the current position.
   *
   * \return The current best move.
   */
  [[nodiscard]] Move GetCurrentBestMove() const;

  /**
   * \brief Calculates the best move for the current position.
   *
   * \details This function uses iterative deepening to find the best move.
   *
   * \param depth The maximum depth to search.
   *
   * \return The best move.
   */
  Move ComputeBestMove(size_t depth);

 private:
  /**
   * \brief Starts the alpha-beta search algorithm.
   *
   * \param depth The max depth.
   */
  void FindBestMove(size_t depth);

  /**
   * \brief Performs the alpha-beta search algorithm.
   *
   * \param depth The current depth.
   * \param alpha The current alpha value.
   * \param beta The current beta value.
   *
   * \return Evaluation of subtree.
   */
  [[nodiscard]] Eval AlphaBeta(size_t depth, Eval alpha, Eval beta);

  Position current_position_;  //!< Current position.

  MoveGenerator move_generator_;  //!< Move generator.
  Evaluator evaluator_;           //!< Evaluator.

  HashTable best_moves_;  //!< Hash table to store the best moves.
};

template <class Position, class MoveGenerator, class Evaluator, class HashTable>
void AlphaBetaSearcher<Position, MoveGenerator, Evaluator,
                       HashTable>::SetPosition(Position position)
{
  current_position_ = std::move(position);
}

template <class Position, class MoveGenerator, class Evaluator, class HashTable>
const Position& AlphaBetaSearcher<Position, MoveGenerator, Evaluator,
                                  HashTable>::GetPosition() const
{
  return current_position_;
}

template <class Position, class MoveGenerator, class Evaluator, class HashTable>
void AlphaBetaSearcher<Position, MoveGenerator, Evaluator,
                       HashTable>::FindBestMove(const size_t depth)
{
  // current best evaluation
  Eval best_eval = std::numeric_limits<Eval>::min();

  for (auto moves = move_generator_(current_position_);
       const auto& move : moves)
  {
    // make the move and search the tree
    current_position_.DoMove(move);
    auto temp_eval = -AlphaBeta(depth - 1, std::numeric_limits<Eval>::min(),
                                std::numeric_limits<Eval>::max());

    // undo the move
    current_position_.UndoMove(move);

    // check if we have found a better move
    if (temp_eval > best_eval)
    {
      best_eval = temp_eval;
      best_moves_[current_position_] = move;
    }
  }
}

template <class Position, class MoveGenerator, class Evaluator, class HashTable>
typename AlphaBetaSearcher<Position, MoveGenerator, Evaluator, HashTable>::Move
AlphaBetaSearcher<Position, MoveGenerator, Evaluator,
                  HashTable>::GetCurrentBestMove() const
{
  return best_moves_[current_position_];
}

template <class Position, class MoveGenerator, class Evaluator, class HashTable>
typename AlphaBetaSearcher<Position, MoveGenerator, Evaluator, HashTable>::Move
AlphaBetaSearcher<Position, MoveGenerator, Evaluator,
                  HashTable>::ComputeBestMove(size_t depth)
{
  for (size_t current_depth = 0; current_depth < depth; ++current_depth)
  {
    FindBestMove(current_depth);
  }

  return GetCurrentBestMove();
}

template <class Position, class MoveGenerator, class Evaluator, class HashTable>
typename AlphaBetaSearcher<Position, MoveGenerator, Evaluator, HashTable>::Eval
AlphaBetaSearcher<Position, MoveGenerator, Evaluator, HashTable>::AlphaBeta(
    const size_t depth, Eval alpha, Eval beta)
{
  // return the evaluation of the current position if we have reached the end of
  // the search tree
  if (depth == 0)
  {
    return evaluator_(current_position_);
  }

  // lambda function to analyze a move
  auto analyze_move = [this, alpha, beta, depth](const auto& move)
  {
    // make the move and search the tree
    current_position_.DoMove(move);
    auto temp_eval = -AlphaBeta(depth - 1, -beta, -alpha);

    // undo the move
    current_position_.UndoMove(move);

    // return the evaluation
    return temp_eval;
  };

  // check if we have already searched this position
  if (best_moves_.Contains(current_position_))
  {
    auto pv_move = best_moves_[current_position_];

    auto temp_eval = analyze_move(pv_move);

    // check if we have found a better move
    if (temp_eval >= beta)
    {
      return beta;
    }

    // update the best move
    if (temp_eval > alpha)
    {
      alpha = temp_eval;
    }
  }

  // get all the possible moves
  auto moves = move_generator_(current_position_);

  // check if there are no possible moves
  if (moves.empty())
  {
    return evaluator_.GetGameResult(current_position_);
  }

  // search the tree
  for (auto& move : moves)
  {
    auto temp_eval = analyze_move(move);

    // check if we have found a better move
    if (temp_eval >= beta)
    {
      best_moves_[current_position_] = std::move(move);

      return beta;
    }

    // update the best move
    if (temp_eval > alpha)
    {
      best_moves_[current_position_] = std::move(move);

      alpha = temp_eval;
    }
  }

  // return the best evaluation
  return alpha;
}
