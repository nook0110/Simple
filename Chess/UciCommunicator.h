#pragma once
#include <future>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <variant>

#include "MoveFactory.h"
#include "Perft.h"
#include "Position.h"
#include "SimpleChessEngine.h"
#include "StreamUtility.h"

namespace SimpleChessEngine {
struct Info {
  Position position;
  ChessEngine::SearchTimeInfo info;
};

class UciDebugPrinter final : public InfoPrinter {
 public:
  explicit UciDebugPrinter(std::ostream& o_stream) : o_stream_(o_stream) {}

  void operator()(const DepthInfo& depth_info) const override {
    o_stream_ << "info depth " << depth_info.current_depth << std::endl;
  }

  void operator()(const ScoreInfo& score_info) const override {
    const auto& eval = score_info.current_eval;
    if (!IsMateScore(eval)) {
      o_stream_ << "info score cp " << eval << std::endl;
      return;
    }

    o_stream_ << "info score mate "
              << IsMateScore(eval) * (-kMateValue - std::abs(eval))
              << std::endl;
  }

  void operator()(const NodesInfo& nodes_info) const override {
    o_stream_ << "info nodes " << nodes_info.nodes << std::endl;
  }

  void operator()(const NodePerSecondInfo& nps_info) const override {
    o_stream_ << "info nps " << nps_info.nodes_per_second << std::endl;
  }

  void operator()(
      const PrincipalVariationInfo& principal_variation) const override {
    o_stream_ << "info depth " << principal_variation.current_depth << " pv";
    for (const auto move : principal_variation.best_moves) {
      o_stream_ << " ";
      std::visit(
          [this](const auto& unwrapped_move) { o_stream_ << unwrapped_move; },
          move);
    }
    o_stream_ << std::endl;
  }

  void operator()(
      const PrincipalVariationHitsInfo& pv_hits_info) const override {
    o_stream_ << "info hits " << pv_hits_info.pv_hits << std::endl;
  }

  void operator()(const BestMoveInfo& best_move) const override {
    o_stream_ << "bestmove ";
    std::visit([this](const auto& move) { o_stream_ << move; }, best_move.move);
    o_stream_ << std::endl;
  }

  void operator()(const EBFInfo& ebf) const override {
    o_stream_ << "info last ebf " << ebf.last_ebf << std::endl;
    o_stream_ << "info avg odd-even-ebf " << ebf.avg_odd_even_ebf << std::endl;
    o_stream_ << "info avg ebf " << ebf.avg_ebf << std::endl;
  }

 private:
  std::ostream& o_stream_;
};

class SearchThread {
 public:
  explicit SearchThread(const Position position, std::ostream& o_stream)
      : engine_(position, std::make_unique<UciDebugPrinter>(o_stream)) {}

  explicit SearchThread(std::ostream& o_stream)
      : engine_(PositionFactory{}(),
                std::make_unique<UciDebugPrinter>(o_stream)) {}

  void Start(const Info& info) {
    StopThread();
    engine_.SetPosition(info.position);

    thread_ = std::thread([this, &info] {
      engine_.ComputeBestMove(
          std::chrono::milliseconds(info.info.player_time[static_cast<size_t>(
              info.position.GetSideToMove())]),
          std::chrono::milliseconds(info.info.player_inc[static_cast<size_t>(
              info.position.GetSideToMove())]));
    });
  }

  void Stop() {
    engine_.PrintBestMove();
    StopThread();
  }

 private:
  void StopThread() {
    if (thread_ && thread_.value().joinable()) {
      thread_.value().join();
    }
  }

  ChessEngine engine_;

  std::optional<std::thread> thread_;
};

class UciChessEngine {
 public:
  explicit UciChessEngine(std::istream& i_stream = std::cin,
                          std::ostream& o_stream = std::cout)
      : i_stream_(i_stream),
        o_stream_(o_stream),
        search_thread_(o_stream),
        info_() {}

  ~UciChessEngine();

  void Start();

 private:
  void StartSearch();
  void StopSearch();

  void ParseCommand(std::stringstream command);

