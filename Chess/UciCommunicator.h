#pragma once
#include <functional>
#include <future>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

#include "Evaluator.h"
#include "Move.h"
#include "Position.h"
#include "SimpleChessEngine.h"

namespace SimpleChessEngine
{
class UciDebugPrinter final : public InfoPrinter
{
 public:
  explicit UciDebugPrinter(std::ostream& o_stream) : o_stream_(o_stream) {}

  void operator()(const DepthInfo& depth_info) const override
  {
    o_stream_ << "info depth " << depth_info.current_depth << std::endl;
  }

  void operator()(const ScoreInfo& score_info) const override
  {
    o_stream_ << "info score cp " << score_info.current_eval << std::endl;
  }

  void operator()(const NodePerSecondInfo& nps_info) const override
  {
    o_stream_ << "info nps " << nps_info.nodes_per_second << std::endl;
  }

 private:
  std::ostream& o_stream_;
};

class SearchThread
{
 public:
  explicit SearchThread(std::ostream& o_stream)
      : engine_(std::make_unique<UciDebugPrinter>(o_stream))
  {}

  void Start()
  {
    Stop();
    thread_ = std::thread(
        [this] { engine_.ComputeBestMove(std::chrono::seconds(1)); });
  }

  void Stop()
  {
    if (thread_ && thread_.value().joinable())
    {
      thread_.value().join();
    }
  }

 private:
  ChessEngine engine_;

  std::optional<std::thread> thread_;
};

class UciChessEngine
{
 public:
  explicit UciChessEngine(std::istream& i_stream = std::cin,
                          std::ostream& o_stream = std::cout)
      : i_stream_(i_stream), o_stream_(o_stream), search_thread_(o_stream)
  {}

  ~UciChessEngine();

  void Start();

 private:
  void StartSearch();
  void StopSearch();

  void ParseCommand(std::stringstream command);

  void ParseUci(std::stringstream command);
  void ParseIsReady(std::stringstream command);
  void ParseUciNewGame(std::stringstream command);
  void ParseFen(const std::string& fen);
  void ParsePosition(std::stringstream command);
  void ParseGo(std::stringstream command);
  void ParseStop(std::stringstream command);
  void ParseQuit(std::stringstream command);

  void SendOptions();

  void Send(const std::string& message) const
  {
    o_stream_ << message << std::endl;
  }

  std::istream& i_stream_;
  std::ostream& o_stream_;

  SearchThread search_thread_;

  bool quit_ = false;
};

inline UciChessEngine::~UciChessEngine() { StopSearch(); }

inline void UciChessEngine::StartSearch() { search_thread_.Start(); }

inline void UciChessEngine::StopSearch() { search_thread_.Stop(); }

inline void UciChessEngine::Start()
{
  std::string command;
  while (!quit_ && std::getline(i_stream_, command))
  {
    ParseCommand(std::stringstream{command});
  }
}

inline void UciChessEngine::ParseCommand(std::stringstream command)
{
  std::string command_name;

  command >> command_name;

  if (command_name == "uci")
  {
    return ParseUci(std::move(command));
  }
  if (command_name == "isready")
  {
    return ParseIsReady(std::move(command));
  }
  if (command_name == "ucinewgame")
  {
    return ParseUciNewGame(std::move(command));
  }
  if (command_name == "position")
  {
    return ParsePosition(std::move(command));
  }
  if (command_name == "go")
  {
    return ParseGo(std::move(command));
  }
  if (command_name == "stop")
  {
    return ParseStop(std::move(command));
  }
  if (command_name == "quit")
  {
    return ParseQuit(std::move(command));
  }
  Send("No such command!");
}

inline void UciChessEngine::ParseUci(std::stringstream command)
{
  const std::string name = "SimpleChessEngine";
  const std::string author = "nook0110";

  Send("id name " + name);
  Send("id author " + author);

  SendOptions();

  // ReSharper disable once StringLiteralTypo
  Send("uciok");
}

inline void UciChessEngine::ParseIsReady(std::stringstream command)
{
  // ReSharper disable once StringLiteralTypo
  Send("readyok");
}

inline void UciChessEngine::ParseUciNewGame(std::stringstream command)
{
  // ReSharper disable once StringLiteralTypo
  Send("isready");
}

inline void UciChessEngine::ParseFen(const std::string& fen) {}

inline void UciChessEngine::ParsePosition(std::stringstream command)
{
  std::string fen;
  command >> fen;

  ParseFen(fen);
}

inline void UciChessEngine::ParseGo(std::stringstream command)
{
  std::string token;
  while (command >> token)
  {
    if (token == "searchmoves")
    {
      assert(false);
    }
  }

  inline void UciChessEngine::ParseStop(std::stringstream command) {}

  inline void UciChessEngine::ParseQuit(std::stringstream command)
  {
    quit_ = true;
    StopSearch();
  }

  inline void UciChessEngine::SendOptions() {}
}  // namespace SimpleChessEngine
