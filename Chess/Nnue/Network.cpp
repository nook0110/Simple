#include <array>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <system_error>
#include <vector>

#include "../Position.h"
#include "Accumulator.h"
#include "Architecture.h"
#include "Common.h"
#include "FeatureTransformer.h"
#include "Features.h"
#include "Nnue.h"

namespace SimpleChessEngine::Nnue {
namespace {
constexpr std::array kPieces = {Piece::kPawn, Piece::kKnight, Piece::kBishop,
                                Piece::kRook, Piece::kQueen,  Piece::kKing};
constexpr std::array kPlayers = {Player::kWhite, Player::kBlack};

template <IndexType kHalfDimensions>
struct RefreshCache {
  std::array<std::array<RefreshEntry<kHalfDimensions>, 2>, 64> entries;
  std::uint32_t generation = 0;
};

template <IndexType kHalfDimensions>
RefreshCache<kHalfDimensions>& GetRefreshCache(
    const FeatureTransformer<kHalfDimensions>& transformer,
    const std::uint32_t generation) {
  thread_local std::unique_ptr<RefreshCache<kHalfDimensions>> cache;
  if (!cache) cache.reset(new RefreshCache<kHalfDimensions>);
  if (cache->generation != generation) {
    for (auto& square_entries : cache->entries) {
      for (auto& entry : square_entries) transformer.ClearEntry(entry);
    }
    cache->generation = generation;
  }
  return *cache;
}

template <IndexType kL1, int kL2, int kL3>
class Network {
 public:
  using Transformer = FeatureTransformer<kL1>;
  using Architecture = NetworkArchitecture<kL1, kL2, kL3>;

  static constexpr std::uint32_t kHash =
      Transformer::GetHashValue() ^ Architecture::GetHashValue();

  bool ReadParameters(std::istream& stream) {
    transformer_.reset(new Transformer);
    stacks_.reset(new Architecture[kLayerStacks]);
    if (ReadLittleEndian<std::uint32_t>(stream) !=
            Transformer::GetHashValue() ||
        !transformer_->ReadParameters(stream)) {
      return false;
    }
    for (IndexType i = 0; i < kLayerStacks; ++i) {
      if (ReadLittleEndian<std::uint32_t>(stream) !=
              Architecture::GetHashValue() ||
          !stacks_[i].ReadParameters(stream)) {
        return false;
      }
    }
    return stream && stream.peek() == std::ios::traits_type::eof();
  }

  NetworkOutput Evaluate(const Position& position, AccumulatorStack& stack,
                         const std::uint32_t generation) const {
    auto& accumulators = stack.template Accumulators<kL1>();
    auto& cache = GetRefreshCache(*transformer_, generation);
    UpdateSide<Player::kWhite>(position, stack, accumulators, cache);
    UpdateSide<Player::kBlack>(position, stack, accumulators, cache);
    return Propagate(position, accumulators[stack.Size() - 1]);
  }

  NetworkOutput EvaluateFromScratch(const Position& position) const {
    auto accumulator = std::make_unique<Accumulator<kL1>>();
    RefreshFromScratch<Player::kWhite>(position, *accumulator);
    RefreshFromScratch<Player::kBlack>(position, *accumulator);
    return Propagate(position, *accumulator);
  }

 private:
  NetworkOutput Propagate(const Position& position,
                          const Accumulator<kL1>& accumulator) const {
    const int bucket =
        static_cast<int>((position.GetAllPieces().Count() - 1) / 4);
    alignas(kCacheLineSize) TransformedFeatureType transformed[kL1];
    alignas(kCacheLineSize) std::uint16_t nonzero[kL1 / 4];
    IndexType nonzero_count;
    const std::int32_t psqt = transformer_->Transform(
        accumulator, position.GetSideToMove(), bucket, transformed, nonzero,
        nonzero_count);
    const std::int32_t positional =
        stacks_[bucket].Propagate(transformed, nonzero, nonzero_count);
    return {psqt / kOutputScale, positional / kOutputScale};
  }

  template <Player kPerspective>
  void UpdateSide(const Position& position, const AccumulatorStack& stack,
                  std::vector<Accumulator<kL1>>& accumulators,
                  RefreshCache<kL1>& cache) const {
    constexpr auto perspective = static_cast<std::size_t>(kPerspective);
    const std::size_t top = stack.Size() - 1;
    std::size_t last = top;
    while (last > 0 && !accumulators[last].computed[perspective] &&
           !Features::RequiresRefresh(stack.Dirty(last), kPerspective)) {
      --last;
    }
    const BitIndex king_square = position.GetKingSquare(kPerspective);
    if (accumulators[last].computed[perspective]) {
      for (std::size_t next = last + 1; next <= top; ++next) {
        UpdateIncremental<kPerspective, true>(king_square, accumulators[next],
                                              accumulators[next - 1],
                                              stack.Dirty(next));
      }
      return;
    }
    Refresh<kPerspective>(position, accumulators[top], cache);
    for (std::size_t next = top; next > last; --next) {
      UpdateIncremental<kPerspective, false>(
          king_square, accumulators[next - 1], accumulators[next],
          stack.Dirty(next));
    }
  }

