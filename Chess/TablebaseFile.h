#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <vector>

#include "Move.h"
#include "Tablebase.h"

namespace SimpleChessEngine {
class Position;

namespace Tablebase {

constexpr std::uint32_t kFileVersion = 1;
constexpr std::uint32_t kEndianMarker = 0x01020304;
constexpr std::size_t kRankBlockWords = 8;

struct FileHeader {
  std::array<char, 8> magic{};
  std::uint32_t version{};
  std::uint32_t endian_marker{};
  std::uint32_t class_count{};
  std::uint32_t header_size{};
  std::uint64_t directory_offset{};
  std::uint64_t file_size{};
  std::uint64_t directory_checksum{};
  std::array<std::uint64_t, 2> reserved{};
};
static_assert(sizeof(FileHeader) == 64);

struct ClassDirectoryEntry {
  std::uint8_t extra_count{};
  std::array<std::uint8_t, 2> extras{};
  std::uint8_t flags{};
  std::uint32_t reserved0{};
  std::uint64_t raw_size{};
  std::uint64_t dense_count{};
  std::uint64_t occupancy_offset{};
  std::uint64_t occupancy_word_count{};
  std::uint64_t rank_offset{};
  std::uint64_t rank_count{};
  std::uint64_t wdl_offset{};
  std::uint64_t dtz_offset{};
  std::uint64_t payload_checksum{};
  std::uint64_t reserved1{};
};
static_assert(sizeof(ClassDirectoryEntry) == 88);

struct ClassData {
  MaterialClass material;
  std::uint64_t raw_size{};
  std::vector<std::uint64_t> occupancy;
  std::vector<std::uint64_t> rank_checkpoints;
  std::vector<std::int8_t> wdl;
  std::vector<std::int16_t> dtz;
};

[[nodiscard]] std::vector<std::uint64_t> BuildRankCheckpoints(
    std::span<const std::uint64_t> occupancy);
[[nodiscard]] std::optional<std::uint64_t> DenseIndex(
    std::uint64_t raw_index, std::uint64_t raw_size,
    std::span<const std::uint64_t> occupancy,
    std::span<const std::uint64_t> rank_checkpoints);

class FileWriter {
 public:
  [[nodiscard]] static bool Write(const std::filesystem::path& path,
                                  std::span<const ClassData> classes);
};

class MappedFile {
 public:
  MappedFile();
  MappedFile(const MappedFile&) = delete;
  MappedFile& operator=(const MappedFile&) = delete;
  MappedFile(MappedFile&& other) noexcept;
  MappedFile& operator=(MappedFile&& other) noexcept;
  ~MappedFile();

  [[nodiscard]] static std::optional<MappedFile> Open(
      const std::filesystem::path& path);
  [[nodiscard]] std::optional<ProbeResult> Probe(
      const Position& position) const;
  [[nodiscard]] std::optional<Wdl> ProbeWdl(const Position& position) const;
  [[nodiscard]] std::optional<Move> RootMove(Position position) const;
  [[nodiscard]] bool VerifyChecksums() const;
  [[nodiscard]] std::uint32_t ClassCount() const;
  [[nodiscard]] std::vector<MaterialClass> Materials() const;
  [[nodiscard]] bool Contains(const MaterialClass& material,
                              std::uint64_t raw_index) const;

 private:
  [[nodiscard]] const FileHeader* Header() const;
  [[nodiscard]] std::span<const ClassDirectoryEntry> Directory() const;
  [[nodiscard]] std::optional<std::size_t> FindClass(
      const MaterialClass& material) const;
  [[nodiscard]] std::optional<ProbeResult> ProbeDirect(
      const Position& position, bool include_dtz) const;
  void Close();

  int descriptor_{-1};
  const std::byte* mapping_{};
  std::size_t size_{};
  std::array<std::int16_t, 4096> class_lookup_{};
};

}  // namespace Tablebase
}  // namespace SimpleChessEngine
