#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <istream>
#include <type_traits>

#if defined(__AVX2__)
#define SCE_NNUE_AVX2
#include <immintrin.h>
#if defined(__AVXVNNI__)
#define SCE_NNUE_AVXVNNI
#endif
#endif

namespace SimpleChessEngine::Nnue {
using IndexType = std::uint32_t;
using TransformedFeatureType = std::uint8_t;

constexpr std::uint32_t kVersion = 0x7AF32F20u;
constexpr int kOutputScale = 16;
constexpr int kWeightScaleBits = 6;
constexpr std::size_t kCacheLineSize = 64;
constexpr std::size_t kMaxSimdWidth = 32;
#if defined(SCE_NNUE_AVX2)
constexpr std::size_t kSimdWidth = 32;
#endif

constexpr IndexType kPsqtBuckets = 8;
constexpr IndexType kLayerStacks = 8;

constexpr IndexType kSmallDimensions = 128;
constexpr int kSmallL2 = 15;
constexpr int kSmallL3 = 32;

constexpr IndexType kBigDimensions = 3072;
constexpr int kBigL2 = 15;
constexpr int kBigL3 = 32;

template <typename IntType>
constexpr IntType CeilToMultiple(IntType n, IntType base) {
  return (n + base - 1) / base * base;
}

template <typename IntType>
IntType ReadLittleEndian(std::istream& stream) {
  IntType result{};
  stream.read(reinterpret_cast<char*>(&result), sizeof(IntType));
  return result;
}

template <typename IntType>
void ReadLittleEndian(std::istream& stream, IntType* out,
                      const std::size_t count) {
  stream.read(reinterpret_cast<char*>(out), sizeof(IntType) * count);
}

constexpr char kLeb128MagicString[] = "COMPRESSED_LEB128";
constexpr std::size_t kLeb128MagicStringSize = sizeof(kLeb128MagicString) - 1;

template <typename IntType>
bool ReadLeb128(std::istream& stream, IntType* out, const std::size_t count) {
  static_assert(std::is_signed_v<IntType>);
  char magic[kLeb128MagicStringSize];
  stream.read(magic, kLeb128MagicStringSize);
  if (!stream ||
      std::memcmp(magic, kLeb128MagicString, kLeb128MagicStringSize) != 0) {
    return false;
  }

  constexpr std::uint32_t kBufferSize = 4096;
  std::uint8_t buffer[kBufferSize];
  auto bytes_left = ReadLittleEndian<std::uint32_t>(stream);
  std::uint32_t buffer_position = kBufferSize;
  for (std::size_t i = 0; i < count; ++i) {
    IntType result = 0;
    std::size_t shift = 0;
    do {
      if (buffer_position == kBufferSize) {
        stream.read(reinterpret_cast<char*>(buffer),
                    std::min(bytes_left, kBufferSize));
        buffer_position = 0;
      }
      const std::uint8_t byte = buffer[buffer_position++];
      --bytes_left;
      result |= static_cast<IntType>((byte & 0x7f) << shift);
      shift += 7;
      if ((byte & 0x80) == 0) {
        out[i] = (sizeof(IntType) * 8 <= shift || (byte & 0x40) == 0)
                     ? result
                     : static_cast<IntType>(result | ~((1 << shift) - 1));
        break;
      }
    } while (shift < sizeof(IntType) * 8);
  }
  return bytes_left == 0 && static_cast<bool>(stream);
}

template <typename T, std::size_t kMaxSize>
class ValueList {
 public:
  [[nodiscard]] std::size_t size() const { return size_; }
  void push_back(const T& value) { values_[size_++] = value; }
  [[nodiscard]] const T* begin() const { return values_; }
  [[nodiscard]] const T* end() const { return values_ + size_; }
  [[nodiscard]] const T& operator[](const std::size_t index) const {
    return values_[index];
  }

 private:
  T values_[kMaxSize];
  std::size_t size_ = 0;
};
}  // namespace SimpleChessEngine::Nnue