  template <Player kPerspective, bool kForward>
  void UpdateIncremental(const BitIndex king_square, Accumulator<kL1>& target,
                         const Accumulator<kL1>& source,
                         const DirtyPiece& dirty) const {
    constexpr auto perspective = static_cast<std::size_t>(kPerspective);
    Features::ChangedIndexList removed;
    Features::ChangedIndexList added;
    Features::AppendChangedIndices<kPerspective, kForward>(king_square, dirty,
                                                           removed, added);
    transformer_->ApplyUpdate(
        source.accumulation[perspective], target.accumulation[perspective],
        source.psqt_accumulation[perspective],
        target.psqt_accumulation[perspective], removed, added);
    target.computed[perspective] = true;
  }

  template <Player kPerspective>
  void Refresh(const Position& position, Accumulator<kL1>& accumulator,
               RefreshCache<kL1>& cache) const {
    constexpr auto perspective = static_cast<std::size_t>(kPerspective);
    const BitIndex king_square = position.GetKingSquare(kPerspective);
    auto& entry = cache.entries[king_square][perspective];
    Features::IndexList removed;
    Features::IndexList added;
    for (const Player color : kPlayers) {
      const auto color_index = static_cast<std::size_t>(color);
      for (const Piece piece : kPieces) {
        const auto piece_index = static_cast<std::size_t>(piece);
        const Bitboard old_pieces{entry.by_color[color_index] &
                                  entry.by_type[piece_index]};
        const Bitboard new_pieces = position.GetPiecesOf(color, piece);
        Bitboard to_remove = old_pieces & ~new_pieces;
        Bitboard to_add = new_pieces & ~old_pieces;
        while (to_remove.Any()) {
          removed.push_back(Features::MakeIndex<kPerspective>(
              to_remove.PopFirstBit(), piece, color, king_square));
        }
        while (to_add.Any()) {
          added.push_back(Features::MakeIndex<kPerspective>(
              to_add.PopFirstBit(), piece, color, king_square));
        }
      }
    }
    transformer_->ApplyRefresh(entry, removed, added,
                               accumulator.accumulation[perspective],
                               accumulator.psqt_accumulation[perspective]);
    accumulator.computed[perspective] = true;
    for (const Player color : kPlayers) {
      entry.by_color[static_cast<std::size_t>(color)] =
          static_cast<std::uint64_t>(position.GetPieces(color));
    }
    for (const Piece piece : kPieces) {
      entry.by_type[static_cast<std::size_t>(piece)] =
          static_cast<std::uint64_t>(
              position.GetPiecesOf(Player::kWhite, piece) |
              position.GetPiecesOf(Player::kBlack, piece));
    }
  }

  template <Player kPerspective>
  void RefreshFromScratch(const Position& position,
                          Accumulator<kL1>& accumulator) const {
    constexpr auto perspective = static_cast<std::size_t>(kPerspective);
    const BitIndex king_square = position.GetKingSquare(kPerspective);
    auto& values = accumulator.accumulation[perspective];
    auto& psqt = accumulator.psqt_accumulation[perspective];
    for (IndexType i = 0; i < kL1; ++i) {
      values[i] = static_cast<BiasType>(transformer_->biases[i]);
    }
    for (IndexType i = 0; i < kPsqtBuckets; ++i) psqt[i] = 0;
    Bitboard pieces = position.GetAllPieces();
    while (pieces.Any()) {
      const BitIndex square = pieces.PopFirstBit();
      const Player color = position.GetPieces(Player::kWhite).Test(square)
                               ? Player::kWhite
                               : Player::kBlack;
      const IndexType index = Features::MakeIndex<kPerspective>(
          square, position.GetPieceAt(square), color, king_square);
      for (IndexType i = 0; i < kL1; ++i) {
        values[i] = static_cast<BiasType>(
            values[i] + transformer_->weights[index * kL1 + i]);
      }
      for (IndexType i = 0; i < kPsqtBuckets; ++i) {
        psqt[i] += transformer_->psqt_weights[index * kPsqtBuckets + i];
      }
    }
    accumulator.computed[perspective] = true;
  }

