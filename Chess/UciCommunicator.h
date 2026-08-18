#pragma once

#include <algorithm>
#include <cassert>
#include <functional>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <variant>
#include <vector>

#include "MoveFactory.h"
#include "Perft.h"
#include "Position.h"
#include "Quiescence.h"
#include "SimpleChessEngine.h"

namespace SimpleChessEngine {
struct TournamentTime {
  std::array<std::chrono::milliseconds, 2> player_time = {
      std::chrono::milliseconds{0}, std::chrono::milliseconds{0}};

  std::array<std::chrono::milliseconds, 2> player_inc = {
      std::chrono::milliseconds{0}, std::chrono::milliseconds{0}};
};

struct TimePerMove {
  std::chrono::milliseconds movetime;
};

struct MaxDepth {
  Depth depth;
};

using TimeControl = std::variant<TournamentTime, TimePerMove, MaxDepth>;

struct Info {
  Position position;
  TimeControl time_control;
};

class SearchThread {
 public:
  explicit SearchThread(Position position, std::ostream& o_stream);
  explicit SearchThread(std::ostream& o_stream);

  void Start(const Info& info);
  void PonderHit(const Info& info) {
    assert(pondering_);
    pondering_->condition = GetCondition(info);
  }
  void GoPonder() {
    StopThread();
    stop_signal_ = std::make_shared<std::atomic_bool>(false);
    pondering_.emplace(stop_signal_);
    thread_ = std::thread([this] { engine_.GoPonder(*pondering_); });
  }

  void Stop();
  void SetThreadCount(std::size_t thread_count) {
    StopThread();
    thread_count_ = std::max<std::size_t>(1, thread_count);
  }

 private:
  Condition GetCondition(const Info& info) {
    if (const auto tournament =
            std::get_if<TournamentTime>(&info.time_control)) {
      constexpr size_t kAverageGameLength = 40;
      auto left_time =
          tournament
              ->player_time[static_cast<size_t>(info.position.GetSideToMove())];

      auto inc_time =
          tournament
              ->player_inc[static_cast<size_t>(info.position.GetSideToMove())];
      std::chrono::milliseconds time_for_move =
          left_time / kAverageGameLength + inc_time;
      time_for_move = std::min(left_time / 2, time_for_move);

      return TimeCondition{time_for_move, stop_signal_};
    }
    if (const auto time_per_move =
            std::get_if<TimePerMove>(&info.time_control)) {
      return TimeCondition{time_per_move->movetime, stop_signal_};
    }
    if (const auto max_depth = std::get_if<MaxDepth>(&info.time_control)) {
      return DepthCondition{max_depth->depth, stop_signal_};
    }
    assert(false);
    std::unreachable();
  }

  void StopThread();

  ChessEngine::SharedTranspositionTable transposition_table_ =
      std::make_shared<Searcher::SearcherTranspositionTable>();
  ChessEngine engine_;

  std::optional<Pondering> pondering_;
  std::optional<std::thread> thread_;
  std::vector<std::unique_ptr<ChessEngine>> helper_engines_;
  std::vector<std::thread> helper_threads_;
  StopSignal stop_signal_;
  std::size_t thread_count_ = 1;
};

struct OptionBase {
  OptionBase(std::string name) : name_(std::move(name)) {}

  virtual bool SetValue(const std::string& value) = 0;

  const std::string& GetName() const { return name_; }

  virtual std::string GetOptionDescription() const = 0;

  virtual ~OptionBase() = default;

 private:
  std::string name_;
};

struct SpinOption : public OptionBase {
  using Setter = std::function<void(int)>;

  SpinOption(std::string name, int default_value, int min_value, int max_value,
             Setter setter = {})
      : OptionBase(std::move(name)), value_(default_value),
        default_(default_value), min_(min_value), max_(max_value),
        setter_(setter) {}

  bool SetValue(const std::string& value) override {
    const int parsed = std::stoi(value);
    if (parsed < min_ || parsed > max_) return false;
    value_ = parsed;
    if (setter_) setter_(parsed);
    return true;
  }

  std::string GetOptionDescription() const override {
    return "type spin default " + std::to_string(default_) + " min " +
           std::to_string(min_) + " max " + std::to_string(max_);
  }

  [[nodiscard]] int GetValue() const { return value_; }

