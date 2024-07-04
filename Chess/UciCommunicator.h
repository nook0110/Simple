#pragma once

#include <future>
#include <iostream>
#include <optional>
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
    pondering_ = Pondering{};
    thread_ = std::thread([this] { engine_.GoPonder(*pondering_); });
  }

  void Stop();

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

      return TimeCondition{time_for_move};
    }
    if (const auto time_per_move =
            std::get_if<TimePerMove>(&info.time_control)) {
      return TimeCondition{time_per_move->movetime};
    }
    if (const auto max_depth = std::get_if<MaxDepth>(&info.time_control)) {
      return DepthCondition{max_depth->depth};
    }
    assert(false);
  }

  void StopThread();

  ChessEngine engine_;

  std::optional<Pondering> pondering_;
  std::optional<std::thread> thread_;
};

struct OptionBase {
  OptionBase(std::string name) : name_(std::move(name)) {}

  virtual bool SetValue(const std::string& value) = 0;

  const std::string& GetName() const { return name_; }

  virtual std::string GetOptionDescription() const = 0;

 private:
  std::string name_;
};

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
    return "type check default false";
  }

 private:
  bool value_ = false;
};

using PonderOption = BooleanOption;
struct EngineOptions {
  EngineOptions() {
    options.emplace_back(std::make_unique<PonderOption>("Ponder"));
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
  }

  void PrintOptionsNames(std::ostream& out) {
    for (const auto& option : options) {
      out << "option name " << option->GetName() << ' '
          << option->GetOptionDescription() << '\n';
    }
  }
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
  void StopSearch();

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
  void ParseGo(std::stringstream command);
  void ParsePonderhit(std::stringstream command);
  void ParseMoveTime(std::stringstream command);
  void ParsePlayersTime(std::stringstream command);
  void ParseDepth(std::stringstream command);
  void ParseStop(std::stringstream command);
  void ParseQuit(std::stringstream command);

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

  Send("No such command!");
}

inline void UciChessEngine::ParseUci(std::stringstream) {
  const std::string name = "SimpleChessEngine";
  const std::string author = "nook0110";

  Send("id name " + name);
  Send("id author " + author);

  options_.PrintOptionsNames(o_stream_);

  // ReSharper disable once StringLiteralTypo
  Send("uciok");
}

inline void UciChessEngine::ParseSetOption(std::stringstream command) {
  std::string name;
  command >> name;
  if (name != "name") {
    Send("Maybe you meant 'name'?");
    return;
  }
  options_.ParseSetoption(std::move(command));
}
inline void UciChessEngine::ParseIsReady(std::stringstream) const {
  // ReSharper disable once StringLiteralTypo
  Send("readyok");
}

inline void UciChessEngine::ParseUciNewGame(std::stringstream) { /**/
}

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
  info_.time_control = MaxDepth{std::stoull(token)};
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
    auto condition = GetCondition(info);
    std::visit(
        [this](auto& unwrapped_condition) {
          engine_.ComputeBestMove(unwrapped_condition);
        },
        condition);
  });
}

void SearchThread::Stop() {
  if (!pondering_) engine_.PrintBestMove();
  StopThread();
}

void SearchThread::StopThread() {
  if (thread_) {
    if (pondering_) {
      pondering_->pondermiss = true;
    }
    thread_->join();
    thread_ = std::nullopt;
    pondering_ = std::nullopt;
  }
}
}  // namespace SimpleChessEngine