  std::unique_ptr<Transformer> transformer_;
  std::unique_ptr<Architecture[]> stacks_;
};

constexpr int kSmallDefaultScale = 362;
constexpr int kBigDefaultScale = 311;
constexpr int kSmallMaterialThreshold =
    962 * kPieceValues[static_cast<std::size_t>(Piece::kPawn)].eval[0] / 208;
constexpr int kSmallFallbackThresholdNumerator = 236 * kBigDefaultScale;

using SmallNetwork = Network<kSmallDimensions, kSmallL2, kSmallL3>;
using BigNetwork = Network<kBigDimensions, kBigL2, kBigL3>;

enum class NetworkType : std::uint8_t { kNone, kSmall, kBig };

struct Registry {
  NetworkType type = NetworkType::kNone;
  std::unique_ptr<SmallNetwork> small;
  std::unique_ptr<BigNetwork> big;
  std::unique_ptr<SmallNetwork> auxiliary_small;
  std::string description;
  std::string small_description;
  std::uint32_t generation = 0;
};

Registry registry;

std::vector<std::filesystem::path> CandidatePaths(const std::string& path) {
  std::vector<std::filesystem::path> candidates{path};
  const std::filesystem::path requested{path};
  if (requested.is_relative()) {
    std::error_code error;
    const auto executable =
        std::filesystem::read_symlink("/proc/self/exe", error);
    if (!error) candidates.push_back(executable.parent_path() / requested);
  }
  return candidates;
}

template <typename NetworkClass>
bool ReadNetwork(std::istream& stream, std::unique_ptr<NetworkClass>& target) {
  auto network = std::make_unique<NetworkClass>();
  if (!network->ReadParameters(stream)) return false;
  target = std::move(network);
  return true;
}

template <typename EvaluateNetwork>
NetworkOutput SelectNetwork(const Position& position,
                            EvaluateNetwork evaluate_network) {
  if (registry.type == NetworkType::kSmall) {
    return evaluate_network(*registry.small);
  }
  if (registry.type != NetworkType::kBig) return {};
  if (registry.auxiliary_small &&
      std::abs(position.SimpleMaterialEval()) > kSmallMaterialThreshold) {
    const auto output = evaluate_network(*registry.auxiliary_small);
    const int nnue_numerator =
        (output.psqt + output.positional) * kBigDefaultScale;
    if (std::abs(nnue_numerator) >= kSmallFallbackThresholdNumerator) {
      return output;
    }
  }
  return evaluate_network(*registry.big);
}
}  // namespace

bool Load(const std::string& path) {
  for (const auto& candidate : CandidatePaths(path)) {
    std::ifstream stream(candidate, std::ios::binary);
    if (!stream) continue;
    const auto version = ReadLittleEndian<std::uint32_t>(stream);
    const auto hash = ReadLittleEndian<std::uint32_t>(stream);
    const auto description_size = ReadLittleEndian<std::uint32_t>(stream);
    if (!stream || version != kVersion) return false;
    std::string description(description_size, '\0');
    stream.read(description.data(), description_size);
    if (!stream) return false;

    if (hash == SmallNetwork::kHash) {
      if (!ReadNetwork(stream, registry.small)) return false;
      registry.big.reset();
      registry.type = NetworkType::kSmall;
      registry.description = candidate.string() + " (HalfKAv2_hm, 22528->" +
                             std::to_string(kSmallDimensions) + "x2->" +
                             std::to_string(kSmallL2) + "->" +
                             std::to_string(kSmallL3) + "->1)";
    } else if (hash == BigNetwork::kHash) {
      if (!ReadNetwork(stream, registry.big)) return false;
      registry.small.reset();
      registry.type = NetworkType::kBig;
      registry.description = candidate.string() + " (HalfKAv2_hm, 22528->" +
                             std::to_string(kBigDimensions) + "x2->" +
                             std::to_string(kBigL2) + "->" +
                             std::to_string(kBigL3) + "->1)";
    } else {
      return false;
    }
    ++registry.generation;
    return true;
  }
  return false;
}

bool LoadSmall(const std::string& path) {
  for (const auto& candidate : CandidatePaths(path)) {
    std::ifstream stream(candidate, std::ios::binary);
    if (!stream) continue;
    const auto version = ReadLittleEndian<std::uint32_t>(stream);
    const auto hash = ReadLittleEndian<std::uint32_t>(stream);
    const auto description_size = ReadLittleEndian<std::uint32_t>(stream);
    if (!stream || version != kVersion || hash != SmallNetwork::kHash) {
      return false;
    }
    stream.ignore(description_size);
    if (!stream || !ReadNetwork(stream, registry.auxiliary_small)) return false;
    registry.small_description = candidate.string();
    ++registry.generation;
    return true;
  }
  return false;
}

void UnloadSmall() {
  registry.auxiliary_small.reset();
  registry.small_description.clear();
  ++registry.generation;
}

void Unload() {
  registry.type = NetworkType::kNone;
  registry.description.clear();
  UnloadSmall();
}

bool IsLoaded() { return registry.type != NetworkType::kNone; }

std::string Description() {
  return registry.description +
         (registry.type == NetworkType::kBig && registry.auxiliary_small
              ? " + " + registry.small_description
              : "");
}

std::uint32_t Generation() { return registry.generation; }

int DefaultScale() {
  return registry.type == NetworkType::kBig ? kBigDefaultScale
                                            : kSmallDefaultScale;
}

NetworkOutput Evaluate(const Position& position, AccumulatorStack& stack) {
  stack.Synchronize(registry.generation);
  auto& output = stack.Output();
  if (output.valid) return {output.psqt, output.positional};
  const NetworkOutput result = SelectNetwork(position, [&](const auto& network) {
    return network.Evaluate(position, stack, registry.generation);
  });
  output = {true, result.psqt, result.positional};
  return result;
}

NetworkOutput EvaluateFromScratch(const Position& position) {
  return SelectNetwork(position, [&](const auto& network) {
    return network.EvaluateFromScratch(position);
  });
}
}  // namespace SimpleChessEngine::Nnue