 private:
  int value_;
  int default_;
  int min_;
  int max_;
  Setter setter_;
};

template <bool default_value>
struct BooleanOption : public OptionBase {
  BooleanOption(std::string name) : OptionBase(std::move(name)) {}

  bool SetValue(const std::string& value) override {
    if (value == "True" || value == "true") {
      value_ = true;
      return true;
    }
    if (value == "False" || value == "false") {
      value_ = false;
      return true;
    }
    return false;
  }

  std::string GetOptionDescription() const override {
    return std::string("type check default ") +
           (default_value ? "true" : "false");
  }

 private:
  bool value_ = default_value;
};

using PonderOption = BooleanOption<false>;
struct EngineOptions {
  EngineOptions() {
    auto threads = std::make_unique<SpinOption>("Threads", 1, 1, 256);
    threads_ = threads.get();
    options.emplace_back(std::move(threads));
    options.emplace_back(
        std::make_unique<SpinOption>("RFPDepth", 5, 1, 8, [](const int value) {
          Settings::PruneParameters::RFPSettings::kDepthLimit =
              static_cast<Depth>(value);
        }));
    options.emplace_back(std::make_unique<SpinOption>(
        "RFPThreshold", 100, 25, 200, [](const int value) {
          Settings::PruneParameters::RFPSettings::kThreshold = value;
        }));
    options.emplace_back(std::make_unique<SpinOption>(
        "NMPReduction", 3, 2, 5, [](const int value) {
          Settings::PruneParameters::NMPSettings::kNullMoveReduction = value;
        }));
    options.emplace_back(std::make_unique<SpinOption>(
        "IIRBaseDepth", 2, 1, 8, [](const int value) {
          Settings::PruneParameters::IIRSettings::kBaseLimit =
              static_cast<Depth>(value);
        }));
    options.emplace_back(std::make_unique<SpinOption>(
        "IIRCutPenalty", 1, 0, 4, [](const int value) {
          Settings::PruneParameters::IIRSettings::kCutNodePenalty =
              static_cast<Depth>(value);
        }));
    options.emplace_back(std::make_unique<SpinOption>(
        "IIRReduction", 1, 1, 3, [](const int value) {
          Settings::PruneParameters::IIRSettings::kReduction =
              static_cast<Depth>(value);
        }));
    options.emplace_back(
        std::make_unique<SpinOption>("LMRDepth", 3, 2, 8, [](const int value) {
          Settings::PruneParameters::LMRSettings::kDepthLimit = value;
        }));
    options.emplace_back(std::make_unique<SpinOption>(
        "LMRInCheckPenalty", 1, 0, 3, [](const int value) {
          Settings::PruneParameters::LMRSettings::kUnderCheckReductionPenalty =
              value;
        }));
    options.emplace_back(std::make_unique<SpinOption>(
        "LMRGivesCheckPenalty", 2, 0, 3, [](const int value) {
          Settings::PruneParameters::LMRSettings::kDoingCheckReductionPenalty =
              value;
        }));
    AddMaterialOptions();
    AddPawnOptions();
    AddMobilityOptions();
    AddPsqtAdjustmentOptions();
    AddKingSafetyOptions();
  }
  std::vector<std::unique_ptr<OptionBase>> options;
  bool ParseSetoption(std::stringstream command) {
    std::string option_name;
    command >> option_name;

    for (const auto& option : options) {
      if (option->GetName() == option_name) {
        std::string value;
        if (!(command >> value)) /* do sth for button */
          return false;

        if (value != "value") {
          return false;
        }

        command >> value;
        return option->SetValue(value);
      }
    }

    assert(false);
    std::unreachable();
  }

  void PrintOptionsNames(std::ostream& out) {
    for (const auto& option : options) {
      out << "option name " << option->GetName() << ' '
          << option->GetOptionDescription() << '\n';
    }
  }

  [[nodiscard]] std::size_t GetThreadCount() const {
    return static_cast<std::size_t>(threads_->GetValue());
  }

