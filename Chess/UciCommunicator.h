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
struct TournamentTime {
  std::array<std::chrono::milliseconds, 2> player_time = {
      std::chrono::milliseconds{0}, std::chrono::milliseconds{0}};

  std::array<std::chrono::milliseconds, 2> player_inc = {
      std::chrono::milliseconds{0}, std::chrono::milliseconds{0}};
};

struct TimePerMove {
  std::chrono::milliseconds movetime;
};

using TimeControl = std::variant<TournamentTime, TimePerMove>;

struct Info {
  Position position;
  TimeControl time_control;
};

class SearchThread {
 public:
  explicit SearchThread(Position position, std::ostream& o_stream);
  explicit SearchThread(std::ostream& o_stream);

  void Start(const Info& info);
  void Stop();

 private:
  void StopThread();

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
  o_stream_ << NodePerSecondInfo{static_cast<size_t>(nodes / time)};
}

inline void UciChessEngine::ParseEvaluate() const {
  o_stream_ << "eval: " << info_.position.Evaluate() << " cp" << std::endl;
}

inline void UciChessEngine::ParseGo(std::stringstream command) {
  std::string token;
  auto startpos = command.tellg();
  command >> token;

  if (token == "perft") {
    ParsePerft(std::move(command));
    return;
  }

  if (token == "evaluate") {
    ParseEvaluate();
    return;
  }

  if (token == "wtime" || token == "btime" || token == "winc" ||
      token == "binc") {
    command.seekg(startpos);
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
  info_.time_control =
      TimePerMove{std::chrono::milliseconds{std::stoull(token)}};
}

inline void UciChessEngine::ParsePlayersTime(std::stringstream command) {
  std::string token;

  TournamentTime tournament_time;

  while (command >> token) {
    std::size_t time;
    command >> time;
    if (token == "wtime") {
      tournament_time.player_time[static_cast<size_t>(Player::kWhite)] =
          std::chrono::milliseconds{time};
    }
    if (token == "btime") {
      tournament_time.player_time[static_cast<size_t>(Player::kBlack)] =
          std::chrono::milliseconds{time};
    }
    if (token == "winc") {
      tournament_time.player_inc[static_cast<size_t>(Player::kWhite)] =
          std::chrono::milliseconds{time};
    }
    if (token == "binc") {
      tournament_time.player_inc[static_cast<size_t>(Player::kBlack)] =
          std::chrono::milliseconds{time};
    }
  }

  info_.time_control = tournament_time;
}

inline void UciChessEngine::ParseStop(std::stringstream command) {
  StopSearch();
}

inline void UciChessEngine::ParseQuit(std::stringstream command) {
  quit_ = true;
  StopSearch();
}

SearchThread::SearchThread(Position position, std::ostream& o_stream)
    : engine_(std::move(position), o_stream) {}

SearchThread::SearchThread(std::ostream& o_stream)
    : engine_(PositionFactory{}(), o_stream) {}

void SearchThread::Start(const Info& info) {
  StopThread();
  engine_.SetPosition(info.position);

  thread_ = std::thread([this, &info] {
    if (const auto tournament =
            std::get_if<TournamentTime>(&info.time_control)) {
      engine_.ComputeBestMove(
          std::chrono::milliseconds(tournament->player_time[static_cast<size_t>(
              info.position.GetSideToMove())]),
          std::chrono::milliseconds(tournament->player_inc[static_cast<size_t>(
              info.position.GetSideToMove())]));
    }
  });
}

void SearchThread::Stop() {
  engine_.PrintBestMove();
  StopThread();
}

void SearchThread::StopThread() {
  if (thread_) {
    thread_->join();
    thread_ = std::nullopt;
  }
}
}  // namespace SimpleChessEngine
