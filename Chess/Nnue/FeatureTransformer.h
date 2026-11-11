#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <istream>

#include "../Player.h"
#include "Accumulator.h"
#include "Common.h"
#include "Features.h"
#include "Layers.h"

namespace SimpleChessEngine::Nnue {
using BiasType = std::int16_t;
using WeightType = std::int16_t;
using PsqtWeightType = std::int32_t;

template <IndexType kHalfDimensions>
struct alignas(kCacheLineSize) RefreshEntry {
  BiasType accumulation[kHalfDimensions];
  PsqtWeightType psqt_accumulation[kPsqtBuckets];
  std::array<std::uint64_t, 2> by_color;
  std::array<std::uint64_t, 7> by_type;
};

template <IndexType kHalfDimensions>
class FeatureTransformer {
 public:
  using OutputType = TransformedFeatureType;

  static constexpr IndexType kInputDimensions = Features::kDimensions;
  static constexpr IndexType kOutputDimensions = kHalfDimensions;

  static constexpr std::uint32_t GetHashValue() {
    return Features::kHashValue ^ (kOutputDimensions * 2);
  }

  bool ReadParameters(std::istream& stream) {
    if (!ReadLeb128<BiasType>(stream, biases, kHalfDimensions) ||
        !ReadLeb128<WeightType>(stream, weights,
                                kHalfDimensions * kInputDimensions) ||
        !ReadLeb128<PsqtWeightType>(stream, psqt_weights,
                                    kPsqtBuckets * kInputDimensions)) {
      return false;
    }
    PermuteWeights();
    for (auto& weight : weights) weight = static_cast<WeightType>(weight * 2);
    for (auto& bias : biases) bias = static_cast<BiasType>(bias * 2);
    return !stream.fail();
  }

  std::int32_t Transform(const Accumulator<kHalfDimensions>& accumulator,
                         const Player side_to_move, const int bucket,
                         OutputType* output,
                         [[maybe_unused]] std::uint16_t* nonzero,
                         IndexType& nonzero_count) const {
    nonzero_count = 0;
#if defined(SCE_NNUE_AVX2)
    __m128i base = _mm_setzero_si128();
    const __m128i increment = _mm_set1_epi16(8);
#endif
    const std::array<std::size_t, 2> perspectives = {
        static_cast<std::size_t>(side_to_move),
        static_cast<std::size_t>(Flip(side_to_move))};
    const std::int32_t psqt =
        (accumulator.psqt_accumulation[perspectives[0]][bucket] -
         accumulator.psqt_accumulation[perspectives[1]][bucket]) /
        2;

    for (std::size_t p = 0; p < 2; ++p) {
      const IndexType offset = (kHalfDimensions / 2) * p;
      const BiasType* values = accumulator.accumulation[perspectives[p]];
#if defined(SCE_NNUE_AVX2)
      constexpr IndexType kOutputChunkSize = 32;
      static_assert((kHalfDimensions / 2) % kOutputChunkSize == 0);
      constexpr IndexType kNumOutputChunks =
          kHalfDimensions / 2 / kOutputChunkSize;

      const __m256i zero = _mm256_setzero_si256();
      const __m256i one = _mm256_set1_epi16(127 * 2);
      const auto in0 = reinterpret_cast<const __m256i*>(values);
      const auto in1 =
          reinterpret_cast<const __m256i*>(values + kHalfDimensions / 2);
      const auto out = reinterpret_cast<__m256i*>(output + offset);
      constexpr int kShift = 7;
      for (IndexType j = 0; j < kNumOutputChunks; ++j) {
        const __m256i sum0a = _mm256_slli_epi16(
            _mm256_max_epi16(_mm256_min_epi16(in0[j * 2 + 0], one), zero),
            kShift);
        const __m256i sum0b = _mm256_slli_epi16(
            _mm256_max_epi16(_mm256_min_epi16(in0[j * 2 + 1], one), zero),
            kShift);
        const __m256i sum1a = _mm256_min_epi16(in1[j * 2 + 0], one);
        const __m256i sum1b = _mm256_min_epi16(in1[j * 2 + 1], one);
        const __m256i pa = _mm256_mulhi_epi16(sum0a, sum1a);
        const __m256i pb = _mm256_mulhi_epi16(sum0b, sum1b);
        const __m256i transformed = _mm256_packus_epi16(pa, pb);
        out[j] = transformed;
        Layers::AppendNonZero(transformed, base, nonzero, nonzero_count);
        base = _mm_add_epi16(base, increment);
      }
#else
      for (IndexType j = 0; j < kHalfDimensions / 2; ++j) {
        const BiasType sum0 =
            std::clamp<BiasType>(values[j], 0, static_cast<BiasType>(127 * 2));
        const BiasType sum1 = std::clamp<BiasType>(
            values[j + kHalfDimensions / 2], 0, static_cast<BiasType>(127 * 2));
        output[offset + j] =
            static_cast<OutputType>(static_cast<unsigned>(sum0 * sum1) / 512);
      }
#endif
    }
    return psqt;
  }