 private:
  void AddPsqtAdjustmentOptions() {
    constexpr std::array piece_names = {"Knight", "Bishop", "Rook", "Queen",
                                        "King"};
    for (size_t piece_offset = 0; piece_offset < piece_names.size();
         ++piece_offset) {
      const auto piece = static_cast<Piece>(
          static_cast<size_t>(Piece::kKnight) + piece_offset);
      for (size_t rank = 0; rank < 8; ++rank) {
        for (size_t file = 0; file < 4; ++file) {
          const std::string square =
              std::string{static_cast<char>('A' + file)} +
              std::to_string(rank + 1);
          const auto piece_index = static_cast<size_t>(piece);
          options.emplace_back(std::make_unique<SpinOption>(
              std::string{piece_names[piece_offset]} + "PSQT" + square + "MG",
              Settings::EvaluationParameters::psqt_adjustment[piece_index][rank]
                                                             [file]
                                                                 .eval[0],
              -100, 100, [piece, rank, file](const int value) {
                Settings::EvaluationParameters::SetPsqtAdjustment(
                    piece, rank, file, GamePhase::kMiddleGame, value);
              }));
          options.emplace_back(std::make_unique<SpinOption>(
              std::string{piece_names[piece_offset]} + "PSQT" + square + "EG",
              Settings::EvaluationParameters::psqt_adjustment[piece_index][rank]
                                                             [file]
                                                                 .eval[1],
              -100, 100, [piece, rank, file](const int value) {
                Settings::EvaluationParameters::SetPsqtAdjustment(
                    piece, rank, file, GamePhase::kEndGame, value);
              }));
        }
      }
    }
  }

  void AddMobilityOptions() {
    AddMobilityOptions<Piece::kKnight>("Knight", 0, 12);
    AddMobilityOptions<Piece::kBishop>("Bishop", 0, 12);
    AddMobilityOptions<Piece::kRook>("Rook", 0, 10);
    AddMobilityOptions<Piece::kQueen>("Queen", 0, 8);
  }

  template <Piece piece>
  void AddMobilityOptions(const std::string& piece_name, const int minimum,
                          const int maximum) {
    constexpr auto piece_index = static_cast<size_t>(piece);
    options.emplace_back(std::make_unique<SpinOption>(
        piece_name + "MobilityMG",
        Settings::EvaluationParameters::mobility[piece_index].eval[0], minimum,
        maximum, [](const int value) {
          Settings::EvaluationParameters::SetMobility(
              piece, GamePhase::kMiddleGame, value);
        }));
    options.emplace_back(std::make_unique<SpinOption>(
        piece_name + "MobilityEG",
        Settings::EvaluationParameters::mobility[piece_index].eval[1], minimum,
        maximum, [](const int value) {
          Settings::EvaluationParameters::SetMobility(
              piece, GamePhase::kEndGame, value);
        }));
  }

  void AddMaterialOptions() {
    AddMaterialValueOptions<Piece::kPawn>("Pawn", 40, 160);
    AddMaterialValueOptions<Piece::kKnight>("Knight", 200, 500);
    AddMaterialValueOptions<Piece::kBishop>("Bishop", 200, 500);
    AddMaterialValueOptions<Piece::kRook>("Rook", 350, 700);
    AddMaterialValueOptions<Piece::kQueen>("Queen", 700, 1300);
  }

  template <Piece piece>
  void AddMaterialValueOptions(const std::string& piece_name, const int minimum,
                               const int maximum) {
    constexpr auto piece_index = static_cast<size_t>(piece);
    options.emplace_back(std::make_unique<SpinOption>(
        piece_name + "ValueMG",
        Settings::EvaluationParameters::material_value[piece_index]
            .eval[static_cast<size_t>(GamePhase::kMiddleGame)],
        minimum, maximum, [](const int value) {
          Settings::EvaluationParameters::SetMaterialValue(
              piece, GamePhase::kMiddleGame, value);
        }));
    options.emplace_back(std::make_unique<SpinOption>(
        piece_name + "ValueEG",
        Settings::EvaluationParameters::material_value[piece_index]
            .eval[static_cast<size_t>(GamePhase::kEndGame)],
        minimum, maximum, [](const int value) {
          Settings::EvaluationParameters::SetMaterialValue(
              piece, GamePhase::kEndGame, value);
        }));
  }

