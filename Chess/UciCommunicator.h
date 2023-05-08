#pragma once
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

#include "Evaluator.h"
#include "Move.h"
#include "Position.h"

namespace SimpleChessEngine
{
class UciCommunicator
{
 public:
  explicit UciCommunicator(std::istream& i_stream = std::cin,
                           std::ostream& o_stream = std::cout)
      : i_stream_(i_stream), o_stream_(o_stream)
  {}

  void Start() { io_thread_.detach(); }

 private:
  void InputOutputStream();

  void ParseCommand(std::stringstream command);

  void ParseUci(std::stringstream command);
  void ParseDebug(std::stringstream command);
  void ParseIsReady(std::stringstream command);
  void ParseSetOption(std::stringstream command);
  void ParseUciNewGame(std::stringstream command);
  void ParseFen(const std::string& fen);
  void ParsePosition(std::stringstream command);
  void ParseGo(std::stringstream command);
  void ParseStop(std::stringstream command);

  void SendOptions();

  void Send(const std::string& message) const
  {
    o_stream_ << message << std::endl;
  }

  std::istream& i_stream_;
  std::ostream& o_stream_;

  std::thread io_thread_{[this] {}};
};

inline void UciCommunicator::InputOutputStream()
{
  std::string command;
  while (std::getline(i_stream_, command))
  {
    ParseCommand(std::stringstream{command});
  }
}

inline void UciCommunicator::ParseCommand(std::stringstream command)
{
  std::string command_name;

  command >> command_name;

  Send("Parsing command:" + command_name);

  if (command_name == "uci")
  {}
}

inline void UciCommunicator::ParseUci(std::stringstream command)
{
  const std::string name = "SimpleChessEngine";
  const std::string author = "nook0110";

  Send("id name " + name + "\n");
  Send("id author " + author + "\n");

  SendOptions();

  // ReSharper disable once StringLiteralTypo
  Send("uciok");
}

inline void UciCommunicator::ParseDebug(std::stringstream command)
{
  assert(false);
}

inline void UciCommunicator::ParseIsReady(std::stringstream command)
{
  // ReSharper disable once StringLiteralTypo
  Send("readyok");
}

inline void UciCommunicator::ParseSetOption(std::stringstream command)
{
  assert(false);
}

inline void UciCommunicator::ParseUciNewGame(std::stringstream command)
{
  // ReSharper disable once StringLiteralTypo
  Send("isready");
}

inline void UciCommunicator::ParsePosition(std::stringstream command)
{
  std::string fen;
  command >> fen;

  ParseFen(fen);
}

inline void UciCommunicator::ParseGo(std::stringstream command) {}

inline void UciCommunicator::ParseStop(std::stringstream command) {}
}  // namespace SimpleChessEngine