  template <std::size_t kMaxChanges>
  void ApplyUpdate(const BiasType* input, BiasType* output,
                   const PsqtWeightType* psqt_input,
                   PsqtWeightType* psqt_output,
                   const ValueList<IndexType, kMaxChanges>& removed,
                   const ValueList<IndexType, kMaxChanges>& added) const {
#if defined(SCE_NNUE_AVX2)
    constexpr IndexType kNumChunks =
        kHalfDimensions * sizeof(BiasType) / sizeof(__m256i);
    const auto in = reinterpret_cast<const __m256i*>(input);
    const auto out = reinterpret_cast<__m256i*>(output);
    const __m256i* removed_columns[kMaxChanges]{};
    const __m256i* added_columns[kMaxChanges]{};
    for (std::size_t i = 0; i < removed.size(); ++i) {
      removed_columns[i] = reinterpret_cast<const __m256i*>(
          &weights[kHalfDimensions * removed[i]]);
    }
    for (std::size_t i = 0; i < added.size(); ++i) {
      added_columns[i] = reinterpret_cast<const __m256i*>(
          &weights[kHalfDimensions * added[i]]);
    }
    constexpr IndexType kNumRegisters = 8;
    static_assert(kNumChunks % kNumRegisters == 0);
    for (IndexType tile = 0; tile < kNumChunks; tile += kNumRegisters) {
      __m256i registers[kNumRegisters];
      for (IndexType k = 0; k < kNumRegisters; ++k) {
        registers[k] = in[tile + k];
      }
      for (std::size_t r = 0; r < removed.size(); ++r) {
        for (IndexType k = 0; k < kNumRegisters; ++k) {
          registers[k] =
              _mm256_sub_epi16(registers[k], removed_columns[r][tile + k]);
        }
      }
      for (std::size_t a = 0; a < added.size(); ++a) {
        for (IndexType k = 0; k < kNumRegisters; ++k) {
          registers[k] =
              _mm256_add_epi16(registers[k], added_columns[a][tile + k]);
        }
      }
      for (IndexType k = 0; k < kNumRegisters; ++k) {
        out[tile + k] = registers[k];
      }
    }
    static_assert(kPsqtBuckets * sizeof(PsqtWeightType) == sizeof(__m256i));
    __m256i psqt =
        _mm256_loadu_si256(reinterpret_cast<const __m256i*>(psqt_input));
    for (const auto index : removed) {
      psqt = _mm256_sub_epi32(
          psqt, _mm256_loadu_si256(reinterpret_cast<const __m256i*>(
                    &psqt_weights[index * kPsqtBuckets])));
    }
    for (const auto index : added) {
      psqt = _mm256_add_epi32(
          psqt, _mm256_loadu_si256(reinterpret_cast<const __m256i*>(
                    &psqt_weights[index * kPsqtBuckets])));
    }
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(psqt_output), psqt);
#else
    if (output != input) {
      std::memcpy(output, input, sizeof(BiasType) * kHalfDimensions);
      std::memcpy(psqt_output, psqt_input,
                  sizeof(PsqtWeightType) * kPsqtBuckets);
    }
    for (const auto index : removed) {
      const IndexType offset = kHalfDimensions * index;
      for (IndexType i = 0; i < kHalfDimensions; ++i) {
        output[i] = static_cast<BiasType>(output[i] - weights[offset + i]);
      }
      for (IndexType i = 0; i < kPsqtBuckets; ++i) {
        psqt_output[i] -= psqt_weights[index * kPsqtBuckets + i];
      }
    }
    for (const auto index : added) {
      const IndexType offset = kHalfDimensions * index;
      for (IndexType i = 0; i < kHalfDimensions; ++i) {
        output[i] = static_cast<BiasType>(output[i] + weights[offset + i]);
      }
      for (IndexType i = 0; i < kPsqtBuckets; ++i) {
        psqt_output[i] += psqt_weights[index * kPsqtBuckets + i];
      }
    }
#endif
  }

