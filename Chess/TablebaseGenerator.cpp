#include "TablebaseGenerator.h"

#include <algorithm>
#include <atomic>
#include <bit>
#include <mutex>
#include <thread>
#include <utility>

#include "MoveGenerator.h"
#include "TablebasePosition.h"

namespace SimpleChessEngine::Tablebase {
namespace {

constexpr std::int8_t kUnknown = 3;
constexpr std::int16_t kUnknownDtz = std::numeric_limits<std::int16_t>::min();

template <class Callback>
void ForEachDensePosition(const ClassData& data, Callback callback) {
  std::uint64_t dense_index = 0;
  for (std::size_t word = 0; word < data.occupancy.size(); ++word) {
    auto bits = data.occupancy[word];
    while (bits != 0) {
      const auto bit = std::countr_zero(bits);
      const auto raw_index = word * 64 + bit;
      if (raw_index < data.raw_size) callback(raw_index, dense_index);
      ++dense_index;
      bits &= bits - 1;
    }
  }
}

[[nodiscard]] std::uint64_t DenseOffsetForWord(const ClassData& data,
                                               const std::size_t word) {
  const auto block = word / kRankBlockWords;
  auto dense_index = data.rank_checkpoints[block];
  for (auto previous = block * kRankBlockWords; previous < word; ++previous) {
    dense_index += std::popcount(data.occupancy[previous]);
  }
  return dense_index;
}

template <class Callback>
void ParallelForEachDensePosition(const ClassData& data,
                                  const std::size_t requested_threads,
                                  Callback callback) {
  const auto thread_count = std::max<std::size_t>(1, requested_threads);
  std::atomic<std::size_t> next_word{};
  std::vector<std::thread> workers;
  workers.reserve(thread_count);
  for (std::size_t thread = 0; thread < thread_count; ++thread) {
    workers.emplace_back([&] {
      while (true) {
        const auto word = next_word.fetch_add(1, std::memory_order_relaxed);
        if (word >= data.occupancy.size()) return;
        auto bits = data.occupancy[word];
        auto dense_index = DenseOffsetForWord(data, word);
        while (bits != 0) {
          const auto bit = std::countr_zero(bits);
          const auto raw_index = word * 64 + bit;
          if (raw_index < data.raw_size) callback(raw_index, dense_index);
          ++dense_index;
          bits &= bits - 1;
        }
      }
    });
  }
  for (auto& worker : workers) worker.join();
}

template <class Callback>
void ParallelForIndices(const std::size_t size,
                        const std::size_t requested_threads,
                        Callback callback) {
  const auto thread_count = std::max<std::size_t>(1, requested_threads);
  std::vector<std::thread> workers;
  workers.reserve(thread_count);
  for (std::size_t thread = 0; thread < thread_count; ++thread) {
    workers.emplace_back([&, thread] {
      const auto begin = size * thread / thread_count;
      const auto end = size * (thread + 1) / thread_count;
      for (auto index = begin; index < end; ++index) callback(index);
    });
  }
  for (auto& worker : workers) worker.join();
}

[[nodiscard]] ClassData BuildLayout(const MaterialClass material,
                                    const std::size_t requested_threads) {
  ClassData data;
  data.material = material;
  data.raw_size = PositionIndexer::RawSize(material);
  data.occupancy.resize((data.raw_size + 63) / 64);
  const auto thread_count = std::max<std::size_t>(1, requested_threads);
  std::atomic<std::size_t> next_word{};
  std::vector<std::thread> workers;
  workers.reserve(thread_count);
  for (std::size_t thread = 0; thread < thread_count; ++thread) {
    workers.emplace_back([&] {
      while (true) {
        const auto word = next_word.fetch_add(1, std::memory_order_relaxed);
        if (word >= data.occupancy.size()) return;
        std::uint64_t occupancy = 0;
        const auto begin = word * 64;
        const auto end = std::min(begin + 64, data.raw_size);
        for (auto raw_index = begin; raw_index < end; ++raw_index) {
          if (PositionBuilder::IsCanonicalLegalRaw(material, raw_index)) {
            occupancy |= 1ULL << (raw_index - begin);
          }
        }
        data.occupancy[word] = occupancy;
      }
    });
  }
  for (auto& worker : workers) worker.join();
  data.rank_checkpoints = BuildRankCheckpoints(data.occupancy);
  const auto dense_count = data.rank_checkpoints.back();
  data.wdl.assign(dense_count, kUnknown);
  data.dtz.assign(dense_count, 0);
  return data;
}

}  // namespace

std::optional<std::pair<std::size_t, std::uint64_t>>
GeneratedDatabase::Locate(const Position& position) const {
  Position position_without_ep = position;
  if (position_without_ep.GetEnCroissantSquare().has_value()) {
    PositionBuilder::ClearEnPassant(position_without_ep);
  }
  const auto canonical = PositionIndexer::Canonicalize(position_without_ep);
  if (!canonical) return std::nullopt;
  const auto class_index = class_lookup_[MaterialKey(canonical->material)];
  if (class_index < 0) return std::nullopt;
  const auto& data = classes_[class_index];
  const auto dense_index = DenseIndex(
      PositionIndexer::RawIndex(*canonical), data.raw_size, data.occupancy,
      data.rank_checkpoints);
  if (!dense_index) return std::nullopt;
  return std::pair{static_cast<std::size_t>(class_index), *dense_index};
}

std::optional<ProbeResult> GeneratedDatabase::Probe(
    const Position& position) const {
  const auto location = Locate(position);
  if (!location) return std::nullopt;
  const auto& [class_index, dense_index] = *location;
  const auto& data = classes_[class_index];
  const auto wdl = data.wdl[dense_index];
  if (wdl == kUnknown) return std::nullopt;
  return ProbeResult{static_cast<Wdl>(wdl), data.dtz[dense_index]};
}

bool GeneratedDatabase::Write(const std::filesystem::path& path) const {
  return FileWriter::Write(path, classes_);
}

std::optional<GeneratedDatabase> Generator::GenerateWdl(
    const std::span<const MaterialClass> materials, const std::size_t threads,
    Stats* stats, Progress progress) {
  auto generated_layout = GenerateLayout(materials, threads, progress);
  if (!generated_layout) return std::nullopt;
  auto database = std::move(*generated_layout);
  for (auto& data : database.classes_) {
    std::ranges::fill(data.wdl, kUnknown);
  }

  Stats local_stats;
  for (auto& data : database.classes_) {
    local_stats.legal_positions += data.wdl.size();
    std::atomic<std::uint64_t> terminal_losses{};
    std::atomic<std::uint64_t> terminal_draws{};
    ParallelForEachDensePosition(data, threads,
                                 [&](const std::uint64_t raw_index,
                                     const std::uint64_t dense_index) {
      thread_local MoveGenerator move_generator;
      const auto encoded = PositionIndexer::FromRawIndex(data.material,
                                                         raw_index);
      auto position = encoded ? PositionBuilder::Build(*encoded) : std::nullopt;
      if (!position) return;
      auto moves = move_generator.GenerateMoves<MoveGenerator::Type::kAll>(
          *position);
      if (moves.empty()) {
        if (position->IsUnderCheck()) {
          data.wdl[dense_index] = static_cast<std::int8_t>(Wdl::kLoss);
          terminal_losses.fetch_add(1, std::memory_order_relaxed);
        } else {
          data.wdl[dense_index] = static_cast<std::int8_t>(Wdl::kDraw);
          terminal_draws.fetch_add(1, std::memory_order_relaxed);
        }
      } else if (position->IsInsufficientMaterial()) {
        data.wdl[dense_index] = static_cast<std::int8_t>(Wdl::kDraw);
        terminal_draws.fetch_add(1, std::memory_order_relaxed);
      }
    });
    local_stats.terminal_losses += terminal_losses.load();
    local_stats.terminal_draws += terminal_draws.load();
  }

  const auto virtual_wdl = [&](Position& position) -> std::optional<std::int8_t> {
    Position base_position = position;
    PositionBuilder::ClearEnPassant(base_position);
    std::vector<std::optional<std::int8_t>> options;
    const auto base = database.Locate(base_position);
    if (base) {
      const auto& [base_class, base_dense] = *base;
      const auto value = database.classes_[base_class].wdl[base_dense];
      options.push_back(value == kUnknown ? std::nullopt
                                          : std::optional{value});
    } else {
      options.push_back(std::nullopt);
    }

    if (position.GetEnCroissantSquare().has_value()) {
      MoveGenerator ep_generator;
      const auto moves = ep_generator.GenerateMoves<MoveGenerator::Type::kAll>(
          position);
      for (const auto move : moves) {
        if (!move.IsEnPassant()) continue;
        const auto irreversible = position.GetIrreversibleData();
        position.DoMove(move);
        const auto child = database.Locate(position);
        position.UndoMove(move, irreversible);
        if (!child) {
          options.push_back(std::nullopt);
          continue;
        }
        const auto& [child_class, child_dense] = *child;
        const auto child_value =
            database.classes_[child_class].wdl[child_dense];
        options.push_back(child_value == kUnknown
                              ? std::nullopt
                              : std::optional{static_cast<std::int8_t>(
                                    -child_value)});
      }
    }

    if (std::ranges::any_of(options, [](const auto value) {
          return value && *value == static_cast<std::int8_t>(Wdl::kWin);
        })) {
      return static_cast<std::int8_t>(Wdl::kWin);
    }
    if (std::ranges::any_of(options, [](const auto value) {
          return value && *value == static_cast<std::int8_t>(Wdl::kDraw);
        })) {
      return static_cast<std::int8_t>(Wdl::kDraw);
    }
    if (std::ranges::all_of(options,
                            [](const auto value) {
                              return value &&
                                     *value == static_cast<std::int8_t>(
                                                   Wdl::kLoss);
                            })) {
      return static_cast<std::int8_t>(Wdl::kLoss);
    }
    return std::nullopt;
  };

  while (true) {
    ++local_stats.wdl_iterations;
    std::vector<std::vector<std::int8_t>> proposed(database.classes_.size());
    for (std::size_t class_index = 0;
         class_index < database.classes_.size(); ++class_index) {
      auto& data = database.classes_[class_index];
      proposed[class_index].assign(data.wdl.size(), kUnknown);
      ParallelForEachDensePosition(
          data, threads, [&](const std::uint64_t raw_index,
                             const std::uint64_t dense_index) {
        thread_local MoveGenerator move_generator;
        if (data.wdl[dense_index] != kUnknown) return;
        const auto encoded = PositionIndexer::FromRawIndex(data.material,
                                                           raw_index);
        auto position = encoded ? PositionBuilder::Build(*encoded)
                                : std::nullopt;
        if (!position) return;
        auto moves = move_generator.GenerateMoves<MoveGenerator::Type::kAll>(
            *position);
        bool all_children_win = true;
        for (const auto move : moves) {
          const auto irreversible = position->GetIrreversibleData();
          position->DoMove(move);
          const auto child_value = virtual_wdl(*position);
          position->UndoMove(move, irreversible);
          if (!child_value) {
            all_children_win = false;
            continue;
          }
          if (*child_value == static_cast<std::int8_t>(Wdl::kLoss)) {
            proposed[class_index][dense_index] =
                static_cast<std::int8_t>(Wdl::kWin);
            return;
          }
          if (*child_value != static_cast<std::int8_t>(Wdl::kWin)) {
            all_children_win = false;
          }
        }
        if (all_children_win) {
          proposed[class_index][dense_index] =
              static_cast<std::int8_t>(Wdl::kLoss);
        }
      });
    }
    std::uint64_t update_count = 0;
    std::atomic<std::uint64_t> resolved_wins{};
    std::atomic<std::uint64_t> resolved_losses{};
    for (std::size_t class_index = 0;
         class_index < database.classes_.size(); ++class_index) {
      auto& values = database.classes_[class_index].wdl;
      std::atomic<std::uint64_t> class_updates{};
      ParallelForIndices(values.size(), threads, [&](const std::size_t dense_index) {
        const auto value = proposed[class_index][dense_index];
        if (value == kUnknown || values[dense_index] != kUnknown) return;
        values[dense_index] = value;
        class_updates.fetch_add(1, std::memory_order_relaxed);
        if (value == static_cast<std::int8_t>(Wdl::kWin)) {
          resolved_wins.fetch_add(1, std::memory_order_relaxed);
        } else {
          resolved_losses.fetch_add(1, std::memory_order_relaxed);
        }
      });
      update_count += class_updates.load();
    }
    local_stats.resolved_wins += resolved_wins.load();
    local_stats.resolved_losses += resolved_losses.load();
    if (progress) progress("wdl", local_stats.wdl_iterations);
    if (update_count == 0) break;
  }

  for (auto& data : database.classes_) {
    std::atomic<std::uint64_t> draws{};
    ParallelForIndices(data.wdl.size(), threads, [&](const std::size_t index) {
      if (data.wdl[index] != kUnknown) return;
      data.wdl[index] = static_cast<std::int8_t>(Wdl::kDraw);
      draws.fetch_add(1, std::memory_order_relaxed);
    });
    local_stats.unresolved_draws += draws.load();
  }

  for (auto& data : database.classes_) {
    std::ranges::fill(data.dtz, kUnknownDtz);
    ParallelForEachDensePosition(data, threads,
                                 [&](const std::uint64_t raw_index,
                                     const std::uint64_t dense_index) {
      thread_local MoveGenerator move_generator;
      if (data.wdl[dense_index] == static_cast<std::int8_t>(Wdl::kDraw)) {
        data.dtz[dense_index] = 0;
        return;
      }
      const auto encoded = PositionIndexer::FromRawIndex(data.material,
                                                         raw_index);
      auto position = encoded ? PositionBuilder::Build(*encoded) : std::nullopt;
      if (!position) return;
      const auto moves = move_generator.GenerateMoves<MoveGenerator::Type::kAll>(
          *position);
      if (moves.empty()) data.dtz[dense_index] = 0;
    });
  }

  while (true) {
    ++local_stats.dtz_iterations;
    std::vector<std::vector<std::int16_t>> proposed(database.classes_.size());
    for (std::size_t class_index = 0;
         class_index < database.classes_.size(); ++class_index) {
      auto& data = database.classes_[class_index];
      proposed[class_index].assign(data.dtz.size(), kUnknownDtz);
      ParallelForEachDensePosition(
          data, threads, [&](const std::uint64_t raw_index,
                             const std::uint64_t dense_index) {
        thread_local MoveGenerator move_generator;
        if (data.dtz[dense_index] != kUnknownDtz) return;
        const auto encoded = PositionIndexer::FromRawIndex(data.material,
                                                           raw_index);
        auto position = encoded ? PositionBuilder::Build(*encoded)
                                : std::nullopt;
        if (!position) return;
        const auto current_wdl = data.wdl[dense_index];
        const auto moves = move_generator.GenerateMoves<MoveGenerator::Type::kAll>(
            *position);
        std::optional<std::uint16_t> winning_distance;
        std::uint16_t losing_distance = 0;
        bool all_losing_children_known = true;
        for (const auto move : moves) {
          const auto moving_piece = position->GetPieceAt(move.From());
          const bool zeroing = moving_piece == Piece::kPawn ||
                               !move.IsQuiet(*position);
          const auto irreversible = position->GetIrreversibleData();
          position->DoMove(move);
          if (zeroing) {
            const auto child_wdl = virtual_wdl(*position);
            position->UndoMove(move, irreversible);
            if (!child_wdl || *child_wdl != -current_wdl) {
              all_losing_children_known = false;
              continue;
            }
            if (current_wdl == static_cast<std::int8_t>(Wdl::kWin)) {
              winning_distance = 1;
            } else {
              losing_distance = std::max<std::uint16_t>(losing_distance, 1);
            }
            continue;
          }
          const auto child = database.Locate(*position);
          position->UndoMove(move, irreversible);
          if (!child) {
            all_losing_children_known = false;
            continue;
          }
          const auto& [child_class, child_dense] = *child;
          const auto& child_data = database.classes_[child_class];
          if (child_data.wdl[child_dense] != -current_wdl) continue;
          const auto child_dtz = child_data.dtz[child_dense];
          if (child_dtz == kUnknownDtz) {
            all_losing_children_known = false;
            continue;
          }
          const auto distance = static_cast<std::uint16_t>(
              std::min<std::int32_t>(
                  std::abs(static_cast<std::int32_t>(child_dtz)) + 1,
                  std::numeric_limits<std::int16_t>::max()));
          if (current_wdl == static_cast<std::int8_t>(Wdl::kWin)) {
            winning_distance = winning_distance
                                   ? std::min(*winning_distance, distance)
                                   : distance;
          } else {
            losing_distance = std::max(losing_distance, distance);
          }
        }
        if (current_wdl == static_cast<std::int8_t>(Wdl::kWin) &&
            winning_distance) {
          proposed[class_index][dense_index] =
              static_cast<std::int16_t>(*winning_distance);
        } else if (current_wdl == static_cast<std::int8_t>(Wdl::kLoss) &&
                   all_losing_children_known) {
          proposed[class_index][dense_index] = static_cast<std::int16_t>(
              -static_cast<std::int32_t>(losing_distance));
        }
      });
    }
    std::uint64_t update_count = 0;
    for (std::size_t class_index = 0;
         class_index < database.classes_.size(); ++class_index) {
      auto& values = database.classes_[class_index].dtz;
      std::atomic<std::uint64_t> class_updates{};
      ParallelForIndices(values.size(), threads, [&](const std::size_t dense_index) {
        const auto value = proposed[class_index][dense_index];
        if (value == kUnknownDtz || values[dense_index] != kUnknownDtz) return;
        values[dense_index] = value;
        class_updates.fetch_add(1, std::memory_order_relaxed);
      });
      update_count += class_updates.load();
    }
    if (progress) progress("dtz", local_stats.dtz_iterations);
    if (update_count == 0) break;
  }

  for (auto& data : database.classes_) {
    std::atomic<bool> invalid_dtz{};
    std::atomic<std::uint16_t> class_max_dtz{};
    ParallelForIndices(data.wdl.size(), threads, [&](const std::size_t i) {
      auto& dtz = data.dtz[i];
      if (dtz == kUnknownDtz) {
        invalid_dtz.store(true, std::memory_order_relaxed);
        return;
      }
      if (dtz == 0 &&
          data.wdl[i] == static_cast<std::int8_t>(Wdl::kLoss)) {
        dtz = -1;
      }
      const auto distance = static_cast<std::uint16_t>(
          std::abs(static_cast<std::int32_t>(dtz)));
      auto maximum = class_max_dtz.load(std::memory_order_relaxed);
      while (maximum < distance &&
             !class_max_dtz.compare_exchange_weak(
                 maximum, distance, std::memory_order_relaxed)) {
      }
      if (dtz > 100 && data.wdl[i] == static_cast<std::int8_t>(Wdl::kWin)) {
        data.wdl[i] = static_cast<std::int8_t>(Wdl::kCursedWin);
      } else if (dtz < -100 &&
                 data.wdl[i] == static_cast<std::int8_t>(Wdl::kLoss)) {
        data.wdl[i] = static_cast<std::int8_t>(Wdl::kBlessedLoss);
      }
    });
    if (invalid_dtz.load()) return std::nullopt;
    local_stats.max_dtz =
        std::max(local_stats.max_dtz, class_max_dtz.load());
  }
  if (stats) *stats = local_stats;
  return database;
}

std::optional<GeneratedDatabase> Generator::GenerateLayout(
    const std::span<const MaterialClass> materials, const std::size_t threads,
    Progress progress) {
  if (materials.empty()) return std::nullopt;
  GeneratedDatabase database;
  database.class_lookup_.fill(-1);
  database.classes_.reserve(materials.size());
  std::uint32_t completed_layouts = 0;
  for (const auto material : materials) {
    const auto key = MaterialKey(material);
    if (database.class_lookup_[key] >= 0) return std::nullopt;
    database.class_lookup_[key] = database.classes_.size();
    database.classes_.push_back(BuildLayout(material, threads));
    auto& data = database.classes_.back();
    std::ranges::fill(data.wdl, static_cast<std::int8_t>(Wdl::kDraw));
    std::ranges::fill(data.dtz, 0);
    if (progress) progress("layout", ++completed_layouts);
  }
  return database;
}

}  // namespace SimpleChessEngine::Tablebase
