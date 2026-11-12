#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <system_error>
#include <vector>

#include "Nnue.h"
#include "Stockfish/attacks.h"
#include "Stockfish/nnue/nnue_feature_transformer.h"
#include "Stockfish/position.h"

namespace SimpleChessEngine::Nnue {
namespace {
namespace SF = Stockfish::Eval::NNUE;

struct Network {
  SF::FeatureTransformer featureTransformer;
  SF::NetworkArchitecture network[SF::LayerStacks];
  static constexpr std::uint32_t hash =
      SF::FeatureTransformer::get_hash_value() ^
      SF::NetworkArchitecture::get_hash_value();

  bool Read(std::istream& stream) {
    if (SF::read_little_endian<std::uint32_t>(stream) !=
            SF::FeatureTransformer::get_hash_value() ||
        !featureTransformer.read_parameters(stream))
      return false;
    for (auto& layer : network) {
      if (SF::read_little_endian<std::uint32_t>(stream) !=
              SF::NetworkArchitecture::get_hash_value() ||
          !layer.read_parameters(stream))
        return false;
    }
    return stream && stream.peek() == std::ios::traits_type::eof();
  }

  NetworkOutput Evaluate(const Position& position, SF::AccumulatorStack& stack,
                         SF::AccumulatorCaches& cache) const {
    alignas(SF::CacheLineSize) SF::TransformedFeatureType
        transformed[SF::FeatureTransformer::BufferSize];
    SF::NNZInfo<SF::L1> nnz;
    const Stockfish::Position pos{position};
    const int bucket = (pos.count<Stockfish::ALL_PIECES>() - 1) / 4;
    const auto psqt = featureTransformer.transform(pos, stack, cache,
                                                   transformed, bucket, nnz);
    const auto positional = network[bucket].propagate(transformed, nnz);
    return {psqt / SF::OutputScale, positional / SF::OutputScale};
  }
};

std::unique_ptr<Network> network;
std::uint32_t generation = 0;
std::string description;

SF::AccumulatorCaches& Cache() {
  thread_local std::unique_ptr<SF::AccumulatorCaches> cache;
  thread_local std::uint32_t cache_generation = 0;
  if (!cache) cache = std::make_unique<SF::AccumulatorCaches>(*network);
  if (cache_generation != generation) {
    cache->clear(*network);
    cache_generation = generation;
  }
  return *cache;
}

std::vector<std::filesystem::path> CandidatePaths(const std::string& path) {
  std::vector<std::filesystem::path> candidates{path};
  if (std::filesystem::path{path}.is_relative()) {
    std::error_code error;
    const auto executable =
        std::filesystem::read_symlink("/proc/self/exe", error);
    if (!error) candidates.push_back(executable.parent_path() / path);
  }
  return candidates;
}
}  // namespace

bool Load(const std::string& path) {
  for (const auto& candidate : CandidatePaths(path)) {
    std::ifstream stream(candidate, std::ios::binary);
    if (!stream) continue;
    const auto version = SF::read_little_endian<std::uint32_t>(stream);
    const auto hash = SF::read_little_endian<std::uint32_t>(stream);
    const auto description_size = SF::read_little_endian<std::uint32_t>(stream);
    if (!stream || version != SF::Version || hash != Network::hash ||
        description_size > 1024 * 1024)
      return false;
    stream.ignore(description_size);
    if (!stream) return false;
    auto replacement = std::make_unique<Network>();
    if (!replacement->Read(stream)) return false;
    static const bool initialized = [] {
      Stockfish::Attacks::init();
      return true;
    }();
    (void)initialized;
    network = std::move(replacement);
    description =
        candidate.string() +
        " (Stockfish 19, FullThreats+PP_3Wide+HalfKAv2_hm, 1024->32->32->1)";
    ++generation;
    return true;
  }
  return false;
}

void Unload() {
  network.reset();
  description.clear();
  ++generation;
}

bool IsLoaded() { return bool(network); }
std::string Description() { return description; }
std::uint32_t Generation() { return generation; }
// round(82 * 1024 / a(1)), with Stockfish 19's WDL anchor a(1) = 304.15540371.
int DefaultScale() { return 276; }

int PostProcess(const Position& position, const NetworkOutput output) {
  std::int64_t nnue = output.psqt + output.positional;
  const int complexity = std::abs(output.psqt - output.positional);
  nnue -= nnue * complexity / 18236;
  const Stockfish::Position pos{position};
  const int material = 534 * pos.count<Stockfish::PAWN>() +
                       Stockfish::KnightValue * pos.count<Stockfish::KNIGHT>() +
                       Stockfish::BishopValue * pos.count<Stockfish::BISHOP>() +
                       Stockfish::RookValue * pos.count<Stockfish::ROOK>() +
                       Stockfish::QueenValue * pos.count<Stockfish::QUEEN>();
  int value = static_cast<int>(nnue + nnue * material / 91000);
  value -= value * static_cast<int>(position.GetHalfMoveClock()) / 199;
  return std::clamp(value, Stockfish::VALUE_TB_LOSS_IN_MAX_PLY + 1,
                    Stockfish::VALUE_TB_WIN_IN_MAX_PLY - 1);
}

NetworkOutput Evaluate(const Position& position, AccumulatorStack& stack) {
  if (!network) return {};
  stack.Synchronize(generation);
  auto& output = stack.Output();
  if (output.valid) return {output.psqt, output.positional};
  const auto result = network->Evaluate(position, stack.Native(), Cache());
  output = {true, result.psqt, result.positional};
  return result;
}

NetworkOutput EvaluateFromScratch(const Position& position) {
  if (!network) return {};
  SF::AccumulatorStack stack;
  SF::AccumulatorCaches cache{*network};
  return network->Evaluate(position, stack, cache);
}

bool VerifyAccumulators(const Position& position, AccumulatorStack& stack) {
  if (!network) return true;
  (void)Evaluate(position, stack);
  SF::AccumulatorStack scratch;
  SF::AccumulatorCaches cache{*network};
  (void)network->Evaluate(position, scratch, cache);
  const auto& actual = stack.Native().latest();
  const auto& expected = scratch.latest();
  return actual.computed == expected.computed &&
         actual.accumulation == expected.accumulation &&
         actual.psqtAccumulation == expected.psqtAccumulation;
}
}  // namespace SimpleChessEngine::Nnue