  void ApplyRefresh(RefreshEntry<kHalfDimensions>& entry,
                    const Features::IndexList& removed,
                    const Features::IndexList& added, BiasType* output,
                    PsqtWeightType* psqt_output) const {
#if defined(SCE_NNUE_AVX2)
    constexpr IndexType kNumRegisters = BestRegisterCount();
    constexpr IndexType kTileHeight =
        kNumRegisters * sizeof(__m256i) / sizeof(BiasType);
    static_assert(kHalfDimensions % kTileHeight == 0);
    __m256i registers[kNumRegisters];
    for (IndexType j = 0; j < kHalfDimensions / kTileHeight; ++j) {
      auto entry_tile =
          reinterpret_cast<__m256i*>(&entry.accumulation[j * kTileHeight]);
      auto output_tile = reinterpret_cast<__m256i*>(&output[j * kTileHeight]);
      for (IndexType k = 0; k < kNumRegisters; ++k) {
        registers[k] = entry_tile[k];
      }
      for (const auto index : removed) {
        const auto column = reinterpret_cast<const __m256i*>(
            &weights[kHalfDimensions * index + j * kTileHeight]);
        for (IndexType k = 0; k < kNumRegisters; ++k) {
          registers[k] = _mm256_sub_epi16(registers[k], column[k]);
        }
      }
      for (const auto index : added) {
        const auto column = reinterpret_cast<const __m256i*>(
            &weights[kHalfDimensions * index + j * kTileHeight]);
        for (IndexType k = 0; k < kNumRegisters; ++k) {
          registers[k] = _mm256_add_epi16(registers[k], column[k]);
        }
      }
      for (IndexType k = 0; k < kNumRegisters; ++k) {
        _mm256_store_si256(&entry_tile[k], registers[k]);
        _mm256_store_si256(&output_tile[k], registers[k]);
      }
    }
#else
    for (const auto index : removed) {
      const IndexType offset = kHalfDimensions * index;
      for (IndexType i = 0; i < kHalfDimensions; ++i) {
        entry.accumulation[i] =
            static_cast<BiasType>(entry.accumulation[i] - weights[offset + i]);
      }
    }
    for (const auto index : added) {
      const IndexType offset = kHalfDimensions * index;
      for (IndexType i = 0; i < kHalfDimensions; ++i) {
        entry.accumulation[i] =
            static_cast<BiasType>(entry.accumulation[i] + weights[offset + i]);
      }
    }
    std::memcpy(output, entry.accumulation, sizeof(BiasType) * kHalfDimensions);
#endif
    for (const auto index : removed) {
      for (IndexType i = 0; i < kPsqtBuckets; ++i) {
        entry.psqt_accumulation[i] -= psqt_weights[index * kPsqtBuckets + i];
      }
    }
    for (const auto index : added) {
      for (IndexType i = 0; i < kPsqtBuckets; ++i) {
        entry.psqt_accumulation[i] += psqt_weights[index * kPsqtBuckets + i];
      }
    }
    std::memcpy(psqt_output, entry.psqt_accumulation,
                sizeof(PsqtWeightType) * kPsqtBuckets);
  }

  void ClearEntry(RefreshEntry<kHalfDimensions>& entry) const {
    std::memcpy(entry.accumulation, biases, sizeof(biases));
    std::memset(entry.psqt_accumulation, 0, sizeof(entry.psqt_accumulation));
    entry.by_color = {};
    entry.by_type = {};
  }

  alignas(kCacheLineSize) BiasType biases[kHalfDimensions];
  alignas(kCacheLineSize)
      WeightType weights[kHalfDimensions * kInputDimensions];
  alignas(kCacheLineSize)
      PsqtWeightType psqt_weights[kInputDimensions * kPsqtBuckets];

 private:
#if defined(SCE_NNUE_AVX2)
  static constexpr IndexType BestRegisterCount() {
    constexpr IndexType kMaxRegisters = 16;
    constexpr IndexType kIdeal =
        kHalfDimensions * sizeof(BiasType) / sizeof(__m256i);
    if (kIdeal <= kMaxRegisters) return kIdeal;
    for (IndexType divisor = kMaxRegisters; divisor > 1; --divisor) {
      if (kIdeal % divisor == 0) return divisor;
    }
    return 1;
  }

  static void PermuteBlocks(std::byte* data, const std::size_t total_size) {
    constexpr std::size_t kBlockSize = 16;
    constexpr std::array<std::size_t, 8> kOrder = {0, 2, 1, 3, 4, 6, 5, 7};
    constexpr std::size_t kChunkSize = kBlockSize * kOrder.size();
    std::array<std::byte, kChunkSize> buffer{};
    for (std::size_t i = 0; i < total_size; i += kChunkSize) {
      std::byte* values = data + i;
      for (std::size_t j = 0; j < kOrder.size(); ++j) {
        std::memcpy(&buffer[j * kBlockSize], values + kOrder[j] * kBlockSize,
                    kBlockSize);
      }
      std::memcpy(values, buffer.data(), kChunkSize);
    }
  }

  void PermuteWeights() {
    static_assert(sizeof(biases) % 128 == 0);
    PermuteBlocks(reinterpret_cast<std::byte*>(biases), sizeof(biases));
    PermuteBlocks(reinterpret_cast<std::byte*>(weights), sizeof(weights));
  }
#else
  void PermuteWeights() {}
#endif
};
}  // namespace SimpleChessEngine::Nnue