  void ParseUci(std::stringstream command);
  void ParseIsReady(std::stringstream command) const;
  static void ParseUciNewGame(std::stringstream command);
  void ParseFen(const std::string& fen);
  void ParseStartPos();
  void ParseMoves(std::stringstream command);
  void ParsePosition(std::stringstream command);
  void ParsePerft(std::stringstream command);
  void ParseEvaluate() const;
  void ParseGo(std::stringstream command);
  void ParseMoveTime(std::stringstream command);
  void ParsePlayersTime(std::stringstream command);
  void ParseStop(std::stringstream command);
  void ParseQuit(std::stringstream command);

  void Send(const std::string& message) const {
    o_stream_ << message << std::endl;
  }

  std::istream& i_stream_;
  std::ostream& o_stream_;

  SearchThread search_thread_;

  Info info_;

  bool quit_ = false;
};

inline UciChessEngine::~UciChessEngine() { StopSearch(); }

inline void UciChessEngine::StartSearch() { search_thread_.Start(info_); }

inline void UciChessEngine::StopSearch() { search_thread_.Stop(); }

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
  if (command_name == "stop") {
    return ParseStop(std::move(command));
  }
  if (command_name == "quit") {
    return ParseQuit(std::move(command));
  }
  Send("No such command!");
}

inline void UciChessEngine::ParseUci(std::stringstream command) {
  const std::string name = "SimpleChessEngine";
  const std::string author = "nook0110";

  Send("id name " + name);
  Send("id author " + author);

  // ReSharper disable once StringLiteralTypo
  Send("uciok");
}

inline void UciChessEngine::ParseIsReady(std::stringstream command) const {
  // ReSharper disable once StringLiteralTypo
  Send("readyok");
}

inline void UciChessEngine::ParseUciNewGame(std::stringstream command) {}

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
    std::string board, side_to_move, castling_rights, en_croissant, rule50,
        move_number;
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
  UciDebugPrinter{o_stream_}(
      NodePerSecondInfo{static_cast<size_t>(nodes / time)});
}

inline void UciChessEngine::ParseEvaluate() const {
  o_stream_ << "eval: " << info_.position.Evaluate() << " cp" << std::endl;
}

inline void UciChessEngine::ParseGo(std::stringstream command) {
  std::string token;
  command >> token;

  if (token == "perft") {
    ParsePerft(std::move(command));
    return;
  }

  if (token == "evaluate") {
    ParseEvaluate();
    return;
  }

  if (token == "wtime") {
    ParsePlayersTime(std::move(command));
  }
  if (token == "movetime") {
    ParseMoveTime(std::move(command));
  }

  StartSearch();
}

inline void UciChessEngine::ParseMoveTime(std::stringstream command) {
  std::string token;
  command >> token;
  const auto time = std::stoi(token);

  info_.info.player_time[static_cast<size_t>(info_.position.GetSideToMove())] =
      time;
}

inline void UciChessEngine::ParsePlayersTime(std::stringstream command) {
  auto cur_player = Player::kWhite;

  std::string token;

  std::array<size_t, 2>* cur_change = &info_.info.player_time;
  while (command >> token) {
    if (token == "wtime") {
      cur_change = &info_.info.player_time;
      cur_player = Player::kWhite;
      continue;
    }
    if (token == "btime") {
      cur_change = &info_.info.player_time;
      cur_player = Player::kBlack;
      continue;
    }
    if (token == "winc") {
      cur_change = &info_.info.player_inc;
      cur_player = Player::kWhite;
      continue;
    }
    if (token == "binc") {
      cur_change = &info_.info.player_inc;
      cur_player = Player::kBlack;
      continue;
    }

    const auto time = std::stoi(token);
    (*cur_change)[static_cast<size_t>(cur_player)] = time;
  }
}

inline void UciChessEngine::ParseStop(std::stringstream command) {
  StopSearch();
}

inline void UciChessEngine::ParseQuit(std::stringstream command) {
  quit_ = true;
  StopSearch();
}
}  // namespace SimpleChessEngine