  void AddPawnOptions() {
    options.emplace_back(std::make_unique<SpinOption>(
        "DoubledPawnMG", Settings::EvaluationParameters::doubled_pawn.eval[0],
        -50, 0, [](const int value) {
          Settings::EvaluationParameters::SetDoubledPawn(GamePhase::kMiddleGame,
                                                         value);
        }));
    options.emplace_back(std::make_unique<SpinOption>(
        "DoubledPawnEG", Settings::EvaluationParameters::doubled_pawn.eval[1],
        -50, 0, [](const int value) {
          Settings::EvaluationParameters::SetDoubledPawn(GamePhase::kEndGame,
                                                         value);
        }));
    options.emplace_back(std::make_unique<SpinOption>(
        "IsolatedPawnMG", Settings::EvaluationParameters::isolated_pawn.eval[0],
        -50, 0, [](const int value) {
          Settings::EvaluationParameters::SetIsolatedPawn(
              GamePhase::kMiddleGame, value);
        }));
    options.emplace_back(std::make_unique<SpinOption>(
        "IsolatedPawnEG", Settings::EvaluationParameters::isolated_pawn.eval[1],
        -50, 0, [](const int value) {
          Settings::EvaluationParameters::SetIsolatedPawn(GamePhase::kEndGame,
                                                          value);
        }));
    AddPassedPawnOptions<3>("4");
    AddPassedPawnOptions<4>("5");
    AddPassedPawnOptions<5>("6");
    AddPassedPawnOptions<6>("7");
    AddPawnPsqtRankOptions<5>("6");
    AddPawnPsqtRankOptions<6>("7");
  }

  template <size_t relative_rank>
  void AddPassedPawnOptions(const std::string& rank_name) {
    options.emplace_back(std::make_unique<SpinOption>(
        "PassedPawn" + rank_name + "MG",
        Settings::EvaluationParameters::passed_pawn[relative_rank].eval[0], 0,
        200, [](const int value) {
          Settings::EvaluationParameters::SetPassedPawn(
              relative_rank, GamePhase::kMiddleGame, value);
        }));
    options.emplace_back(std::make_unique<SpinOption>(
        "PassedPawn" + rank_name + "EG",
        Settings::EvaluationParameters::passed_pawn[relative_rank].eval[1], 0,
        200, [](const int value) {
          Settings::EvaluationParameters::SetPassedPawn(
              relative_rank, GamePhase::kEndGame, value);
        }));
  }

  template <size_t relative_rank, size_t file>
  void AddPawnPsqtSquareOptions(const std::string& square_name) {
    options.emplace_back(std::make_unique<SpinOption>(
        "PawnPSQT" + square_name + "MG",
        Settings::EvaluationParameters::pawn_psqt_adjustment[relative_rank]
                                                            [file]
                                                                .eval[0],
        -250, 250, [](const int value) {
          Settings::EvaluationParameters::SetPawnPsqtAdjustment(
              relative_rank, file, GamePhase::kMiddleGame, value);
        }));
    options.emplace_back(std::make_unique<SpinOption>(
        "PawnPSQT" + square_name + "EG",
        Settings::EvaluationParameters::pawn_psqt_adjustment[relative_rank]
                                                            [file]
                                                                .eval[1],
        -250, 250, [](const int value) {
          Settings::EvaluationParameters::SetPawnPsqtAdjustment(
              relative_rank, file, GamePhase::kEndGame, value);
        }));
  }

  template <size_t relative_rank>
  void AddPawnPsqtRankOptions(const std::string& rank_name) {
    AddPawnPsqtSquareOptions<relative_rank, 0>("A" + rank_name);
    AddPawnPsqtSquareOptions<relative_rank, 1>("B" + rank_name);
    AddPawnPsqtSquareOptions<relative_rank, 2>("C" + rank_name);
    AddPawnPsqtSquareOptions<relative_rank, 3>("D" + rank_name);
    AddPawnPsqtSquareOptions<relative_rank, 4>("E" + rank_name);
    AddPawnPsqtSquareOptions<relative_rank, 5>("F" + rank_name);
    AddPawnPsqtSquareOptions<relative_rank, 6>("G" + rank_name);
    AddPawnPsqtSquareOptions<relative_rank, 7>("H" + rank_name);
  }

