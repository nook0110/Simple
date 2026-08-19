#include "TablebaseFile.h"

#include <algorithm>
#include <bit>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <limits>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>

#include "MoveGenerator.h"
#include "Position.h"
#include "TablebasePosition.h"

namespace SimpleChessEngine::Tablebase {
namespace {

constexpr std::array<char, 8> kMagic{'S', 'C', 'E', 'T', 'B', '0', '1', '\0'};

[[nodiscard]] std::uint64_t Align(const std::uint64_t value,
                                  const std::uint64_t alignment) {
  return (value + alignment - 1) / alignment * alignment;
}

[[nodiscard]] std::uint64_t Checksum(const void* data, const std::size_t size,
                                     std::uint64_t hash =
                                         14695981039346656037ULL) {
  const auto* bytes = static_cast<const std::uint8_t*>(data);
  for (std::size_t i = 0; i < size; ++i) {
    hash ^= bytes[i];
    hash *= 1099511628211ULL;
  }
  return hash;
}

template <class T>
[[nodiscard]] std::uint64_t ChecksumSpan(const std::span<const T> values,
                                         const std::uint64_t seed =
                                             14695981039346656037ULL) {
  return Checksum(values.data(), values.size_bytes(), seed);
}

[[nodiscard]] MaterialClass DecodeMaterial(
    const ClassDirectoryEntry& entry) {
  return MaterialClass{{static_cast<PieceCode>(entry.extras[0]),
                        static_cast<PieceCode>(entry.extras[1])},
                       entry.extra_count};
}

[[nodiscard]] bool IsRangeValid(const std::uint64_t offset,
                                const std::uint64_t byte_count,
                                const std::uint64_t file_size) {
  return offset <= file_size && byte_count <= file_size - offset;
}

[[nodiscard]] bool ValidateClassData(const ClassData& data) {
  if (data.material.extra_count > 2 ||
      data.raw_size != PositionIndexer::RawSize(data.material)) {
    return false;
  }
  const auto expected_words = (data.raw_size + 63) / 64;
  if (data.occupancy.size() != expected_words ||
      data.rank_checkpoints != BuildRankCheckpoints(data.occupancy) ||
      data.wdl.size() != data.dtz.size()) {
    return false;
  }
  const auto dense_count = data.rank_checkpoints.back();
  if (data.wdl.size() != dense_count) return false;
  return std::ranges::all_of(data.wdl, [](const std::int8_t value) {
    return value >= static_cast<std::int8_t>(Wdl::kLoss) &&
           value <= static_cast<std::int8_t>(Wdl::kWin);
  });
}

[[nodiscard]] Wdl NegateWdl(const Wdl value) {
  return static_cast<Wdl>(-static_cast<std::int8_t>(value));
}

[[nodiscard]] int WdlSign(const Wdl value) {
  return (static_cast<std::int8_t>(value) > 0) -
         (static_cast<std::int8_t>(value) < 0);
}

[[nodiscard]] ProbeResult BetterResult(const ProbeResult lhs,
                                       const ProbeResult rhs) {
  if (lhs.wdl != rhs.wdl) {
    return static_cast<std::int8_t>(lhs.wdl) >
                   static_cast<std::int8_t>(rhs.wdl)
               ? lhs
               : rhs;
  }
  if (lhs.wdl == Wdl::kWin || lhs.wdl == Wdl::kCursedWin) {
    return std::abs(lhs.dtz) <= std::abs(rhs.dtz) ? lhs : rhs;
  }
  if (lhs.wdl == Wdl::kLoss || lhs.wdl == Wdl::kBlessedLoss) {
    return std::abs(lhs.dtz) >= std::abs(rhs.dtz) ? lhs : rhs;
  }
  return lhs;
}

[[nodiscard]] Wdl ApplyHalfmoveClock(const Wdl wdl, const std::int16_t dtz,
                                     const std::size_t halfmove_clock) {
  if (wdl == Wdl::kWin && halfmove_clock + std::abs(dtz) > 100) {
    return Wdl::kCursedWin;
  }
  if (wdl == Wdl::kLoss && halfmove_clock + std::abs(dtz) > 100) {
    return Wdl::kBlessedLoss;
  }
  return wdl;
}

}  // namespace

std::vector<std::uint64_t> BuildRankCheckpoints(
    const std::span<const std::uint64_t> occupancy) {
  const auto block_count =
      (occupancy.size() + kRankBlockWords - 1) / kRankBlockWords;
  std::vector<std::uint64_t> checkpoints(block_count + 1);
  std::uint64_t count = 0;
  for (std::size_t block = 0; block < block_count; ++block) {
    checkpoints[block] = count;
    const auto begin = block * kRankBlockWords;
    const auto end = std::min(begin + kRankBlockWords, occupancy.size());
    for (auto word = begin; word < end; ++word) {
      count += std::popcount(occupancy[word]);
    }
  }
  checkpoints.back() = count;
  return checkpoints;
}

std::optional<std::uint64_t> DenseIndex(
    const std::uint64_t raw_index, const std::uint64_t raw_size,
    const std::span<const std::uint64_t> occupancy,
    const std::span<const std::uint64_t> rank_checkpoints) {
  if (raw_index >= raw_size) return std::nullopt;
  const auto word_index = raw_index / 64;
  const auto bit_index = raw_index % 64;
  if (word_index >= occupancy.size() ||
      (occupancy[word_index] & (1ULL << bit_index)) == 0) {
    return std::nullopt;
  }
  const auto block = word_index / kRankBlockWords;
  if (block >= rank_checkpoints.size()) return std::nullopt;
  std::uint64_t rank = rank_checkpoints[block];
  for (auto word = block * kRankBlockWords; word < word_index; ++word) {
    rank += std::popcount(occupancy[word]);
  }
  const auto before = bit_index == 0 ? 0 : ((1ULL << bit_index) - 1);
  rank += std::popcount(occupancy[word_index] & before);
  return rank;
}

bool FileWriter::Write(const std::filesystem::path& path,
                       const std::span<const ClassData> classes) {
  if (classes.empty() ||
      !std::ranges::all_of(classes, ValidateClassData)) {
    return false;
  }
  std::vector<ClassDirectoryEntry> directory(classes.size());
  auto offset = Align(sizeof(FileHeader) +
                          directory.size() * sizeof(ClassDirectoryEntry),
                      64);
  for (std::size_t i = 0; i < classes.size(); ++i) {
    const auto& data = classes[i];
    auto& entry = directory[i];
    entry.extra_count = data.material.extra_count;
    entry.extras = {static_cast<std::uint8_t>(data.material.extras[0]),
                    static_cast<std::uint8_t>(data.material.extras[1])};
    entry.raw_size = data.raw_size;
    entry.dense_count = data.wdl.size();
    entry.occupancy_offset = offset;
    entry.occupancy_word_count = data.occupancy.size();
    offset = Align(offset + data.occupancy.size() * sizeof(std::uint64_t), 64);
    entry.rank_offset = offset;
    entry.rank_count = data.rank_checkpoints.size();
    offset = Align(offset +
                       data.rank_checkpoints.size() * sizeof(std::uint64_t),
                   64);
    entry.wdl_offset = offset;
    offset = Align(offset + data.wdl.size() * sizeof(std::int8_t), 64);
    entry.dtz_offset = offset;
    offset = Align(offset + data.dtz.size() * sizeof(std::int16_t), 64);

    auto checksum = ChecksumSpan<std::uint64_t>(data.occupancy);
    checksum = ChecksumSpan<std::uint64_t>(data.rank_checkpoints, checksum);
    checksum = ChecksumSpan<std::int8_t>(data.wdl, checksum);
    entry.payload_checksum = ChecksumSpan<std::int16_t>(data.dtz, checksum);
  }

  FileHeader header;
  header.magic = kMagic;
  header.version = kFileVersion;
  header.endian_marker = kEndianMarker;
  header.class_count = classes.size();
  header.header_size = sizeof(FileHeader);
  header.directory_offset = sizeof(FileHeader);
  header.file_size = offset;
  header.directory_checksum = ChecksumSpan<ClassDirectoryEntry>(directory);

  const auto descriptor =
      open(path.c_str(), O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR |
                                                       S_IRGRP | S_IROTH);
  if (descriptor < 0) return false;
  if (ftruncate(descriptor, header.file_size) != 0) {
    close(descriptor);
    return false;
  }
  auto* mapping = static_cast<std::byte*>(
      mmap(nullptr, header.file_size, PROT_READ | PROT_WRITE, MAP_SHARED,
           descriptor, 0));
  if (mapping == MAP_FAILED) {
    close(descriptor);
    return false;
  }
  std::memcpy(mapping, &header, sizeof(header));
  std::memcpy(mapping + header.directory_offset, directory.data(),
              directory.size() * sizeof(ClassDirectoryEntry));
  for (std::size_t i = 0; i < classes.size(); ++i) {
    const auto& data = classes[i];
    const auto& entry = directory[i];
    std::memcpy(mapping + entry.occupancy_offset, data.occupancy.data(),
                data.occupancy.size() * sizeof(std::uint64_t));
    std::memcpy(mapping + entry.rank_offset, data.rank_checkpoints.data(),
                data.rank_checkpoints.size() * sizeof(std::uint64_t));
    std::memcpy(mapping + entry.wdl_offset, data.wdl.data(),
                data.wdl.size() * sizeof(std::int8_t));
    std::memcpy(mapping + entry.dtz_offset, data.dtz.data(),
                data.dtz.size() * sizeof(std::int16_t));
  }
  const auto success = msync(mapping, header.file_size, MS_SYNC) == 0;
  munmap(mapping, header.file_size);
  close(descriptor);
  return success;
}

MappedFile::MappedFile() { class_lookup_.fill(-1); }

MappedFile::MappedFile(MappedFile&& other) noexcept
    : descriptor_(std::exchange(other.descriptor_, -1)),
      mapping_(std::exchange(other.mapping_, nullptr)),
      size_(std::exchange(other.size_, 0)),
      class_lookup_(other.class_lookup_) {
  other.class_lookup_.fill(-1);
}

MappedFile& MappedFile::operator=(MappedFile&& other) noexcept {
  if (this == &other) return *this;
  Close();
  descriptor_ = std::exchange(other.descriptor_, -1);
  mapping_ = std::exchange(other.mapping_, nullptr);
  size_ = std::exchange(other.size_, 0);
  class_lookup_ = other.class_lookup_;
  other.class_lookup_.fill(-1);
  return *this;
}

MappedFile::~MappedFile() { Close(); }

std::optional<MappedFile> MappedFile::Open(
    const std::filesystem::path& path) {
  MappedFile result;
  result.descriptor_ = open(path.c_str(), O_RDONLY);
  if (result.descriptor_ < 0) return std::nullopt;
  struct stat status {};
  if (fstat(result.descriptor_, &status) != 0 ||
      status.st_size < static_cast<off_t>(sizeof(FileHeader))) {
    return std::nullopt;
  }
  result.size_ = status.st_size;
  result.mapping_ = static_cast<const std::byte*>(
      mmap(nullptr, result.size_, PROT_READ, MAP_PRIVATE, result.descriptor_, 0));
  if (result.mapping_ == MAP_FAILED) {
    result.mapping_ = nullptr;
    return std::nullopt;
  }
  const auto* header = result.Header();
  if (header->magic != kMagic || header->version != kFileVersion ||
      header->endian_marker != kEndianMarker ||
      header->header_size != sizeof(FileHeader) ||
      header->file_size != result.size_ ||
      !IsRangeValid(header->directory_offset,
                    static_cast<std::uint64_t>(header->class_count) *
                        sizeof(ClassDirectoryEntry),
                    result.size_) ||
      header->directory_checksum !=
          ChecksumSpan<ClassDirectoryEntry>(result.Directory())) {
    return std::nullopt;
  }
  std::size_t class_index = 0;
  for (const auto& entry : result.Directory()) {
    const auto key = MaterialKey(DecodeMaterial(entry));
    if (entry.extra_count > 2 ||
        key >= result.class_lookup_.size() ||
        result.class_lookup_[key] >= 0 ||
        entry.raw_size != PositionIndexer::RawSize(DecodeMaterial(entry)) ||
        !IsRangeValid(entry.occupancy_offset,
                      entry.occupancy_word_count * sizeof(std::uint64_t),
                      result.size_) ||
        !IsRangeValid(entry.rank_offset,
                      entry.rank_count * sizeof(std::uint64_t), result.size_) ||
        !IsRangeValid(entry.wdl_offset,
                      entry.dense_count * sizeof(std::int8_t), result.size_) ||
        !IsRangeValid(entry.dtz_offset,
                      entry.dense_count * sizeof(std::int16_t), result.size_)) {
      return std::nullopt;
    }
    result.class_lookup_[key] = class_index++;
  }
  return result;
}

std::optional<ProbeResult> MappedFile::Probe(const Position& position) const {
  if (position.GetAllPieces().Count() > 4) return std::nullopt;
  if (!position.GetEnCroissantSquare().has_value()) {
    auto result = ProbeDirect(position, true);
    if (result) {
      result->wdl = ApplyHalfmoveClock(result->wdl, result->dtz,
                                       position.GetHalfMoveClock());
    }
    return result;
  }
  Position base = position;
  PositionBuilder::ClearEnPassant(base);
  auto best = ProbeDirect(base, true);
  if (best) {
    best->wdl = ApplyHalfmoveClock(best->wdl, best->dtz,
                                   position.GetHalfMoveClock());
  }
  MoveGenerator generator;
  auto mutable_position = position;
  const auto moves =
      generator.GenerateMoves<MoveGenerator::Type::kAll>(mutable_position);
  for (const auto move : moves) {
    if (!move.IsEnPassant()) continue;
    const auto irreversible = mutable_position.GetIrreversibleData();
    mutable_position.DoMove(move);
    const auto child = Probe(mutable_position);
    mutable_position.UndoMove(move, irreversible);
    if (!child) continue;
    const auto wdl = NegateWdl(child->wdl);
    const ProbeResult result{wdl,
                             static_cast<std::int16_t>(WdlSign(wdl))};
    best = best ? BetterResult(*best, result) : result;
  }
  return best;
}

std::optional<Wdl> MappedFile::ProbeWdl(const Position& position) const {
  const auto result = Probe(position);
  return result ? std::optional{result->wdl} : std::nullopt;
}

std::optional<Move> MappedFile::RootMove(Position position) const {
  if (position.GetAllPieces().Count() > 4) return std::nullopt;
  const auto& castling_rights = position.GetCastlingRights();
  if (castling_rights[0].any() || castling_rights[1].any()) {
    return std::nullopt;
  }
  MoveGenerator generator;
  const auto moves = generator.GenerateMoves<MoveGenerator::Type::kAll>(position);
  std::optional<Move> best_move;
  std::optional<ProbeResult> best_result;
  for (const auto move : moves) {
    const auto moving_piece = position.GetPieceAt(move.From());
    const bool zeroing = moving_piece == Piece::kPawn || !move.IsQuiet(position);
    const auto irreversible = position.GetIrreversibleData();
    position.DoMove(move);
    const auto child = Probe(position);
    position.UndoMove(move, irreversible);
    if (!child) return std::nullopt;
    const auto wdl = NegateWdl(child->wdl);
    const auto distance = zeroing
                              ? 1
                              : std::min<int>(
                                    std::abs(static_cast<int>(child->dtz)) + 1,
                                    std::numeric_limits<std::int16_t>::max());
    ProbeResult result{wdl, static_cast<std::int16_t>(WdlSign(wdl) * distance)};
    if (!best_result || BetterResult(result, *best_result) == result) {
      best_result = result;
      best_move = move;
    }
  }
  return best_move;
}

std::optional<ProbeResult> MappedFile::ProbeDirect(
    const Position& position, const bool include_dtz) const {
  const auto canonical = PositionIndexer::Canonicalize(position);
  if (!canonical) return std::nullopt;
  const auto class_index = FindClass(canonical->material);
  if (!class_index) return std::nullopt;
  const auto& entry = Directory()[*class_index];
  const auto occupancy = std::span{
      reinterpret_cast<const std::uint64_t*>(mapping_ + entry.occupancy_offset),
      static_cast<std::size_t>(entry.occupancy_word_count)};
  const auto checkpoints = std::span{
      reinterpret_cast<const std::uint64_t*>(mapping_ + entry.rank_offset),
      static_cast<std::size_t>(entry.rank_count)};
  const auto dense_index = DenseIndex(PositionIndexer::RawIndex(*canonical),
                                      entry.raw_size, occupancy, checkpoints);
  if (!dense_index || *dense_index >= entry.dense_count) return std::nullopt;
  const auto* wdl = reinterpret_cast<const std::int8_t*>(
      mapping_ + entry.wdl_offset);
  if (!include_dtz) {
    return ProbeResult{static_cast<Wdl>(wdl[*dense_index]), 0};
  }
  const auto* dtz =
      reinterpret_cast<const std::int16_t*>(mapping_ + entry.dtz_offset);
  return ProbeResult{static_cast<Wdl>(wdl[*dense_index]),
                     dtz[*dense_index]};
}

bool MappedFile::VerifyChecksums() const {
  if (!mapping_) return false;
  for (const auto& entry : Directory()) {
    auto checksum = Checksum(mapping_ + entry.occupancy_offset,
                             entry.occupancy_word_count * sizeof(std::uint64_t));
    checksum = Checksum(mapping_ + entry.rank_offset,
                        entry.rank_count * sizeof(std::uint64_t), checksum);
    checksum = Checksum(mapping_ + entry.wdl_offset,
                        entry.dense_count * sizeof(std::int8_t), checksum);
    checksum = Checksum(mapping_ + entry.dtz_offset,
                        entry.dense_count * sizeof(std::int16_t), checksum);
    if (checksum != entry.payload_checksum) return false;
  }
  return true;
}

std::uint32_t MappedFile::ClassCount() const {
  return mapping_ ? Header()->class_count : 0;
}

std::vector<MaterialClass> MappedFile::Materials() const {
  std::vector<MaterialClass> result;
  if (!mapping_) return result;
  result.reserve(Header()->class_count);
  for (const auto& entry : Directory()) result.push_back(DecodeMaterial(entry));
  return result;
}

bool MappedFile::Contains(const MaterialClass& material,
                          const std::uint64_t raw_index) const {
  const auto class_index = FindClass(material);
  if (!class_index) return false;
  const auto& entry = Directory()[*class_index];
  const auto occupancy = std::span{
      reinterpret_cast<const std::uint64_t*>(mapping_ + entry.occupancy_offset),
      static_cast<std::size_t>(entry.occupancy_word_count)};
  const auto checkpoints = std::span{
      reinterpret_cast<const std::uint64_t*>(mapping_ + entry.rank_offset),
      static_cast<std::size_t>(entry.rank_count)};
  return DenseIndex(raw_index, entry.raw_size, occupancy, checkpoints).has_value();
}

const FileHeader* MappedFile::Header() const {
  return reinterpret_cast<const FileHeader*>(mapping_);
}

std::span<const ClassDirectoryEntry> MappedFile::Directory() const {
  const auto* header = Header();
  return {reinterpret_cast<const ClassDirectoryEntry*>(
              mapping_ + header->directory_offset),
          header->class_count};
}

std::optional<std::size_t> MappedFile::FindClass(
    const MaterialClass& material) const {
  const auto key = MaterialKey(material);
  if (key >= class_lookup_.size() || class_lookup_[key] < 0) {
    return std::nullopt;
  }
  return static_cast<std::size_t>(class_lookup_[key]);
}

void MappedFile::Close() {
  if (mapping_) {
    munmap(const_cast<std::byte*>(mapping_), size_);
    mapping_ = nullptr;
  }
  if (descriptor_ >= 0) {
    close(descriptor_);
    descriptor_ = -1;
  }
  size_ = 0;
  class_lookup_.fill(-1);
}

}  // namespace SimpleChessEngine::Tablebase