  void AddKingSafetyOptions() {
    options.emplace_back(std::make_unique<SpinOption>(
        "KingShieldNearMG", Settings::EvaluationParameters::king_shield_near, 0,
        50, Settings::EvaluationParameters::SetKingShieldNear));
    options.emplace_back(std::make_unique<SpinOption>(
        "KingShieldFarMG", Settings::EvaluationParameters::king_shield_far, 0,
        50, Settings::EvaluationParameters::SetKingShieldFar));
    options.emplace_back(std::make_unique<SpinOption>(
        "KingSemiOpenFileMG",
        Settings::EvaluationParameters::king_semi_open_file, -50, 0,
        Settings::EvaluationParameters::SetKingSemiOpenFile));
    options.emplace_back(std::make_unique<SpinOption>(
        "KingOpenFileMG", Settings::EvaluationParameters::king_open_file, -50,
        0, Settings::EvaluationParameters::SetKingOpenFile));
    options.emplace_back(std::make_unique<SpinOption>(
        "KingPawnStormNearMG",
        Settings::EvaluationParameters::king_pawn_storm_near, -50, 0,
        Settings::EvaluationParameters::SetKingPawnStormNear));
    options.emplace_back(std::make_unique<SpinOption>(
        "KingPawnStormFarMG",
        Settings::EvaluationParameters::king_pawn_storm_far, -50, 0,
        Settings::EvaluationParameters::SetKingPawnStormFar));
    AddKingAttackOption<Piece::kKnight>("Knight");
    AddKingAttackOption<Piece::kBishop>("Bishop");
    AddKingAttackOption<Piece::kRook>("Rook");
    AddKingAttackOption<Piece::kQueen>("Queen");
  }

  template <Piece piece>
  void AddKingAttackOption(const std::string& piece_name) {
    options.emplace_back(std::make_unique<SpinOption>(
        piece_name + "KingAttackMG",
        Settings::EvaluationParameters::king_attack[static_cast<size_t>(piece)],
        0, 50, [](const int value) {
          Settings::EvaluationParameters::SetKingAttack<piece>(value);
        }));
  }

  SpinOption* threads_ = nullptr;
};

class UciChessEngine {
 public:
  explicit UciChessEngine(std::istream& i_stream = std::cin,
                          std::ostream& o_stream = std::cout)
      : i_stream_(i_stream), o_stream_(o_stream), search_thread_(o_stream) {}

  ~UciChessEngine();

  void Start();

 private:
  void StartSearch(bool ponder);
  void StopSearch() noexcept;

  void ParseCommand(std::stringstream command);

  void ParseUci(std::stringstream command);
  void ParseSetOption(std::stringstream command);
  void ParseIsReady(std::stringstream command) const;
  static void ParseUciNewGame(std::stringstream command);
  void ParseFen(const std::string& fen);
  void ParseStartPos();
  void ParseMoves(std::stringstream command);
  void ParsePosition(std::stringstream command);
  void ParsePerft(std::stringstream command);
  void ParseEvaluate() const;
  void ParseQuiescenceEvaluate() const;
  void ParseGo(std::stringstream command);
  void ParsePonderhit(std::stringstream command);
  void ParseMoveTime(std::stringstream command);
  void ParsePlayersTime(std::stringstream command);
  void ParseDepth(std::stringstream command);
  void ParseStop(std::stringstream command);
  void ParseQuit(std::stringstream command);
  void ParseDebug(std::stringstream command);

  void Send(const std::string& message) const {
    o_stream_ << message << std::endl;
  }

  std::istream& i_stream_;
  std::ostream& o_stream_;

  SearchThread search_thread_;

  Info info_;
  EngineOptions options_;

  bool quit_ = false;
};

inline UciChessEngine::~UciChessEngine() { StopSearch(); }

inline void UciChessEngine::StartSearch(bool ponder) {
  if (ponder) {
    return search_thread_.GoPonder();
  }
  search_thread_.Start(info_);
}

inline void UciChessEngine::StopSearch() noexcept { search_thread_.Stop(); }

inline void UciChessEngine::Start() {
  std::string command;
  while (!quit_ && std::getline(i_stream_, command)) {
    ParseCommand(std::stringstream{command});
  }
}

inline void UciChessEngine::ParseCommand(std::stringstream command) {
  std::string command_name;

  command >> command_name;

  if (command_name == "uci") {
    return ParseUci(std::move(command));
  }
  if (command_name == "isready") {
    return ParseIsReady(std::move(command));
  }
  if (command_name == "ucinewgame") {
    return ParseUciNewGame(std::move(command));
  }
  if (command_name == "position") {
    return ParsePosition(std::move(command));
  }
  if (command_name == "go") {
    return ParseGo(std::move(command));
  }
  if (command_name == "ponderhit") {
    return ParsePonderhit(std::move(command));
  }
  if (command_name == "stop") {
    return ParseStop(std::move(command));
  }
  if (command_name == "quit") {
    return ParseQuit(std::move(command));
  }
  if (command_name == "setoption") {
    return ParseSetOption(std::move(command));
  }
  if (command_name == "debug") {
    return ParseDebug(std::move(command));
  }
  if (command_name == "bench") {
    ParsePosition(std::stringstream{std::string{"startpos"}});
    const auto result = PerftBench(info_.position, 6);
    o_stream_ << result.nodes << " nodes " << result.nps << " nps" << std::endl;
    std::exit(0);
  }

  Send("No such command!");
}

inline void UciChessEngine::ParseUci(std::stringstream) {
  const std::string name = "SimpleChessEngine";
  const std::string author = "nook0110";

  Send("id name " + name);
  Send("id author " + author);

  options_.PrintOptionsNames(o_stream_);

  Send("uciok");
}

inline void UciChessEngine::ParseSetOption(std::stringstream command) {
  std::string name;
  command >> name;
  if (name != "name") {
    Send("Maybe you meant 'name'?");
    return;
  }
  if (options_.ParseSetoption(std::move(command))) {
    search_thread_.SetThreadCount(options_.GetThreadCount());
  }
}
inline void UciChessEngine::ParseIsReady(std::stringstream) const {
  // ReSharper disable once StringLiteralTypo
  Send("readyok");
}

inline void UciChessEngine::ParseUciNewGame(std::stringstream) { /**/ }

inline void UciChessEngine::ParseFen(const std::string& fen) {
  info_.position = PositionFactory{}(fen);
}

inline void UciChessEngine::ParseStartPos() {
  info_.position = PositionFactory{}();
}

inline void UciChessEngine::ParseMoves(std::stringstream command) {
  std::string move;
  while (!command.eof()) {
    command >> move;
    info_.position.DoMove(MoveFactory{}(info_.position, move));
  }
}

inline void UciChessEngine::ParsePosition(std::stringstream command) {
  std::string token;

  command >> token;

  if (token == "fen") {
    std::string board;
    std::string side_to_move;
    std::string castling_rights;
    std::string en_croissant;
    std::string rule50;
    std::string move_number;
    command >> board >> side_to_move >> castling_rights >> rule50 >>
        en_croissant >> move_number;
    const auto fen = board + " " + side_to_move + " " + castling_rights + " " +
                     en_croissant + " " + rule50 + " " + move_number;
    ParseFen(fen);
  }
  if (token == "startpos") {
    ParseStartPos();
  }

  command >> token;

  if (token == "moves") {
    ParseMoves(std::move(command));
  }
}

inline void UciChessEngine::ParsePerft(std::stringstream command) {
  std::string token;
  command >> token;
  const auto depth = std::stoull(token);

  const auto start_time = std::chrono::high_resolution_clock::now();
  const auto nodes = Perft(o_stream_, info_.position, depth);
  const auto time = std::chrono::duration<double>(
                        std::chrono::high_resolution_clock::now() - start_time)
                        .count();
  o_stream_ << "Time: " << time << " seconds" << std::endl;
  o_stream_ << NodePerSecondInfo{static_cast<size_t>(nodes / time)};
}

inline void UciChessEngine::ParseEvaluate() const {
  o_stream_ << "eval: " << info_.position.Evaluate() << " cp" << std::endl;
}

inline void UciChessEngine::ParseQuiescenceEvaluate() const {
  Position position = info_.position;
  const DepthCondition condition{kMaxSearchPly};
  Quiescence quiescence{condition};
  const auto score =
      quiescence.Search<true>(position, kMateValue, -kMateValue, 0);
  if (!score) {
    o_stream_ << "qeval unavailable" << std::endl;
    return;
  }
  const Eval white_score =
      info_.position.GetSideToMove() == Player::kWhite ? *score : -*score;
  o_stream_ << "qeval white_cp " << white_score << " nodes "
            << quiescence.GetSearchedNodes() << std::endl;
}

inline void UciChessEngine::ParseGo(std::stringstream command) {
  std::string token;
  auto startpos = command.tellg();
  command >> token;

  bool ponder = false;

  if (token == "perft") {
    ParsePerft(std::move(command));
    return;
  }

  if (token == "evaluate") {
    ParseEvaluate();
    return;
  }

  if (token == "qeval") {
    ParseQuiescenceEvaluate();
    return;
  }

  if (token == "hash") {
    o_stream_ << "hash: " << info_.position.GetHash() << std::endl;
    return;
  }

  if (token == "ponder") {
    startpos = command.tellg();
    command >> token;
    ponder = true;
  }

  if (token == "wtime" || token == "btime" || token == "winc" ||
      token == "binc") {
    command.seekg(startpos);
    ParsePlayersTime(std::move(command));
  } else if (token == "movetime") {
    ParseMoveTime(std::move(command));
  } else if (token == "depth") {
    ParseDepth(std::move(command));
  }

  StartSearch(ponder);
}

inline void UciChessEngine::ParsePonderhit(std::stringstream) {
  search_thread_.PonderHit(info_);
}

inline void UciChessEngine::ParseMoveTime(std::stringstream command) {
  std::string token;
  command >> token;
  info_.time_control =
      TimePerMove{std::chrono::milliseconds{std::stoull(token)}};
}

inline void UciChessEngine::ParsePlayersTime(std::stringstream command) {
  std::string token;

  TournamentTime tournament_time;

  while (command >> token) {
    using enum SimpleChessEngine::Player;
    std::size_t time;
    command >> time;
    if (token == "wtime") {
      tournament_time.player_time[static_cast<size_t>(kWhite)] =
          std::chrono::milliseconds{time};
    }
    if (token == "btime") {
      tournament_time.player_time[static_cast<size_t>(kBlack)] =
          std::chrono::milliseconds{time};
    }
    if (token == "winc") {
      tournament_time.player_inc[static_cast<size_t>(kWhite)] =
          std::chrono::milliseconds{time};
    }
    if (token == "binc") {
      tournament_time.player_inc[static_cast<size_t>(kBlack)] =
          std::chrono::milliseconds{time};
    }
  }

  info_.time_control = tournament_time;
}

inline void SimpleChessEngine::UciChessEngine::ParseDepth(
    std::stringstream command) {
  std::string token;
  command >> token;
  info_.time_control = MaxDepth{static_cast<Depth>(std::stoul(token))};
}

inline void UciChessEngine::ParseStop(std::stringstream) { StopSearch(); }

inline void UciChessEngine::ParseQuit(std::stringstream) {
  quit_ = true;
  StopSearch();
}

inline void UciChessEngine::ParseDebug(std::stringstream) {
  o_stream_ << FenFactory{}(info_.position) << std::endl;
}

inline SearchThread::SearchThread(Position position, std::ostream& o_stream)
    : engine_(std::move(position), o_stream, transposition_table_) {}

inline SearchThread::SearchThread(std::ostream& o_stream)
    : engine_(PositionFactory{}(), o_stream, transposition_table_) {}

inline void SearchThread::Start(const Info& info) {
  StopThread();
  engine_.SetPosition(info.position);
  stop_signal_ = std::make_shared<std::atomic_bool>(false);

  helper_engines_.clear();
  helper_threads_.clear();
  helper_engines_.reserve(thread_count_ - 1);
  helper_threads_.reserve(thread_count_ - 1);

  for (std::size_t worker = 1; worker < thread_count_; ++worker) {
    helper_engines_.push_back(std::make_unique<ChessEngine>(
        info.position, std::cout, transposition_table_, false,
        static_cast<Depth>(1 + worker % 2)));
    auto& helper = *helper_engines_.back();
    helper_threads_.emplace_back([this, info, &helper] {
      auto condition = GetCondition(info);
      std::visit(
          [&helper](auto& unwrapped_condition) {
            helper.ComputeBestMove(unwrapped_condition);
          },
          condition);
    });
  }

  thread_ = std::thread([this, info] {
    auto condition = GetCondition(info);
    std::visit(
        [this](auto& unwrapped_condition) {
          engine_.ComputeBestMove(unwrapped_condition);
        },
        condition);
  });
}

inline void SearchThread::Stop() { StopThread(); }

inline void SearchThread::StopThread() {
  if (stop_signal_) {
    stop_signal_->store(true, std::memory_order_relaxed);
  }
  if (thread_) {
    thread_->join();
    thread_ = std::nullopt;
  }
  for (auto& helper_thread : helper_threads_) {
    if (helper_thread.joinable()) helper_thread.join();
  }
  helper_threads_.clear();
  helper_engines_.clear();
  pondering_ = std::nullopt;
}
}  // namespace SimpleChessEngine
